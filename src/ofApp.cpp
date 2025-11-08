#include "ofApp.h"

void ofApp::setup() {
	cout << ofToDataPath("") << endl;

	leds.ui = &u.uis["leds"];
	fish.ui = &u.uis["fish"];
	retangulo.ui = &u.uis["rect"];

	cout << "CWD " << std::filesystem::current_path() << endl;
	cout << "--------" << endl;

	ofSetCircleResolution(96);
	//	ofAddListener(uiUVC->uiEvent, this, &ofApp::uiEvents);
	ofAddListener(uiCam->uiEvent, this, &ofApp::uiEventsCam);
	int fps = 60;
	ofSetFrameRate(fps);
	webcam.listDevices();
	// webcam.setDeviceID(0);
	// webcam.setDesiredFrameRate(fps);
	// webcam.setup(res.x, res.y);

	// fbo->allocate(res.x, res.y * 2, GL_RGBA);

	//	webcam.setPixelFormat(OF_PIXELS_BGR);
	//	webcam.setPixelFormat(OF_PIXELS_GRAY);
	// img.allocate(webcam.getWidth(), webcam.getHeight(), OF_IMAGE_COLOR);

	// #ifdef VIDEOWRITER
	// 	writer.setFbo(fbo);
	// 	writer.setFps(fps);
	// 	writer.setScale(1);
	// #endif

	ofxOscSenderSettings set;
	set.port = 8000;

	string ip = ofTrim(ofBufferFromFile("_osc_ip.txt").getText());
	if (ip.empty()) ip = "127.0.0.1";
	set.host = ip;
	cout << "setting OSC to ip " << ip << endl;

	// Enable broadcast mode if IP ends with 255
	size_t lastDot = ip.find_last_of('.');
	set.broadcast = (lastDot != string::npos && ip.substr(lastDot + 1) == "255");
	sender.setup(set);

	velocidade.trigger = [this]() {
		//		cout << "TRIGGER inside lambda" << endl;
		ofxOscMessage m;
		m.setAddress("/trigger");
		m.addIntArg(1);
		sender.sendMessage(m, false);

		ofSetColor(0, 255, 0);
		ofDrawRectangle(10, 10, 620, 460);
	};
}

void ofApp::update() { }

void ofApp::draw() {
	checkCamera();

	if (!webcam.isInitialized() || webcam.getWidth() == 0 || webcam.getHeight() == 0) {
		return;
	}

	if (ofGetElapsedTimef() > nextJump) {
		ofxOscMessage m;
		m.setAddress("/tempo");
		m.addFloatArg(ofGetElapsedTimef());
		sender.sendMessage(m, false);
		nextJump = ofGetElapsedTimef() + 1.0f;
		// ofSetWindowTitle(ofToString(ofGetFrameRate()));
	}

	webcam.update();
	//	cout << "test" << endl;
	if (webcam.isFrameNew()) {

		velocidade.alphaSpeed = ui->pFloat["alphaSpeed"];
		velocidade.speedThreshold = ui->pFloat["speedThreshold"];

		pts.clear();

		// trocar pra inicialização local.
		cv::Mat inputImage(webcam.getHeight(), webcam.getWidth(), CV_8UC3, webcam.getPixels().getData());

		if (inputImage.empty()) {
			cout << "inputimage empty" << endl;
			return;
		}

		// int margem = uiCv->pInt["margem"];
		// cv::Rect roi(margem, 0, webcam.getWidth() - margem * 2, webcam.getHeight());

		int largura = webcam.getWidth() - (uiCv->pInt["margemEsquerda"] + uiCv->pInt["margemDireita"]);
		cv::Rect roi(uiCv->pInt["margemEsquerda"], 0, largura, webcam.getHeight() - uiCv->pInt["margemBaixo"]);
		inputImage = inputImage(roi).clone();

		if (ui->pBool["brco_on"]) {
			inputImage.convertTo(inputImage, -1, ui->pFloat["contrast"], ui->pFloat["brightness"]);
		}

		if (ui->pBool["blur_on"]) {
			int b = 1 + ui->pInt["blur"] * 2;
			cv::GaussianBlur(inputImage, inputImage, cv::Size(b, b), 0); // Apply Gaussian blur with a 9x9 kernel
			//cv::blur(inputImage, inputImage, int(ui->pInt["blur"]));
		}

		if (ui->pBool["threshold_on"]) {
			double threshold_value = ui->pFloat["threshold"];
			double max_value = 255;
			cv::threshold(inputImage, inputImage, threshold_value, max_value, cv::THRESH_BINARY);
		}

		cv::SimpleBlobDetector::Params params;
		params.filterByArea = true;
		params.minArea = uiCv->pInt["minArea"]; // Minimum blob area
		params.maxArea = uiCv->pInt["maxArea"]; // Maximum blob area

		if (params.maxArea < params.minArea) {
			cout << "ERROR, maxArea < minArea" << endl;
			return;
		}

		params.filterByCircularity = true;
		//		params.minCircularity = 0.8; // Minimum circularity (0.0 to 1.0)
		params.minCircularity = uiCv->pFloat["minCircularity"];

		params.filterByConvexity = true;
		//		params.minConvexity = 0.9;
		params.minConvexity = uiCv->pFloat["minConvexity"];

		params.filterByInertia = true;
		params.minInertiaRatio = 0.1; // Minimum inertia ratio
		//		params.filterByColor = true;
		//		params.blobColor = 255; // 0 for dark blobs, 255 for light blobs
		params.filterByColor = false;
		//		params.blobColor = 255; // 0 for dark blobs, 255 for light blobs

		cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params); // Or SimpleBlobDetector::create() for default params

		std::vector<cv::KeyPoint> keypoints;
		detector->detect(inputImage, keypoints);

		for (auto & k : keypoints) {
			pts.emplace_back(glm::vec2(k.pt.x, k.pt.y), k.size);
		}

		cv::drawKeypoints(inputImage, keypoints, inputImage, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

		fbo->begin();
		ofClear(100, 255);
		ofSetColor(255);
		ofDrawBitmapString(ofToString(ofGetFrameRate()), 20, 30);

		//		std::memcpy(img.getPixels().getData(), inputImage.data, img.getPixels().size());
		img.setFromPixels(inputImage.data, inputImage.cols, inputImage.rows, OF_IMAGE_COLOR);
		img.update();
		img.draw(0, 0);

		webcam.draw(0, webcam.getHeight());
		ofDrawLine(0, img.getHeight(), img.getWidth(), img.getHeight());
		// retangulo.setDimensions({ webcam.getWidth(), webcam.getHeight() });

		retangulo.setDimensions({ img.getWidth(), img.getHeight() });
		retangulo.draw();

		if (pts.size() == 2) {
			std::sort(pts.begin(), pts.end(), [](const pt & a, const pt & b) {
				return a.size > b.size;
			});

			ofSetColor(255, 0, 80, 180);
			for (auto & p : pts) {
				p.draw();
			}
			ofDrawLine(pts[0].pos.x, pts[0].pos.y, pts[1].pos.x, pts[1].pos.y);
			glm::vec2 dist { pts[1].pos - pts[0].pos };
			float angle { glm::degrees(std::atan2(dist.y, dist.x)) + ui->pFloat["offsetAngle"] };
			glm::vec2 pos { (pts[1].pos + pts[0].pos) / 2.0f };
			velocidade.setPos(pos);
			float distance = glm::distance(pts[1].pos, pts[0].pos);
			float z = ofMap(distance, ui->pFloat["minDistanceZ"], ui->pFloat["maxDistanceZ"], -1.0f, 1.0f);

			if (ui->pFloat["rampZ"]) {
				float distMeio = std::abs(webcam.getWidth() * 0.5 - pos.x);
				z += ofMap(distMeio, 0, webcam.getWidth() * 0.5, 0, ui->pFloat["rampZ"]);
			}
			if (ui->pFloat["clampZ"]) {
				z = ofClamp(z, -1.0f, 1.0f);
			}

			float remap { 1.0f };
			// float aspect = webcam.getHeight() / (float)webcam.getWidth();
			// glm::vec2 posMap {
			// 	ofMap(pos.x, 0, webcam.getWidth(), -remap, remap),
			// 	ofMap(pos.y, 0, webcam.getHeight(), -remap * aspect, remap * aspect)
			// };

			glm::vec2 posMap {
				ofMap(pos.x, retangulo.rect.x, retangulo.rect.x + retangulo.rect.width, -remap, remap),
				ofMap(pos.y, retangulo.rect.y, retangulo.rect.y + retangulo.rect.height, -remap, remap)
			};

			if (ui->pBool["flipX"]) {
				posMap.y *= -1.0;
			}
			if (ui->pBool["flipY"]) {
				posMap.x *= -1.0;
			}

			float a = ofMap(angle, -180, 180, -1.0f, 1.0f);

			suave.angleAlpha = ui->pFloat["angleAlpha"];
			suave.posAlpha = ui->pFloat["posAlpha"];
			suave.setPos(posMap);
			suave.setAngle(a);

			xyza = glm::vec4 {
				suave.posEasy.x,
				suave.posEasy.y,
				z,
				suave.angleEasy
			};

			orienta.set(posMap, angle);
			// set data for prediction position angle, etc.
			orienta.setXyza(xyza);

			if (velocidade.speed > uiLeds->pFloat["speedThresholdRandom"]) {
				uiLeds->set("random", true);
			} else {
				uiLeds->set("random", false);
			}

			((ofxMicroUI::inspector *)ui->getElement("iPos"))->set("pos: " + ofToString(pos.x) + " x " + ofToString(pos.y));
			((ofxMicroUI::inspector *)ui->getElement("iPos2"))->set("xy: " + ofToString(xyza.x) + "  : " + ofToString(xyza.y));
			((ofxMicroUI::inspector *)ui->getElement("ia"))->set("a: " + ofToString(a));
			((ofxMicroUI::inspector *)ui->getElement("ispeed"))->set("speed: " + ofToString(velocidade.speed));
			((ofxMicroUI::inspector *)ui->getElement("idist"))->set("distance: " + ofToString(distance));
			((ofxMicroUI::inspector *)ui->getElement("iz"))->set("z: " + ofToString(z));
		} else {
			xyza = orienta.getXyza();
		}

		fish.xyza = xyza;
		fish.setVal(xyza.y);
		fish.send();

		ofxOscBundle bundle;
		{
			ofxOscMessage m;
			m.setAddress("/y");
			m.addFloatArg(xyza.x);
			bundle.add(m);
		}
		{
			ofxOscMessage m;
			m.setAddress("/x");
			m.addFloatArg(xyza.y);
			bundle.add(m);
		}
		{
			ofxOscMessage m;
			m.setAddress("/z");
			m.addFloatArg(xyza.z);
			bundle.add(m);
		}
		{
			ofxOscMessage m;
			m.setAddress("/a");
			m.addFloatArg(xyza.w);
			bundle.add(m);
		}
		{
			ofxOscMessage m;
			m.setAddress("/speed");
			m.addFloatArg(velocidade.speed);
			bundle.add(m);
		}

		sender.send(bundle);

		ofPushMatrix();
		float mult = 5.0f;
		ofTranslate(20, 20);
		ofSetColor(0, 255, 0);
		ofNoFill();
		ofDrawRectangle(0, 0, 200, 20);
		float x = ui->pFloat["speedThreshold"] * mult;
		ofDrawLine(x, 0, x, 20);
		ofFill();
		ofDrawRectangle(0, 0, velocidade.speed * mult, 20);
		ofPopMatrix();

		leds.draw();
		leds.send();

		fbo->end();
#ifdef VIDEOWRITER
		writer.addFrame();
#endif
	}
	soft.drawFbo();

	if (ui->pBool["testRect"]) {
		//		cout << "------" << endl;
		//		cout << xyza.x << endl;
		//		cout << xyza.y << endl;
		//		cout << xyza.z << endl;
		//		cout << xyza.a << endl;

		ofPushMatrix();
		float x = ofMap(xyza.x, -1, 1, 0, ofGetWindowWidth());
		float y = ofMap(xyza.y, -1, 1, 0, ofGetWindowHeight());
		ofTranslate(x, y);
		float ang = ofMap(xyza.w, -1, 1, -180.0f, 180.0f);
		ofRotateZDeg(ang);
		ofDrawRectangle(-100, -50, 200, 100);
		ofPopMatrix();
	}
}

void ofApp::keyPressed(int key) {
	if (key == 't') {
		fish.toggle();
	}
	// if (key == 'q') {
	// 	webcam.getDevicesInfo();
	// }
	if (key == '0') {
		webcam.close();
		webcam.setDeviceID(0);
	}
	if (key == '1') {
		webcam.close();
		webcam.setDeviceID(1);
	}
	if (key == '2') {
		webcam.close();
		webcam.setDeviceID(2);
	}
#ifdef VIDEOWRITER
	if (key == 'r') {
		writer.toggleRecording();
	}
#endif
}

void ofApp::uiEventsCam(ofxMicroUI::element & e) {
	if (e.name == "device" || e.name == "framerate" || e.name == "res") {
		reopenCamera = true;
	}
}
