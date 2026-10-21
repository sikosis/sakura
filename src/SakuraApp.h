#ifndef SAKURA_APP_H
#define SAKURA_APP_H


#include <Application.h>


class MainWindow;


const char* const kAppSignature = "application/x-vnd.sikosis-Sakura";


class SakuraApp : public BApplication {
public:
	SakuraApp();

	virtual void AboutRequested();
	virtual void ReadyToRun();

private:
	MainWindow* fMainWindow;
};


#endif
