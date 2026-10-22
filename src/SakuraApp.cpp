#include "SakuraApp.h"

#include "AboutWindow.h"
#include "MainWindow.h"

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <MimeType.h>
#include <TranslationUtils.h>
#include <View.h>


static BBitmap* CreateApplicationIcon(const BBitmap* source, int32 size) {
	BRect bounds(0, 0, size - 1, size - 1);
	BBitmap* icon = new BBitmap(bounds, B_CMAP8, true);
	if (icon->InitCheck() != B_OK) {
		delete icon;
		return NULL;
	}

	BView* canvas = new BView(bounds, "icon canvas", B_FOLLOW_NONE, B_WILL_DRAW);
	icon->AddChild(canvas);

	if (icon->Lock()) {
		canvas->SetHighColor(B_TRANSPARENT_COLOR);
		canvas->FillRect(bounds);
		canvas->SetDrawingMode(B_OP_ALPHA);
		canvas->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		canvas->DrawBitmap(source, source->Bounds(), bounds, B_FILTER_BITMAP_BILINEAR);
		canvas->Sync();
		icon->Unlock();
	}

	return icon;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static void RegisterApplicationIcon() {
	BBitmap* source = BTranslationUtils::GetBitmap('PNG ', "about-icon.png");
	if (source == NULL)
		return;

	BBitmap* miniIcon = CreateApplicationIcon(source, 16);
	BBitmap* largeIcon = CreateApplicationIcon(source, 32);
	delete source;

	BMimeType applicationType(kAppSignature);
	if (!applicationType.IsInstalled())
		applicationType.Install();
	applicationType.SetShortDescription("Sakura");
	if (miniIcon != NULL)
		applicationType.SetIcon(miniIcon, B_MINI_ICON);
	if (largeIcon != NULL)
		applicationType.SetIcon(largeIcon, B_LARGE_ICON);

	delete miniIcon;
	delete largeIcon;
}
//---------------------------------------------------------------------------------------------------------------------------------//


SakuraApp::SakuraApp() : BApplication(kAppSignature), fMainWindow(NULL) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SakuraApp::AboutRequested() {
	(new AboutWindow())->Show();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SakuraApp::ReadyToRun() {
	RegisterApplicationIcon();
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
