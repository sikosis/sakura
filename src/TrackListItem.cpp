#include "TrackListItem.h"


TrackListItem::TrackListItem(const char* title, const char* path)
	: BStringItem(title), fPath(path) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


const char* TrackListItem::Path() const {
	return fPath.String();
}
//---------------------------------------------------------------------------------------------------------------------------------//
