#include "TrackListItem.h"


TrackListItem::TrackListItem(const char* title, const char* artist, const char* album,
	const char* genre, const char* path, int32 duration, int32 trackNumber)
	: BStringItem(""), fAlbum(album), fArtist(artist), fDuration(duration), fGenre(genre),
		fPath(path), fTitle(title), fTrackNumber(trackNumber) {
	BString label;
	if (fTrackNumber > 0)
		label.SetToFormat("%ld. ", (long)fTrackNumber);
	label.Append(fTitle);
	if (!fArtist.IsEmpty())
		label << "  —  " << fArtist;
	if (!fAlbum.IsEmpty())
		label << "  —  " << fAlbum;
	if (fDuration > 0) {
		int32 minutes = fDuration / 60;
		int32 seconds = fDuration % 60;
		BString durationLabel;
		durationLabel.SetToFormat("  ·  %ld:%02ld", (long)minutes, (long)seconds);
		label.Append(durationLabel);
	}
	SetText(label.String());
}
//---------------------------------------------------------------------------------------------------------------------------------//


const char* TrackListItem::Album() const {
	return fAlbum.String();
}
//---------------------------------------------------------------------------------------------------------------------------------//


const char* TrackListItem::Artist() const {
	return fArtist.String();
}
//---------------------------------------------------------------------------------------------------------------------------------//


int32 TrackListItem::Duration() const {
	return fDuration;
}
//---------------------------------------------------------------------------------------------------------------------------------//


const char* TrackListItem::Genre() const {
	return fGenre.String();
}
//---------------------------------------------------------------------------------------------------------------------------------//


const char* TrackListItem::Path() const {
	return fPath.String();
}
//---------------------------------------------------------------------------------------------------------------------------------//


const char* TrackListItem::Title() const {
	return fTitle.String();
}
//---------------------------------------------------------------------------------------------------------------------------------//


int32 TrackListItem::TrackNumber() const {
	return fTrackNumber;
}
//---------------------------------------------------------------------------------------------------------------------------------//
