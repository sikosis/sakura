#ifndef SAKURA_MAIN_WINDOW_H
#define SAKURA_MAIN_WINDOW_H


#include <Entry.h>
#include <String.h>
#include <Window.h>

#include <vector>


class BCardLayout;
class BButton;
class BFilePanel;
class BListView;
class BSlider;
class BStringView;
class BTabView;
class BView;
class AudioPlayer;
class LocalLibraryScanner;


class MainWindow : public BWindow {
public:
	MainWindow();
	virtual ~MainWindow();

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
	void _AddFolder(const entry_ref& folder);
	void _PlaySelectedTrack();
	void _RescanLibrary();
	void _SelectPage(int32 index);
	void _SetPlaybackState(bool playing);

	BCardLayout* fCardLayout;
	AudioPlayer* fAudioPlayer;
	BFilePanel* fFolderPanel;
	BListView* fFoldersList;
	BListView* fNavigation;
	BListView* fSongsList;
	BStringView* fPageTitle;
	BStringView* fSourceStatus;
	BStringView* fTrackArtist;
	BStringView* fTrackTitle;
	BTabView* fLibraryTabs;
	BButton* fPlayButton;
	LocalLibraryScanner* fScanner;
	BSlider* fVolumeSlider;
	std::vector<BString> fFolderPaths;
	int32 fTrackCount;
};


#endif
