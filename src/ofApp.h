#pragma once

#include "ofGraphics.h"
#include "ofMain.h"

#include <glm/gtc/matrix_transform.hpp> // glm::toMat4
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp> // glm::mat4
#include <opencv2/opencv.hpp>

#include "ofxMicroUI.h"
#include "ofxMicroUISoftware.h"

#include "ofxOsc.h"

// #define VIDEOWRITER
#ifdef VIDEOWRITER
	#include "ofVideoWriter.h"
#endif

#include "artnet2025.h"

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void uiEvents(ofxMicroUI::element & e);
	void uiEventsCam(ofxMicroUI::element & e);

	ofVideoGrabber webcam;
	ofImage img;

	ofxMicroUI u { "u.txt" };
	ofxMicroUISoftware soft { &u, 1 };
	ofxMicroUI * ui { &u.uis["ui"] };
	//	ofxMicroUI * uiUVC { &u.uis["uvc"] };
	ofxMicroUI * uiCam { &u.uis["cam"] };
	ofxMicroUI * uiCv { &u.uis["cv"] };
	ofxMicroUI * uiLeds { &u.uis["leds"] };
	ofxMicroUI * uiFish { &u.uis["fish"] };
	// ofxMicroUI * uiH { &u.uis["hough"] };

	ofFbo * fbo { soft.fboFinal };

	ofxOscSender sender;
	glm::ivec2 res { 1280, 720 };

	// cv::Mat inputImage;
	cv::Mat inputCam;

#ifdef VIDEOWRITER
	ofVideoWriter writer;
#endif

	inline static float c2a(glm::vec2 xy) { return glm::degrees(std::atan2(xy.y, xy.x)); }
	inline static float r2x(float a, float m) { return m * std::cos(glm::radians(a)); }
	inline static float r2y(float a, float m) { return m * std::sin(glm::radians(a)); }

	struct orientacao {
		bool ok = false;
		glm::vec2 lastPos { 0.0f, 0.0f };
		glm::vec2 pos { 0.0f, 0.0f };
		//        glm::vec2 walk { 0.0f, 0.0f };

		float angle = 0.0f;
		float lastAngle = 0.0f;
		float mag = 0.0f;

		float angularVel = 0.0f;

		void set(glm::vec2 p, float a) {
			pos = p;
			//            walk = pos - lastPos;
			mag = glm::distance(pos, lastPos);
			lastPos = pos;
			ok = true;

			angle = a;
			angularVel = a - lastAngle;
			lastAngle = a;
		}

		glm::vec4 xyza;

		void setXyza(glm::vec4 p) {
			xyza = p;
		}

		glm::vec4 getXyza() {
			// Fixme: predição

			angle += angularVel;
			glm::vec2 newPos = pos + glm::vec2(r2x(angle, mag), r2y(angle, mag));

			angularVel *= 0.9f;
			mag *= 0.9f;

			xyza.x = newPos.x;
			xyza.y = newPos.y;
			xyza.a = ofMap(angle, -180, 180, -1.0f, 1.0f);

			return xyza;
		}

		//        glm::vec2 project() {
		//        }
	} orienta;

	glm::vec4 xyza;

	struct pt {
		glm::vec2 pos;
		float size;

		void draw() {
			ofDrawCircle(pos.x, pos.y, size);
		}
	};

	vector<pt> pts;
	float nextJump = 2.0f;

	struct suaviza {
		float angleAlpha = 0.1f;
		float posAlpha = 0.1f;

		float angle = 0.0f;
		float angleEasy = 0.0f;
		glm::vec2 pos { 0.0f, 0.0f };
		glm::vec2 posEasy { 0.0f, 0.0f };

		void setAngle(float a) {
			angle = a;
			angleEasy = angle * angleAlpha + angleEasy * (1.0f - angleAlpha);
			// FIXME: ignora angulo
			//			angleEasy = angle;
		}

		void setPos(glm::vec2 p) {
			pos = p;
			posEasy = pos * posAlpha + posEasy * (1.0f - posAlpha);
			// FIXME: ignora angulo
			//			posEasy = pos;
		}
	} suave;

	// speed
	struct vel {
	public:
		glm::vec2 lastPos;
		glm::vec2 pos;
		float distance;
		float speed = 0.0f;
		bool triggered = false;
		void idle() {
			speed = 0.0f;
		}

		float alphaSpeed = 0.1f;
		float speedThreshold = 20.0f;

		std::function<void()> trigger = nullptr;

		void setPos(glm::vec2 p) {
			lastPos = pos;
			pos = p;
			distance = glm::distance(pos, lastPos);
			//			speed = distance * 0.7 + speed * 0.3;
			speed = distance * alphaSpeed + speed * (1.0f - alphaSpeed);

			if (speed > speedThreshold) {
				if (triggered == false) {
					//					trigger();
					triggered = true;
					//					cout << "TRIGGER " << ofGetElapsedTimef() << endl;
					if (trigger != nullptr) {
						trigger();
					}
				}
			}

			else if (speed < speedThreshold * 0.5f) {
				triggered = false;
				//				untrigger();
			}
		}
	} velocidade;

	struct ledsArtnet {
		ofxMicroUI * ui = nullptr;
		artuniverse art { 1, "10.1.91.71" };
		// artuniverse art2 { 1, "127.0.0.1" };

		void draw() {
			for (int a = 0; a < 15; a++) {
				// for (int a = 0; a < 15; a++) {
				float c { ofNoise(a * ui->pFloat["noiseIndex"], ofGetElapsedTimef() * ui->pFloat["noiseTime"]) };
				if (ui->pBool["random"]) {
					c = ofRandom(0, 10) > 5.0f ? 1.0f : 0.0f;
				}
				ofSetFloatColor(c, 1.0f);
				ofDrawRectangle((a + 1) * 20, 50, 20, 20);

				for (int b = 0; b < 4; b++) {
					int ch = a * 12 + b * 3;
					art.setChannel(ch + 0, 255 * c);
					art.setChannel(ch + 1, 255 * c);
					art.setChannel(ch + 2, 255 * c);
					// art2.setChannel(ch, 255 * c);
				}
			}
		}

		void send() {

			if (ui->pBool["send"]) {
				art.send();
				// art2.send();
			}
		}

	} leds;

	struct peixe {
		ofxMicroUI * ui = nullptr;
		ofxOscSender sender;
		ofxOscSender s2, s3;
		float amplitude = 30.0f;
		float a2 = 30.0f;
		float a3 = 30.0f;
		float val = 0.0f; // RAW Value for head axis
		glm::vec4 xyza;
		void setVal(float v) {
			val = v;

			amplitude = ofMap(
				ofClamp(val - ui->pFloat["floor"], 0.0f, 1.0f),
				0.0f, 1.0f - ui->pFloat["floor"],
				ui->pFloat["minAmplitude"], ui->pFloat["maxAmplitude"]);
		}

		bool started = false;

		void toggle() {
			ofxOscMessage m;
			started = !started;
			if (started) {
				m.setAddress("/fish/start");
			} else {
				m.setAddress("/fish/stop");
			}
			sender.sendMessage(m, false);
		}

		peixe() {
			ofxOscSenderSettings settings;
			settings.port = 8000;
			vector<string> ips { ofxMicroUI::textToVector("_osc_ip_peixe.txt") };
			// settings.host = ofTrim(ofBufferFromFile("_osc_ip_peixe.txt").getText());
			settings.host = ips.at(0);
			if (settings.host.empty()) {
				settings.host = "10.1.91.255";
				settings.broadcast = true;
			}
			cout << "setting OSC Peixe to ip " << settings.host << endl;
			sender.setup(settings);

			if (ips.size() > 1) {
				settings.host = ips.at(1);
				s2.setup(settings);
			}

			if (ips.size() > 2) {
				settings.host = ips.at(2);
				s3.setup(settings);
			}
		}

		void send() {
			if (ui->pBool["send"]) {
				// amplitude 105.
				if (sender.isReady()) {
					ofxOscMessage m;
					m.setAddress("/fish/amplitude");
					m.addFloatArg(100.0f);
					m.setAddress("/fish/min_speed");
					m.addFloatArg(100.0f);
					m.setAddress("/fish/max_speed");
					m.addFloatArg(ofMap(std::abs(xyza.x), 0.3, 0.5, 100.0f, 200.0f));
					sender.sendMessage(m, false);
				} else {
					cout << "sender is not ready" << endl;
				}
				if (s2.isReady()) {
					ofxOscMessage m;
					m.setAddress("/fish/amplitude");
					// FIXME: a2
					// m.addFloatArg(a2);
					m.addFloatArg(amplitude);

					s2.sendMessage(m, false);
				}
				if (s3.isReady()) {
					ofxOscMessage m;
					m.setAddress("/fish/amplitude");
					// FIXME: a3
					// m.addFloatArg(a3);
					m.addFloatArg(amplitude);
					s3.sendMessage(m, false);
				}
			}
		}
	} fish;

	struct rect {
		ofxMicroUI * ui = nullptr;
		glm::ivec2 dimensions { 1280, 720 };
		ofRectangle rect;
		glm::ivec2 offset { 0, 0 };

		void setDimensions(glm::ivec2 d) {
			dimensions = d;
			rect.width = dimensions.y * ui->pFloat["width"];
			rect.height = dimensions.y * ui->pFloat["height"];
			rect.x = dimensions.x * 0.5f - rect.width * 0.5f + ui->pFloat["offx"] * dimensions.y;
			rect.y = dimensions.y * 0.5f - rect.height * 0.5f + ui->pFloat["offy"] * dimensions.y;
		}

		void draw() {
			ofPushStyle();
			ofNoFill();
			ofSetColor(0, 255, 0);
			ofDrawRectangle(rect);
			ofPopStyle();
		}
	} retangulo;
};
