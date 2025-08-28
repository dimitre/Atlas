#include "ofApp.h"

void ofApp::setup(){
	ofSetCircleResolution(96);
	ofAddListener(uiUVC->uiEvent, this, &ofApp::uiEvents);
	ofAddListener(uiCam->uiEvent, this, &ofApp::uiEventsCam);
	int fps = 60;
	ofSetFrameRate(fps);
	webcam.listDevices();
	webcam.setDeviceID(1);
	webcam.setDesiredFrameRate(fps);
	webcam.setup(res.x, res.y);
//	webcam.setPixelFormat(OF_PIXELS_BGR);
//	webcam.setPixelFormat(OF_PIXELS_GRAY);
	img.allocate(webcam.getWidth(),webcam.getHeight(), OF_IMAGE_COLOR);
#ifdef VIDEOWRITER
	writer.setFbo(fbo);
	writer.setFps(fps);
	writer.setScale(1);
#endif

	
	
	
	ofxOscSenderSettings set;
//	set.host = "127.0.0.1";
	
	string ip = ofTrim(ofBufferFromFile("_osc_ip.txt").getText());
	set.host = ip;
	cout << "setting OSC to ip " << ip << endl;

	set.port = 8000;
	set.broadcast = true;
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

void ofApp::update(){}

void ofApp::draw(){
	if (ofGetElapsedTimef() > nextJump) {
		ofxOscMessage m;
		m.setAddress("/tempo");
		m.addFloatArg(ofGetElapsedTimef());
		sender.sendMessage(m, false);
		nextJump = ofGetElapsedTimef() + 1.0f;
	}
	
	
	if (!webcam.isInitialized()) {
		return;
	}

	if (webcam.getWidth() == 0) {
		return;
	}
	

	webcam.update();
//	cout << "test" << endl;
	if (webcam.isFrameNew()) {
		
		velocidade.alphaSpeed = ui->pFloat["alphaSpeed"];
		velocidade.speedThreshold = ui->pFloat["speedThreshold"];

		pts.clear();
		inputImage = cv::Mat(webcam.getHeight(), webcam.getWidth(), CV_8UC3, webcam.getPixels().getData());
		
		if (inputImage.empty()) {
			cout << "inputimage empty" << endl;
			return;
		}
		
		cv::SimpleBlobDetector::Params params;
		params.filterByArea = true;
//		params.minArea = 250; // Minimum blob area
//		params.maxArea = 35000; // Maximum blob area
		params.minArea = uiCv->pInt["minArea"]; // Minimum blob area
		params.maxArea = uiCv->pInt["maxArea"]; // Maximum blob area

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

		if (ui->pBool["blur1_on"]) {
			int b = 1 + ui->pInt["blur1"] * 2;
			cv::GaussianBlur(inputImage, inputImage, cv::Size(b, b), 0); // Apply Gaussian blur with a 9x9 kernel
		}

		
		if (ui->pBool["threshold_on"]) {
			double threshold_value = ui->pFloat["threshold"];
			double max_value = 255;
//			double max_value = ui->pFloat["thresholdMax"];

			cv::threshold(inputImage, inputImage, threshold_value, max_value, cv::THRESH_BINARY);
		}
		
		if (ui->pBool["blur_on"]) {
			int b = 1 + ui->pInt["blur"] * 2;
			cv::GaussianBlur(inputImage, inputImage, cv::Size(b, b), 0); // Apply Gaussian blur with a 9x9 kernel
		}
		//			cv::blur(inputImage, inputImage, int(ui->pInt["blur"]));

		std::vector<cv::KeyPoint> keypoints;
		  detector->detect(inputImage, keypoints);
		
		for (auto & k : keypoints) {
			pts.emplace_back(glm::vec2(k.pt.x, k.pt.y), k.size);
//			ofDrawBitmapString(ofToString(k.size), k.pt.x, k.pt.y + 30);
		}
		
		cv::drawKeypoints(inputImage, keypoints, inputImage, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

		fbo->begin();
		ofClear(40,255);
		ofSetColor(255);
		ofDrawBitmapString(ofToString(ofGetFrameRate()), 20, 30);
		
		std::memcpy(img.getPixels().getData(), inputImage.data, img.getPixels().size());
		img.update();
		img.draw(0, 0);
//		ofNoFill();
//		ofDrawRectangle(0, 0, img.getWidth(), img.getHeight());
		
		webcam.draw(0, webcam.getHeight());
		ofDrawLine(0, img.getHeight(), img.getWidth(), img.getHeight());

		
		if (pts.size() == 2) {
						std::sort(pts.begin(), pts.end(), [](const pt & a, const pt & b) {
							return a.size < b.size;
						});
			
			ofSetColor(255, 0, 80, 180);

			for (auto & p : pts) {
				p.draw();
				
			}
			ofDrawLine(pts[0].pos.x, pts[0].pos.y, pts[1].pos.x, pts[1].pos.y);
			
			glm::vec2 dist = pts[1].pos - pts[0].pos;
			
			float angle = glm::degrees(std::atan2(dist.y, dist.x));
			//		float a1 = ofRadToDeg(atan2(pts[1].pos.y - pontos[0].pos.y, pontos[1].pos.x - pontos[0].pos.x));
			
			glm::vec2 pos { (pts[1].pos + pts[0].pos) / 2.0f };
			
			velocidade.setPos(pos);
			
			float distance = glm::distance(pts[1].pos, pts[0].pos);
			

			float aspect = webcam.getWidth() / (float) webcam.getHeight();
			float x = ofMap(pos.x, 0, webcam.getWidth(), -0.5 * aspect, 0.5 * aspect);
			float y = ofMap(pos.y, 0, webcam.getHeight(), -0.5, 0.5);

//			ofxOscMessage m;
//			m.setAddress("/atlas");
//			m.addFloatArg(x);
//			m.addFloatArg(y);
//			m.addFloatArg(angle / 360.0f);
//			sender.sendMessage(m, false);
			float a = ofMap(angle, -180, 180, 0, 1);

			ofxOscBundle bundle;
			
			{
				ofxOscMessage m;
				m.setAddress("/x");
				m.addFloatArg(x);
				bundle.add(m);
//				sender.sendMessage(m, false);
			}

			{
				ofxOscMessage m;
				m.setAddress("/y");
				m.addFloatArg(y);
				bundle.add(m);

//				sender.sendMessage(m, false);
			}

			{
				ofxOscMessage m;
				m.setAddress("/a");
				m.addFloatArg(a);
				bundle.add(m);

//				sender.sendMessage(m, false);
			}
			
			{
				ofxOscMessage m;
				m.setAddress("/speed");
				m.addFloatArg(velocidade.speed);
				bundle.add(m);

//				sender.sendMessage(m, false);
			}
			
			{
				ofxOscMessage m;
				m.setAddress("/distance");
				m.addFloatArg(distance);
				bundle.add(m);

//				sender.sendMessage(m, false);
			}

			sender.send(bundle);

			
//			ofSetColor(0, 255, 0);
//			ofSetColor(255);
//			std::string msg {
//				ofToString(a) + "\n" +
//				ofToString(angle) + "\n" +
//				ofToString(pos.x) + " x " +
//				ofToString(pos.y) + "\n" +
//				ofToString(velocidade.speed) + "\n"
//			};
//			ofSetColor(0, 255, 0);
//			ofDrawBitmapString(msg, pos.x, pos.y);
//			ofDrawBitmapString(msg, 20, 120);
			
			((ofxMicroUI::inspector*)ui->getElement("i1"))->set("pos: " + ofToString(pos.x) + " x " + ofToString(pos.y));
			((ofxMicroUI::inspector*)ui->getElement("i2"))->set("a: " + ofToString(a));
			((ofxMicroUI::inspector*)ui->getElement("i3"))->set("angle: " + ofToString(angle));
			((ofxMicroUI::inspector*)ui->getElement("i4"))->set("speed: " + ofToString(velocidade.speed));
			((ofxMicroUI::inspector*)ui->getElement("i5"))->set("distance: " + ofToString(distance));
//			((ofxMicroUI::inspector*)ui->getElement("i1"))->set(ofToString(pos.x) + " x " + ofToString(pos.y));

		} else {
//			velocidade.idle();
		}
		
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
		
		
		fbo->end();
#ifdef VIDEOWRITER
		writer.addFrame();
#endif
	}
	

	
	soft.drawFbo();
	
	
}

void ofApp::keyPressed(int key){
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
	if (key == 'r') {
//		detect ^= 1;
//		drawCam ^= 1;
#ifdef VIDEOWRITER
		writer.toggleRecording();
#endif
	}

}


void ofApp::uiEvents(ofxMicroUI::element & e) {

	vector <string> params {
		"auto-exposure-mode",
		"exposure-time-abs",
		"gain",
		"brightness",
		"contrast",
		"backlight-compensation",
		"gamma",
		
//		"white-balance-temp",
//		"saturation",
//		"sharpness",
//		"hue",
//		"auto-white-balance-temp",
	};
	
	
	string command = "../../../uvc-util -I 0 -s power-line-frequency=2 ";
	//-s auto-exposure-mode=1
	for (auto & p : params) {
		command += "-s " + p + "=" + ofToString(uiUVC->pInt[p]) + " ";
	}
	cout << command << endl;
	ofSystem(command);
	cout << ofSystem("pwd") << endl;
}


void ofApp::uiEventsCam(ofxMicroUI::element & e) {
	if (e.name == "device") {
		webcam.close();
		webcam.setDeviceID(*e.i);
		webcam.setup(res.x, res.y);
	}
	else if (e.name == "framerate") {
		webcam.close();
		webcam.setDesiredFrameRate(*e.i);
		webcam.setup(res.x, res.y);
#ifdef VIDEOWRITER
		writer.setFps(*e.i);
#endif
	}
	else if (e.name == "res") {
		if (*e.s != "") {
			auto s = ofSplitString(*e.s, "x");
			res.x = ofToInt(s[0]);
			res.y = ofToInt(s[1]);
			webcam.close();
			webcam.setup(res.x, res.y);
			img.allocate(webcam.getWidth(),webcam.getHeight(), OF_IMAGE_COLOR);
			
			fbo->allocate(webcam.getWidth(), webcam.getHeight() * 2);

#ifdef VIDEOWRITER
			writer.setFbo(fbo);
#endif
		}
	}
	
}
