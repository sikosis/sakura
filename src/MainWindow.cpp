#include "MainWindow.h"

#include "AudioPlayer.h"
#include "LocalLibraryScanner.h"
#include "Messages.h"
#include "TrackListItem.h"

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <Font.h>
#include <FilePanel.h>
#include <GroupView.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <Size.h>
#include <StringItem.h>
#include <StringView.h>
#include <TabView.h>
#include <TextControl.h>
#include <View.h>

#include <Entry.h>
#include <Path.h>

#include <cstring>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


static const char* kPageTitles[] = {
	"Library",
	"Sources",
	"Now Playing",
	"Playlists",
	"Settings"
};


static BStringView* PageHeading(const char* name, const char* text) {
	BStringView* heading = new BStringView(name, text);
	BFont font(*be_bold_font);
	font.SetSize(be_plain_font->Size() * 1.6f);
	heading->SetFont(&font);
	return heading;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BView* CreateCollectionTab(const char* name, const char* emptyMessage) {
	BView* view = new BView(name, B_WILL_DRAW);
	view->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	BStringView* message = new BStringView("empty collection message", emptyMessage);
	message->SetAlignment(B_ALIGN_CENTER);
	message->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 0.0f)
		.SetInsets(24.0f)
		.AddGlue()
		.Add(message)
		.AddGlue();
	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


MainWindow::MainWindow()
	: BWindow(BRect(80, 80, 1180, 780), "Sakura", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS), fCardLayout(NULL), fAudioPlayer(NULL), fFolderPanel(NULL),
		fFoldersList(NULL), fNavigation(NULL), fSongsList(NULL), fPageTitle(NULL),
		fSourceStatus(NULL), fTrackArtist(NULL), fTrackTitle(NULL), fLibraryTabs(NULL),
		fPlayButton(NULL), fScanner(NULL), fVolumeSlider(NULL), fTrackCount(0) {
	BMenuBar* menuBar = new BMenuBar("menu bar");
	BMenu* applicationMenu = new BMenu(B_TRANSLATE("Sakura"));
	applicationMenu->AddItem(new BMenuItem(B_TRANSLATE("About Sakura" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));
	applicationMenu->AddSeparatorItem();
	applicationMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(applicationMenu);

	BMenu* libraryMenu = new BMenu(B_TRANSLATE("Library"));
	libraryMenu->AddItem(new BMenuItem(B_TRANSLATE("Add music folder" B_UTF8_ELLIPSIS),
		new BMessage(kMessageAddFolder), 'O'));
	libraryMenu->AddItem(new BMenuItem(B_TRANSLATE("Rescan library"),
		new BMessage(kMessageRescan), 'R'));
	menuBar->AddItem(libraryMenu);

	fNavigation = new BListView("navigation", B_SINGLE_SELECTION_LIST);
	fNavigation->SetSelectionMessage(new BMessage(kMessagePageSelected));
	for (int32 index = 0; index < 5; index++)
		fNavigation->AddItem(new BStringItem(B_TRANSLATE(kPageTitles[index])));
	BScrollView* navigationScroll = new BScrollView("navigation scroll", fNavigation,
		B_FRAME_EVENTS, false, false, B_FANCY_BORDER);
	navigationScroll->SetExplicitMinSize(BSize(125.0f, B_SIZE_UNSET));
	navigationScroll->SetExplicitMaxSize(BSize(200.0f, B_SIZE_UNLIMITED));
	navigationScroll->SetExplicitAlignment(
		BAlignment(B_ALIGN_LEFT, B_ALIGN_USE_FULL_HEIGHT));

	fPageTitle = PageHeading("page title", B_TRANSLATE("Library"));
	fPageTitle->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));

	BView* content = new BView("page content", B_WILL_DRAW);
	content->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	content->SetExplicitAlignment(
		BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));
	content->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fCardLayout = new BCardLayout();
	content->SetLayout(fCardLayout);
	fCardLayout->AddView(_CreateLibraryView());
	fCardLayout->AddView(_CreateSourcesView());
	fCardLayout->AddView(_CreateNowPlayingView());
	fCardLayout->AddView(_CreatePlaylistsView());
	fCardLayout->AddView(_CreateSettingsView());

	BView* workspace = new BView("workspace", B_WILL_DRAW);
	workspace->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	workspace->SetExplicitAlignment(
		BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));
	workspace->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	BLayoutBuilder::Group<>(workspace, B_VERTICAL, 10.0f)
		.SetInsets(12.0f)
		.Add(fPageTitle)
		.Add(content, 1.0f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(menuBar)
		.AddGroup(B_HORIZONTAL, 0.0f, 1.0f)
			.Add(navigationScroll, 0.0f)
			.Add(new BSeparatorView(B_VERTICAL))
			.Add(workspace, 1.0f)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(_CreatePlayerBar());

	SetSizeLimits(640.0f, B_SIZE_UNLIMITED, 440.0f, B_SIZE_UNLIMITED);
	fFolderPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
		B_DIRECTORY_NODE, true);
	fAudioPlayer = new AudioPlayer(BMessenger(this));
	fScanner = new LocalLibraryScanner(BMessenger(this));
	fScanner->Run();
	fNavigation->Select(0);
	_SelectPage(0);
}
//---------------------------------------------------------------------------------------------------------------------------------//


MainWindow::~MainWindow() {
	delete fFolderPanel;
	delete fAudioPlayer;
	if (fScanner != NULL && fScanner->Lock())
		fScanner->Quit();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void MainWindow::MessageReceived(BMessage* message) {
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			be_app->AboutRequested();
			break;

		case kMessagePageSelected:
			_SelectPage(fNavigation->CurrentSelection());
			break;

		case kMessageAddFolder:
			_SelectPage(1);
			fFolderPanel->Show();
			break;

		case kMessageRescan:
			_RescanLibrary();
			break;

		case B_REFS_RECEIVED:
		{
			entry_ref folder;
			for (int32 index = 0; message->FindRef("refs", index, &folder) == B_OK; index++)
				_AddFolder(folder);
			break;
		}

		case kMessageScanStarted:
			fSourceStatus->SetText(B_TRANSLATE("Scanning local music" B_UTF8_ELLIPSIS));
			break;

		case kMessageTrackFound:
		{
			const char* path = NULL;
			const char* title = NULL;
			if (message->FindString("title", &title) == B_OK
				&& message->FindString("path", &path) == B_OK) {
				fSongsList->AddItem(new TrackListItem(title, path));
				fTrackCount++;
			}
			break;
		}

		case kMessageSongSelected:
		{
			TrackListItem* item = dynamic_cast<TrackListItem*>(
				fSongsList->ItemAt(fSongsList->CurrentSelection()));
			if (item != NULL) {
				if (!fAudioPlayer->IsPlaying()) {
					fTrackTitle->SetText(item->Text());
					fTrackArtist->SetText(B_TRANSLATE("Local music"));
				}
				fPlayButton->SetEnabled(true);
			}
			break;
		}

		case kMessagePlaySelection:
			_PlaySelectedTrack();
			break;

		case kMessageTogglePlayback:
			if (fAudioPlayer->IsPlaying()) {
				fAudioPlayer->Stop();
				_SetPlaybackState(false);
			} else {
				_PlaySelectedTrack();
			}
			break;

		case kMessagePlaybackFinished:
			fAudioPlayer->Stop();
			_SetPlaybackState(false);
			break;

		case kMessageVolumeChanged:
			fAudioPlayer->SetVolume(fVolumeSlider->Value() / 100.0f);
			break;

		case kMessageScanFinished:
		{
			BString status;
			status.SetToFormat(B_TRANSLATE("%ld songs indexed"), (long)fTrackCount);
			fSourceStatus->SetText(status.String());
			fLibraryTabs->Select(3);
			break;
		}

		case B_CANCEL:
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool MainWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView* MainWindow::_CreateLibraryView() {
	BView* view = new BView("library page", B_WILL_DRAW);
	view->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	view->SetExplicitAlignment(
		BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));
	view->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	BTextControl* search = new BTextControl("search", "", "Search your library", NULL);
	search->SetExplicitMaxSize(BSize(360.0f, B_SIZE_UNSET));
	search->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
	BButton* allSources = new BButton("all sources", B_TRANSLATE("All sources"), NULL);
	allSources->SetEnabled(false);
	allSources->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));

	fLibraryTabs = new BTabView("collection tabs", B_WIDTH_FROM_LABEL);
	fLibraryTabs->SetExplicitAlignment(
		BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));
	fLibraryTabs->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fLibraryTabs->AddTab(CreateCollectionTab(B_TRANSLATE("Artists"),
		B_TRANSLATE("Artists will appear here after your first library scan.")));
	fLibraryTabs->AddTab(CreateCollectionTab(B_TRANSLATE("Albums"),
		B_TRANSLATE("Album artwork will appear here after your first library scan.")));
	fLibraryTabs->AddTab(CreateCollectionTab(B_TRANSLATE("Genres"),
		B_TRANSLATE("Genres will appear here after your first library scan.")));
	fSongsList = new BListView("songs", B_SINGLE_SELECTION_LIST);
	fSongsList->SetSelectionMessage(new BMessage(kMessageSongSelected));
	fSongsList->SetInvocationMessage(new BMessage(kMessagePlaySelection));
	fLibraryTabs->AddTab(new BScrollView(B_TRANSLATE("Songs"), fSongsList, B_FRAME_EVENTS,
		false, true, B_FANCY_BORDER));
	fFoldersList = new BListView("folders", B_SINGLE_SELECTION_LIST);
	fLibraryTabs->AddTab(new BScrollView(B_TRANSLATE("Folders"), fFoldersList, B_FRAME_EVENTS,
		false, true, B_FANCY_BORDER));

	BLayoutBuilder::Group<>(view, B_VERTICAL, 12.0f)
		.AddGroup(B_HORIZONTAL, 8.0f)
			.Add(search, 0.0f)
			.Add(allSources, 0.0f)
			.AddGlue()
		.End()
		.Add(fLibraryTabs, 1.0f);
	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView* MainWindow::_CreateSourcesView() {
	BView* view = new BView("sources page", B_WILL_DRAW);
	view->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	BStringView* localTitle = new BStringView("local source title", B_TRANSLATE("Local music"));
	localTitle->SetFont(be_bold_font);
	BStringView* localDetail = new BStringView("local source detail", B_TRANSLATE(
		"Choose folders to build your library from music stored on this computer."));
	BButton* addFolder = new BButton("add folder", B_TRANSLATE("Add folder" B_UTF8_ELLIPSIS),
		new BMessage(kMessageAddFolder));
	fSourceStatus = new BStringView("local source status", B_TRANSLATE("No folders added"));
	fSourceStatus->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);

	BStringView* providerNote = new BStringView("provider note", B_TRANSLATE(
		"Remote providers will arrive after local library and playback support."));
	providerNote->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 8.0f)
		.SetInsets(12.0f)
		.Add(localTitle)
		.Add(localDetail)
		.Add(fSourceStatus)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(addFolder)
			.AddGlue()
		.End()
		.AddStrut(18.0f)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddStrut(10.0f)
		.Add(providerNote)
		.AddGlue();
	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView* MainWindow::_CreateNowPlayingView() {
	return _CreatePlaceholderPage("now playing page", B_TRANSLATE("Nothing is playing"),
		B_TRANSLATE("Choose a song from your library to see its artwork and queue here."));
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView* MainWindow::_CreatePlaylistsView() {
	return _CreatePlaceholderPage("playlists page", B_TRANSLATE("No playlists yet"),
		B_TRANSLATE("Your local and mixed-source playlists will appear here."));
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView* MainWindow::_CreateSettingsView() {
	return _CreatePlaceholderPage("settings page", B_TRANSLATE("Settings"),
		B_TRANSLATE("Audio, appearance, scanning, accounts and cache settings are coming next."));
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView* MainWindow::_CreatePlayerBar() {
	BView* bar = new BView("player bar", B_WILL_DRAW);
	bar->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BView* artwork = new BView("artwork placeholder", B_WILL_DRAW);
	artwork->SetViewUIColor(B_CONTROL_BACKGROUND_COLOR);
	artwork->SetExplicitMinSize(BSize(56.0f, 56.0f));
	artwork->SetExplicitMaxSize(BSize(56.0f, 56.0f));

	fTrackTitle = new BStringView("track title", B_TRANSLATE("Not playing"));
	fTrackTitle->SetFont(be_bold_font);
	fTrackArtist = new BStringView("track artist", B_TRANSLATE("Sakura"));
	fTrackArtist->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);

	BButton* previous = new BButton("previous", "◀◀", NULL);
	fPlayButton = new BButton("play", "▶", new BMessage(kMessageTogglePlayback));
	BButton* next = new BButton("next", "▶▶", NULL);
	previous->SetEnabled(false);
	fPlayButton->SetEnabled(false);
	next->SetEnabled(false);

	BSlider* position = new BSlider("position", "", NULL, 0, 100, B_HORIZONTAL);
	position->SetEnabled(false);
	position->SetExplicitMinSize(BSize(100.0f, B_SIZE_UNSET));
	fVolumeSlider = new BSlider("volume", B_TRANSLATE("Volume"),
		new BMessage(kMessageVolumeChanged), 0, 100, B_HORIZONTAL);
	fVolumeSlider->SetModificationMessage(new BMessage(kMessageVolumeChanged));
	fVolumeSlider->SetValue(80);
	fVolumeSlider->SetExplicitMaxSize(BSize(120.0f, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(bar, B_VERTICAL, 4.0f)
		.SetInsets(14.0f, 10.0f)
		.AddGroup(B_HORIZONTAL, 10.0f)
			.Add(artwork)
			.AddGroup(B_VERTICAL, 2.0f)
				.Add(fTrackTitle)
				.Add(fTrackArtist)
			.End()
			.AddGlue()
			.Add(previous)
			.Add(fPlayButton)
			.Add(next)
			.Add(fVolumeSlider)
		.End()
		.Add(position, 1.0f);
	return bar;
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView* MainWindow::_CreatePlaceholderPage(const char* name, const char* title,
	const char* detail) {
	BView* view = new BView(name, B_WILL_DRAW);
	view->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	BStringView* titleView = PageHeading("empty page title", title);
	titleView->SetAlignment(B_ALIGN_CENTER);
	BStringView* detailView = new BStringView("empty page detail", detail);
	detailView->SetAlignment(B_ALIGN_CENTER);
	detailView->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 8.0f)
		.AddGlue()
		.Add(titleView)
		.Add(detailView)
		.AddGlue();
	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void MainWindow::_AddFolder(const entry_ref& folder) {
	BPath path(&folder);
	if (path.InitCheck() != B_OK)
		return;

	for (size_t index = 0; index < fFolderPaths.size(); index++) {
		if (fFolderPaths[index] == path.Path())
			return;
	}

	fFolderPaths.push_back(BString(path.Path()));
	fFoldersList->AddItem(new BStringItem(path.Path()));
	fScanner->Scan(folder);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void MainWindow::_PlaySelectedTrack() {
	TrackListItem* item = dynamic_cast<TrackListItem*>(
		fSongsList->ItemAt(fSongsList->CurrentSelection()));
	if (item == NULL)
		return;

	entry_ref ref;
	status_t status = get_ref_for_path(item->Path(), &ref);
	if (status == B_OK)
		status = fAudioPlayer->Play(ref);
	if (status != B_OK) {
		BString message(B_TRANSLATE("Sakura could not play the selected track."));
		message << "\n\n" << strerror(status);
		(new BAlert("playback error", message.String(), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		_SetPlaybackState(false);
		return;
	}

	fTrackTitle->SetText(item->Text());
	fTrackArtist->SetText(B_TRANSLATE("Local music"));
	_SetPlaybackState(true);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void MainWindow::_RescanLibrary() {
	fSongsList->MakeEmpty();
	fTrackCount = 0;
	for (size_t index = 0; index < fFolderPaths.size(); index++) {
		entry_ref folder;
		if (get_ref_for_path(fFolderPaths[index].String(), &folder) == B_OK)
			fScanner->Scan(folder);
	}
	if (fFolderPaths.empty())
		fSourceStatus->SetText(B_TRANSLATE("No folders added"));
}
//---------------------------------------------------------------------------------------------------------------------------------//


void MainWindow::_SetPlaybackState(bool playing) {
	fPlayButton->SetLabel(playing ? "■" : "▶");
	fPlayButton->SetToolTip(playing ? B_TRANSLATE("Stop") : B_TRANSLATE("Play"));
}
//---------------------------------------------------------------------------------------------------------------------------------//


void MainWindow::_SelectPage(int32 index) {
	if (index < 0 || index >= 5)
		return;

	fCardLayout->SetVisibleItem(index);
	fPageTitle->SetText(B_TRANSLATE(kPageTitles[index]));
	if (fNavigation->CurrentSelection() != index)
		fNavigation->Select(index);
}
//---------------------------------------------------------------------------------------------------------------------------------//
