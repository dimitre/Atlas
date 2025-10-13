#pragma once

#include "ofMain.h"

#include <opencv2/opencv.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::toMat4
#include <glm/mat4x4.hpp>               // glm::mat4
#include <glm/gtc/type_ptr.hpp>

#include "ofxMicroUI.h"
#include "ofxMicroUISoftware.h"

#include "ofxOsc.h"

//#define VIDEOWRITER
#ifdef VIDEOWRITER
#include "ofVideoWriter.h"
#endif

class ofApp : public ofBaseApp{
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
	ofxMicroUI * uiUVC { &u.uis["uvc"] };
	ofxMicroUI * uiCam { &u.uis["cam"] };
	ofxMicroUI * uiCv { &u.uis["cv"] };
	ofxMicroUI * uiH { &u.uis["hough"] };

	ofFbo * fbo { soft.fboFinal };

	ofxOscSender sender;
	glm::ivec2 res { 1280, 800 };

	// cv::Mat inputImage;
	cv::Mat inputCam;


#ifdef VIDEOWRITER
	ofVideoWriter writer;
#endif


    inline static float c2a (glm::vec2 xy) { return glm::degrees(std::atan2(xy.y, xy.x)); }
    inline static float r2x (float a, float m) { return m * std::cos(glm::radians(a)); }
	inline static float r2y (float a, float m) { return m * std::sin(glm::radians(a)); }

    struct orientacao {
        bool ok = false;
        glm::vec2 lastPos { 0.0f, 0.0f };
        glm::vec2 pos { 0.0f, 0.0f };
        glm::vec2 walk { 0.0f, 0.0f };

        float a = 0.0f;
        float m = 0.0f;
        void setPos(glm::vec2 p) {
            pos = p;
            walk = pos - lastPos;

            lastPos = pos;
            ok = true;
        }

        glm::vec2 project() {
            return lastPos + glm::vec2(r2x(a, m), r2y(a, m));
            // speed
            m *= 0.9f;
        }
    } orienta;


	struct pt {
		glm::vec2 pos;
		float size;

		void draw() {
			ofDrawCircle(pos.x, pos.y, size);
		}
	};

	vector <pt> pts;
	float nextJump = 0;


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

};
