#include "LocalLibraryScanner.h"

#include "Messages.h"

#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <String.h>
#include <Volume.h>

#include <cstring>


LocalLibraryScanner::LocalLibraryScanner(const BMessenger& target)
	: BLooper("local library scanner", B_LOW_PRIORITY), fTarget(target) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::MessageReceived(BMessage* message) {
	if (message->what != kMessageScanFolder) {
		BLooper::MessageReceived(message);
		return;
	}

	entry_ref folderRef;
	if (message->FindRef("folder", &folderRef) != B_OK)
		return;

	BPath folderPath(&folderRef);
	BDirectory directory(&folderRef);
	if (folderPath.InitCheck() != B_OK || directory.InitCheck() != B_OK)
		return;

	BMessage started(kMessageScanStarted);
	started.AddString("folder", folderPath.Path());
	fTarget.SendMessage(&started);

	int32 count = _ScanQuery(folderRef, folderPath.Path());
	if (count <= 0)
		count = _ScanDirectory(directory, folderPath.Path());
	BMessage finished(kMessageScanFinished);
	finished.AddString("folder", folderPath.Path());
	finished.AddInt32("count", count);
	fTarget.SendMessage(&finished);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::Scan(const entry_ref& folder) {
	BMessage message(kMessageScanFolder);
	message.AddRef("folder", &folder);
	PostMessage(&message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool LocalLibraryScanner::_IsAudioFile(const BEntry& entry) const {
	BNode node(&entry);
	if (node.InitCheck() == B_OK) {
		BNodeInfo nodeInfo(&node);
		char type[B_MIME_TYPE_LENGTH];
		if (nodeInfo.GetType(type) == B_OK && std::strncmp(type, "audio/", 6) == 0)
			return true;
	}

	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return false;

	BString name(path.Leaf());
	name.ToLower();
	static const char* extensions[] = {
		".mp3", ".m4a", ".aac", ".flac", ".ogg", ".oga", ".opus", ".wav",
		".aiff", ".aif", ".alac", ".wv", ".ape"
	};
	for (size_t index = 0; index < sizeof(extensions) / sizeof(extensions[0]); index++) {
		if (name.EndsWith(extensions[index]))
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------------------------------------------------//


int32 LocalLibraryScanner::_ScanDirectory(BDirectory& directory, const char* rootPath) {
	int32 count = 0;
	BEntry entry;
	while (directory.GetNextEntry(&entry, false) == B_OK) {
		if (entry.IsDirectory()) {
			BDirectory child(&entry);
			if (child.InitCheck() == B_OK)
				count += _ScanDirectory(child, rootPath);
		} else if (entry.IsFile() && _IsAudioFile(entry)) {
			_SendTrack(entry, rootPath);
			count++;
		}
	}
	return count;
}
//---------------------------------------------------------------------------------------------------------------------------------//


int32 LocalLibraryScanner::_ScanQuery(const entry_ref& folder, const char* rootPath) {
	BVolume volume(folder.device);
	if (volume.InitCheck() != B_OK || !volume.KnowsQuery())
		return B_UNSUPPORTED;

	BQuery query;
	if (query.SetVolume(&volume) != B_OK
		|| query.PushAttr("BEOS:TYPE") != B_OK
		|| query.PushString("audio/*") != B_OK
		|| query.PushOp(B_EQ) != B_OK
		|| query.Fetch() != B_OK) {
		return B_ERROR;
	}

	BString folderPrefix(rootPath);
	if (!folderPrefix.EndsWith("/"))
		folderPrefix.Append("/");

	int32 count = 0;
	BEntry entry;
	while (query.GetNextEntry(&entry, false) == B_OK) {
		BPath path;
		if (entry.GetPath(&path) != B_OK)
			continue;
		if (std::strcmp(path.Path(), rootPath) != 0
			&& !BString(path.Path()).StartsWith(folderPrefix)) {
			continue;
		}

		_SendTrack(entry, rootPath);
		count++;
	}
	return count;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::_SendTrack(const BEntry& entry, const char* rootPath) {
	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return;

	BString title(path.Leaf());
	int32 extension = title.FindLast('.');
	if (extension > 0)
		title.Truncate(extension);

	BMessage found(kMessageTrackFound);
	found.AddString("path", path.Path());
	found.AddString("title", title.String());
	found.AddString("folder", rootPath);
	fTarget.SendMessage(&found);
}
//---------------------------------------------------------------------------------------------------------------------------------//
