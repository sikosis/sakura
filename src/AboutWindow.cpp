/*
 * Copyright 2026 Sikosis
 * Distributed under the terms of the MIT License.
 */


#include "AboutWindow.h"

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <File.h>
#include <Font.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Mime.h>
#include <Resources.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <Size.h>
#include <String.h>
#include <StringView.h>
#include <TabView.h>
#include <TranslationUtils.h>
#include <View.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AboutWindow"


class IconView : public BView {
public:
	IconView(BBitmap* icon);
	virtual ~IconView();

	virtual void Draw(BRect updateRect);

private:
	BBitmap* fIcon;
};


class LinkView : public BStringView {
public:
	LinkView(const char* name, const char* label, const char* url);

	virtual void AttachedToWindow();
	virtual void MouseDown(BPoint point);

private:
	BString fUrl;
};


static BBitmap* LoadAppIcon() {
	BBitmap* icon = BTranslationUtils::GetBitmap('PNG ', "about-icon.png");
	if (icon != NULL)
		return icon;

	const int32 iconSize = 96;
	icon = new BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1), B_RGBA32);
	if (icon->InitCheck() == B_OK) {
		BResources* resources = BApplication::AppResources();
		if (resources != NULL) {
			size_t size = 0;
			const uint8* data = static_cast<const uint8*>(
				resources->LoadResource(B_VECTOR_ICON_TYPE, "BEOS:ICON", &size));
			if (data != NULL && size > 0
				&& BIconUtils::GetVectorIcon(data, size, icon) == B_OK) {
				return icon;
			}
		}
	}
	delete icon;

	app_info appInfo;
	if (be_app->GetAppInfo(&appInfo) != B_OK)
		return NULL;

	BFile appFile(&appInfo.ref, B_READ_ONLY);
	if (appFile.InitCheck() != B_OK)
		return NULL;

	icon = new BBitmap(BRect(0, 0, 31, 31), B_RGBA32);
	if (icon->InitCheck() != B_OK) {
		delete icon;
		return NULL;
	}

	BAppFileInfo appFileInfo(&appFile);
	if (appFileInfo.GetIcon(icon, B_LARGE_ICON) != B_OK) {
		delete icon;
		return NULL;
	}

	return icon;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BString AppVersion() {
	BString version(B_TRANSLATE("Version 0.1"));
	app_info appInfo;
	if (be_app->GetAppInfo(&appInfo) != B_OK)
		return version;

	BFile appFile(&appInfo.ref, B_READ_ONLY);
	BAppFileInfo appFileInfo(&appFile);
	version_info versionInfo;
	if (appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK) {
		if (versionInfo.minor == 0) {
			version.SetToFormat(B_TRANSLATE("Version %lu.%lu"),
				(unsigned long)versionInfo.major, (unsigned long)versionInfo.middle);
		} else {
			version.SetToFormat(B_TRANSLATE("Version %lu.%lu.%lu"),
				(unsigned long)versionInfo.major, (unsigned long)versionInfo.middle,
				(unsigned long)versionInfo.minor);
		}
	}

	return version;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BStringView* Heading(const char* name, const char* text) {
	BStringView* heading = new BStringView(name, text);
	heading->SetFont(be_bold_font);
	return heading;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BView* CreateCreditsView() {
	BView* view = new BView("Credits", B_WILL_DRAW);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 5.0f)
		.SetInsets(12.0f)
		.Add(Heading("project heading", B_TRANSLATE("Project")))
		.Add(new BStringView("copyright", "Copyright 2026 The Sakura contributors"))
		.AddStrut(8.0f)
		.Add(Heading("current heading", B_TRANSLATE("Current Contributor")))
		.Add(new BStringView("current contributor", "Sikosis"))
		.AddStrut(8.0f)
		.Add(Heading("heritage heading", B_TRANSLATE("Heritage")))
		.Add(new BStringView("heritage", B_TRANSLATE(
			"Named in homage to the Cherry Blossom player and its contributors.")))
		.AddStrut(8.0f)
		.Add(Heading("thanks heading", B_TRANSLATE("Special Thanks")))
		.Add(new BStringView("haiku thanks", "The Haiku project and HaikuArchives community"))
		.AddGlue();

	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BView* CreateLicencesView() {
	BView* view = new BView("Licences", B_WILL_DRAW);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 4.0f)
		.SetInsets(12.0f)
		.Add(Heading("sakura licence heading", "Sakura"))
		.Add(new BStringView("sakura licence", "MIT License"))
		.Add(new LinkView("mit link", "MIT License", "https://opensource.org/license/mit"))
		.AddStrut(5.0f)
		.Add(Heading("media kit heading", B_TRANSLATE("Media codecs")))
		.Add(new BStringView("media kit dependency", B_TRANSLATE(
			"Playback will use installed Haiku media add-ons, commonly FFmpeg-based.")))
		.Add(new BStringView("ffmpeg licence", B_TRANSLATE(
			"FFmpeg is LGPL 2.1+ or GPL 2+ when optional GPL code is enabled.")))
		.Add(new LinkView("ffmpeg link", "FFmpeg", "https://ffmpeg.org/"))
		.AddGlue();

	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


IconView::IconView(BBitmap* icon) : BView("application icon", B_WILL_DRAW), fIcon(icon) {
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetExplicitMinSize(BSize(96, 96));
	SetExplicitMaxSize(BSize(96, 96));
}
//---------------------------------------------------------------------------------------------------------------------------------//


IconView::~IconView() {
	delete fIcon;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void IconView::Draw(BRect updateRect) {
	(void)updateRect;
	if (fIcon == NULL)
		return;

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmap(fIcon, fIcon->Bounds(), Bounds(), B_FILTER_BITMAP_BILINEAR);
}
//---------------------------------------------------------------------------------------------------------------------------------//


LinkView::LinkView(const char* name, const char* label, const char* url)
	: BStringView(name, label), fUrl(url) {
	SetToolTip(url);
	BFont font;
	GetFont(&font);
	font.SetFace(font.Face() | B_UNDERSCORE_FACE);
	SetFont(&font);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LinkView::AttachedToWindow() {
	BStringView::AttachedToWindow();
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	int32 luminance
		= (299 * background.red + 587 * background.green + 114 * background.blue) / 1000;
	if (luminance < 128)
		SetHighColor(105, 190, 255);
	else
		SetHighColor(0, 76, 168);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LinkView::MouseDown(BPoint point) {
	(void)point;
	const char* arguments[] = { fUrl.String() };
	be_roster->Launch("text/html", 1, arguments);
}
//---------------------------------------------------------------------------------------------------------------------------------//


AboutWindow::AboutWindow()
	: BWindow(BRect(0, 0, 640, 620), B_TRANSLATE("About Sakura"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE) {
	BStringView* title = new BStringView("title", "Sakura");
	BFont titleFont(*be_bold_font);
	titleFont.SetSize(be_plain_font->Size() * 2.0f);
	title->SetFont(&titleFont);

	BString versionText = AppVersion();
	BStringView* version = new BStringView("version", versionText.String());
	BStringView* description = new BStringView("description", B_TRANSLATE(
		"A native music library and player for Haiku."));

	BTabView* tabs = new BTabView("about tabs", B_WIDTH_FROM_LABEL);
	tabs->AddTab(CreateCreditsView());
	tabs->AddTab(CreateLicencesView());

	BButton* closeButton
		= new BButton("close", B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED));
	closeButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 10.0f)
		.SetInsets(18.0f)
		.AddGroup(B_HORIZONTAL, 16.0f)
			.Add(new IconView(LoadAppIcon()))
			.AddGroup(B_VERTICAL, 4.0f)
				.Add(title)
				.Add(version)
				.AddStrut(4.0f)
				.Add(description)
				.Add(new LinkView("source link", "github.com/sikosis/Sakura",
					"https://github.com/sikosis/Sakura"))
				.AddGlue()
			.End()
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(tabs, 1.0f)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.AddGlue()
			.Add(closeButton)
		.End();

	CenterOnScreen();
}
//---------------------------------------------------------------------------------------------------------------------------------//
