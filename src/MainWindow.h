#ifndef SAKURA_MAIN_WINDOW_H
#define SAKURA_MAIN_WINDOW_H


#include <Window.h>


class BCardLayout;
class BListView;
class BStringView;
class BView;


class MainWindow : public BWindow {
public:
	MainWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

private:
	BView* _CreateLibraryView();
	BView* _CreateNowPlayingView();
	BView* _CreatePlaylistsView();
	BView* _CreateSettingsView();
	BView* _CreateSourcesView();
	BView* _CreatePlayerBar();
	BView* _CreatePlaceholderPage(const char* name, const char* title,
		const char* detail);
	void _SelectPage(int32 index);

	BCardLayout* fCardLayout;
	BListView* fNavigation;
	BStringView* fPageTitle;
};


#endif
