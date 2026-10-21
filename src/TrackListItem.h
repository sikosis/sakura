#ifndef SAKURA_TRACK_LIST_ITEM_H
#define SAKURA_TRACK_LIST_ITEM_H


#include <String.h>
#include <StringItem.h>


class TrackListItem : public BStringItem {
public:
	TrackListItem(const char* title, const char* path);

	const char* Path() const;

private:
	BString fPath;
};


#endif
