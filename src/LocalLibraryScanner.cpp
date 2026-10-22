#include "LocalLibraryScanner.h"

#include "Messages.h"

#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Message.h>
#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>
#include <OS.h>
#include <Path.h>
#include <Query.h>
#include <String.h>
#include <Volume.h>

#include <audioproperties.h>
#include <fileref.h>
#include <tag.h>

#include <cstring>
#include <sys/stat.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LocalLibraryScanner"


static BString ConvertTagString(const TagLib::String& value) {
	if (value.isEmpty())
		return BString();
	return BString(value.to8Bit(true).c_str());
}
//---------------------------------------------------------------------------------------------------------------------------------//


static int32 ReadMediaDuration(const entry_ref& ref) {
	BMediaFile mediaFile(&ref);
	if (mediaFile.InitCheck() != B_OK)
		return 0;

	for (int32 index = 0; index < mediaFile.CountTracks(); index++) {
		BMediaTrack* track = mediaFile.TrackAt(index);
		if (track == NULL)
			continue;

		media_format format;
		status_t status = track->EncodedFormat(&format);
		if (status == B_OK
			&& (format.type == B_MEDIA_ENCODED_AUDIO || format.type == B_MEDIA_RAW_AUDIO)) {
			bigtime_t duration = track->Duration();
			mediaFile.ReleaseTrack(track);
			return duration > 0 ? (int32)(duration / 1000000) : 0;
		}
		mediaFile.ReleaseTrack(track);
	}

	return 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//


LocalLibraryScanner::LocalLibraryScanner(const BMessenger& target)
	: BLooper("local library scanner", B_LOW_PRIORITY), fTarget(target) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::MessageReceived(BMessage* message) {
	switch (message->what) {
		case kMessageLoadLibrary:
			_LoadLibrary();
			break;

		case kMessageSearchLibrary:
		{
			const char* searchText = "";
			int32 requestId = 0;
			message->FindString("query", &searchText);
			message->FindInt32("request_id", &requestId);
			_Search(searchText, requestId);
			break;
		}

		case kMessageScanFolder:
		{
			entry_ref folderRef;
			if (message->FindRef("folder", &folderRef) != B_OK)
				break;

			BPath folderPath(&folderRef);
			BDirectory directory(&folderRef);
			if (folderPath.InitCheck() != B_OK || directory.InitCheck() != B_OK)
				break;
			if (_EnsureDatabase() != B_OK) {
				_SendError(B_TRANSLATE("The library database could not be opened."));
				break;
			}
			if (fDatabase.AddSource(folderPath.Path()) != B_OK
				|| fDatabase.BeginScan() != B_OK) {
				_SendError(B_TRANSLATE("The library scan could not be started."));
				break;
			}

			BMessage started(kMessageScanStarted);
			started.AddString("folder", folderPath.Path());
			fTarget.SendMessage(&started);

			int64 scanToken = real_time_clock_usecs();
			int32 count = _ScanQuery(folderRef, folderPath.Path(), scanToken);
			if (count <= 0)
				count = _ScanDirectory(directory, folderPath.Path(), scanToken);
			if (fDatabase.RemoveUnseenTracks(folderPath.Path(), scanToken) != B_OK
				|| fDatabase.CommitScan() != B_OK) {
				fDatabase.RollbackScan();
				_SendError(B_TRANSLATE("The library scan could not be saved."));
				break;
			}

			BMessage finished(kMessageScanFinished);
			finished.AddString("folder", folderPath.Path());
			finished.AddInt32("count", count);
			fTarget.SendMessage(&finished);
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::LoadLibrary() {
	PostMessage(kMessageLoadLibrary);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::Scan(const entry_ref& folder) {
	BMessage message(kMessageScanFolder);
	message.AddRef("folder", &folder);
	PostMessage(&message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::Search(const char* searchText, int32 requestId) {
	BMessage message(kMessageSearchLibrary);
	message.AddString("query", searchText);
	message.AddInt32("request_id", requestId);
	PostMessage(&message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LocalLibraryScanner::_EnsureDatabase() {
	return fDatabase.Open();
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


void LocalLibraryScanner::_LoadLibrary() {
	if (_EnsureDatabase() != B_OK) {
		_SendError(B_TRANSLATE("The library database could not be opened."));
		return;
	}

	std::vector<BString> sources;
	if (fDatabase.LoadSources(sources) != B_OK) {
		_SendError(B_TRANSLATE("Saved music folders could not be loaded."));
		return;
	}
	for (size_t index = 0; index < sources.size(); index++) {
		BMessage source(kMessageSourceFound);
		source.AddString("path", sources[index].String());
		fTarget.SendMessage(&source);
	}

	std::vector<LibraryTrack> tracks;
	if (fDatabase.LoadTracks("", tracks) != B_OK) {
		_SendError(B_TRANSLATE("Saved library tracks could not be loaded."));
		return;
	}
	for (size_t index = 0; index < tracks.size(); index++)
		_SendTrack(tracks[index]);

	BMessage loaded(kMessageLibraryLoaded);
	loaded.AddInt32("count", (int32)tracks.size());
	loaded.AddInt32("source_count", (int32)sources.size());
	fTarget.SendMessage(&loaded);
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool LocalLibraryScanner::_ReadTrack(const BEntry& entry, const char* rootPath,
	LibraryTrack& track) {
	BPath path;
	struct stat fileInfo;
	if (entry.GetPath(&path) != B_OK || entry.GetStat(&fileInfo) != B_OK)
		return false;

	track.path = path.Path();
	track.sourcePath = rootPath;
	track.size = fileInfo.st_size;
	track.modified = fileInfo.st_mtime;
	track.title = path.Leaf();
	int32 extension = track.title.FindLast('.');
	if (extension > 0)
		track.title.Truncate(extension);
	track.duration = 0;
	track.trackNumber = 0;

	TagLib::FileRef file(path.Path(), true, TagLib::AudioProperties::Fast);
	if (!file.isNull()) {
		TagLib::Tag* tag = file.tag();
		if (tag != NULL) {
			BString taggedTitle = ConvertTagString(tag->title());
			if (!taggedTitle.IsEmpty())
				track.title = taggedTitle;
			track.artist = ConvertTagString(tag->artist());
			track.album = ConvertTagString(tag->album());
			track.genre = ConvertTagString(tag->genre());
			track.trackNumber = (int32)tag->track();
		}

		TagLib::AudioProperties* properties = file.audioProperties();
		if (properties != NULL)
			track.duration = properties->lengthInSeconds();
	}

	if (track.duration <= 0) {
		entry_ref ref;
		if (entry.GetRef(&ref) == B_OK)
			track.duration = ReadMediaDuration(ref);
	}
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//


int32 LocalLibraryScanner::_ScanDirectory(BDirectory& directory, const char* rootPath,
	int64 scanToken) {
	int32 count = 0;
	BEntry entry;
	while (directory.GetNextEntry(&entry, false) == B_OK) {
		if (entry.IsDirectory()) {
			BDirectory child(&entry);
			if (child.InitCheck() == B_OK)
				count += _ScanDirectory(child, rootPath, scanToken);
		} else if (entry.IsFile() && _IsAudioFile(entry)) {
			if (_ScanTrack(entry, rootPath, scanToken))
				count++;
		}
	}
	return count;
}
//---------------------------------------------------------------------------------------------------------------------------------//


int32 LocalLibraryScanner::_ScanQuery(const entry_ref& folder, const char* rootPath,
	int64 scanToken) {
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

		if (_ScanTrack(entry, rootPath, scanToken))
			count++;
	}
	return count;
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool LocalLibraryScanner::_ScanTrack(const BEntry& entry, const char* rootPath,
	int64 scanToken) {
	BPath path;
	struct stat fileInfo;
	if (entry.GetPath(&path) != B_OK || entry.GetStat(&fileInfo) != B_OK)
		return false;

	LibraryTrack track;
	if (fDatabase.FindUnchangedTrack(path.Path(), fileInfo.st_size, fileInfo.st_mtime,
		track)) {
		track.sourcePath = rootPath;
		if (fDatabase.MarkTrackSeen(path.Path(), rootPath, scanToken) != B_OK)
			return false;
	} else {
		if (!_ReadTrack(entry, rootPath, track)
			|| fDatabase.StoreTrack(track, scanToken) != B_OK) {
			return false;
		}
	}

	_SendTrack(track);
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::_Search(const char* searchText, int32 requestId) {
	if (_EnsureDatabase() != B_OK) {
		_SendError(B_TRANSLATE("The library database could not be opened."));
		return;
	}

	std::vector<LibraryTrack> tracks;
	if (fDatabase.LoadTracks(searchText, tracks) != B_OK) {
		_SendError(B_TRANSLATE("The library search could not be completed."));
		return;
	}
	for (size_t index = 0; index < tracks.size(); index++)
		_SendTrack(tracks[index], requestId);

	BMessage finished(kMessageSearchFinished);
	finished.AddInt32("request_id", requestId);
	finished.AddInt32("count", (int32)tracks.size());
	fTarget.SendMessage(&finished);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::_SendError(const char* message) {
	BMessage error(kMessageLibraryError);
	error.AddString("message", message);
	fTarget.SendMessage(&error);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LocalLibraryScanner::_SendTrack(const LibraryTrack& track, int32 requestId) {
	BMessage found(kMessageTrackFound);
	found.AddString("path", track.path.String());
	found.AddString("title", track.title.String());
	found.AddString("artist", track.artist.String());
	found.AddString("album", track.album.String());
	found.AddString("genre", track.genre.String());
	found.AddInt32("duration", track.duration);
	found.AddInt32("track_number", track.trackNumber);
	found.AddString("folder", track.sourcePath.String());
	found.AddInt32("request_id", requestId);
	fTarget.SendMessage(&found);
}
//---------------------------------------------------------------------------------------------------------------------------------//
