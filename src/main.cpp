//#include "ofMainLite.h"
#include "ofMain.h"
#include "ofApp.h"

int main() {
	
	
	ofWindowSettings settings;
	settings.setSize(1480, 965);
//	settings.setSize(1000, 620);
//	settings.setSize(1000, 120);
	settings.windowMode = OF_WINDOW;
	auto win { ofCreateWindow(settings) };
	auto app { make_shared<ofApp>() };
	ofRunApp(win, app);
	ofRunMainLoop();
}
