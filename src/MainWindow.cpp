#include "MainWindow.h"

#include <Application.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <Font.h>
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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	kMessagePageSelected = 'pgsl',
	kMessageAddFolder = 'adfd',
	kMessageRescan = 'rscn'
};


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
		B_AUTO_UPDATE_SIZE_LIMITS), fCardLayout(NULL), fNavigation(NULL),
		fPageTitle(NULL) {
	BMenuBar* menuBar = new BMenuBar("menu bar");
	BMenu* applicationMenu = new BMenu(B_TRANSLATE("Sakura"));
	applicationMenu->AddItem(new BMenuItem(B_TRANSLATE("About Sakura") B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	applicationMenu->AddSeparatorItem();
	applicationMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(applicationMenu);

	BMenu* libraryMenu = new BMenu(B_TRANSLATE("Library"));
	libraryMenu->AddItem(new BMenuItem(B_TRANSLATE("Add music folder") B_UTF8_ELLIPSIS,
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
	navigationScroll->SetExplicitMinSize(BSize(170.0f, B_SIZE_UNSET));
	navigationScroll->SetExplicitMaxSize(BSize(220.0f, B_SIZE_UNLIMITED));

	fPageTitle = PageHeading("page title", B_TRANSLATE("Library"));

	BView* content = new BView("page content", B_WILL_DRAW);
	content->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fCardLayout = new BCardLayout();
	content->SetLayout(fCardLayout);
	fCardLayout->AddView(_CreateLibraryView());
	fCardLayout->AddView(_CreateSourcesView());
	fCardLayout->AddView(_CreateNowPlayingView());
	fCardLayout->AddView(_CreatePlaylistsView());
	fCardLayout->AddView(_CreateSettingsView());

	BView* workspace = new BView("workspace", B_WILL_DRAW);
	workspace->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	BLayoutBuilder::Group<>(workspace, B_VERTICAL, 10.0f)
		.SetInsets(18.0f)
		.Add(fPageTitle)
		.Add(content, 1.0f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(menuBar)
		.AddGroup(B_HORIZONTAL, 0.0f, 1.0f)
			.Add(navigationScroll)
			.Add(new BSeparatorView(B_VERTICAL))
			.Add(workspace, 1.0f)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(_CreatePlayerBar());

	SetSizeLimits(820.0f, B_SIZE_UNLIMITED, 560.0f, B_SIZE_UNLIMITED);
	fNavigation->Select(0);
	_SelectPage(0);
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
		case kMessageRescan:
			_SelectPage(1);
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

	BTextControl* search = new BTextControl("search", "", "Search your library", NULL);
	search->SetExplicitMaxSize(BSize(360.0f, B_SIZE_UNSET));
	BButton* allSources = new BButton("all sources", B_TRANSLATE("All sources"), NULL);
	allSources->SetEnabled(false);

	BTabView* tabs = new BTabView("collection tabs", B_WIDTH_FROM_LABEL);
	tabs->AddTab(CreateCollectionTab(B_TRANSLATE("Artists"),
		B_TRANSLATE("Artists will appear here after your first library scan.")));
	tabs->AddTab(CreateCollectionTab(B_TRANSLATE("Albums"),
		B_TRANSLATE("Album artwork will appear here after your first library scan.")));
	tabs->AddTab(CreateCollectionTab(B_TRANSLATE("Genres"),
		B_TRANSLATE("Genres will appear here after your first library scan.")));
	tabs->AddTab(CreateCollectionTab(B_TRANSLATE("Songs"),
		B_TRANSLATE("Songs will appear here after your first library scan.")));
	tabs->AddTab(CreateCollectionTab(B_TRANSLATE("Folders"),
		B_TRANSLATE("Add a local music folder from the Library menu.")));

	BLayoutBuilder::Group<>(view, B_VERTICAL, 12.0f)
		.AddGroup(B_HORIZONTAL, 8.0f)
			.Add(search, 1.0f)
			.Add(allSources)
		.End()
		.Add(tabs, 1.0f);
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
	BButton* addFolder = new BButton("add folder", B_TRANSLATE("Add folder") B_UTF8_ELLIPSIS,
		new BMessage(kMessageAddFolder));
	addFolder->SetEnabled(false);

	BStringView* providerNote = new BStringView("provider note", B_TRANSLATE(
		"Remote providers will arrive after local library and playback support."));
	providerNote->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 8.0f)
		.SetInsets(12.0f)
		.Add(localTitle)
		.Add(localDetail)
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

	BStringView* title = new BStringView("track title", B_TRANSLATE("Not playing"));
	title->SetFont(be_bold_font);
	BStringView* artist = new BStringView("track artist", B_TRANSLATE("Sakura"));
	artist->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DISABLED_LABEL_TINT);

	BButton* previous = new BButton("previous", B_TRANSLATE("Previous"), NULL);
	BButton* play = new BButton("play", B_TRANSLATE("Play"), NULL);
	BButton* next = new BButton("next", B_TRANSLATE("Next"), NULL);
	previous->SetEnabled(false);
	play->SetEnabled(false);
	next->SetEnabled(false);

	BSlider* position = new BSlider("position", "", NULL, 0, 100, B_HORIZONTAL);
	position->SetEnabled(false);
	position->SetExplicitMinSize(BSize(180.0f, B_SIZE_UNSET));
	BSlider* volume = new BSlider("volume", B_TRANSLATE("Volume"), NULL, 0, 100,
		B_HORIZONTAL);
	volume->SetValue(80);
	volume->SetEnabled(false);
	volume->SetExplicitMaxSize(BSize(150.0f, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(bar, B_HORIZONTAL, 10.0f)
		.SetInsets(14.0f, 10.0f)
		.Add(artwork)
		.AddGroup(B_VERTICAL, 2.0f)
			.Add(title)
			.Add(artist)
		.End()
		.AddGlue()
		.Add(previous)
		.Add(play)
		.Add(next)
		.Add(position, 1.0f)
		.Add(volume);
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


void MainWindow::_SelectPage(int32 index) {
	if (index < 0 || index >= 5)
		return;

	fCardLayout->SetVisibleItem(index);
	fPageTitle->SetText(B_TRANSLATE(kPageTitles[index]));
	if (fNavigation->CurrentSelection() != index)
		fNavigation->Select(index);
}
//---------------------------------------------------------------------------------------------------------------------------------//
