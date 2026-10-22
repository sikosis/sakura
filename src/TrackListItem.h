#ifndef SAKURA_TRACK_LIST_ITEM_H
#define SAKURA_TRACK_LIST_ITEM_H


#include <String.h>
#include <StringItem.h>


class TrackListItem : public BStringItem {
public:
	TrackListItem(const char* title, const char* artist, const char* album,
		const char* genre, const char* path, int32 duration, int32 trackNumber);

	const char* Album() const;
	const char* Artist() const;
	int32 Duration() const;
	const char* Genre() const;
	const char* Path() const;
	const char* Title() const;
	int32 TrackNumber() const;

private:
	BString fAlbum;
	BString fArtist;
	int32 fDuration;
	BString fGenre;
	BString fPath;
	BString fTitle;
	int32 fTrackNumber;
};


#endif
