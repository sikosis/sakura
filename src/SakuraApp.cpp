#include "SakuraApp.h"

#include "AboutWindow.h"
#include "MainWindow.h"


SakuraApp::SakuraApp() : BApplication(kAppSignature), fMainWindow(NULL) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SakuraApp::AboutRequested() {
	(new AboutWindow())->Show();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SakuraApp::ReadyToRun() {
	fMainWindow = new MainWindow();
	fMainWindow->Show();
}
//---------------------------------------------------------------------------------------------------------------------------------//


int main() {
	SakuraApp application;
	application.Run();
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//
