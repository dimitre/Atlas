//#include "ofMainLite.h"
#include "ofApp.h"
#include "ofMain.h"

int main() {
	ofWindowSettings settings;
	settings.setSize(1620, 920);
	settings.windowMode = OF_WINDOW;
	auto win { ofCreateWindow(settings) };
	auto app { make_shared<ofApp>() };
	ofRunApp(win, app);
	ofRunMainLoop();
}
