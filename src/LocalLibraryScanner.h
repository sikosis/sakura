#ifndef SAKURA_LOCAL_LIBRARY_SCANNER_H
#define SAKURA_LOCAL_LIBRARY_SCANNER_H


#include "LibraryDatabase.h"

#include <Entry.h>
#include <Looper.h>
#include <Messenger.h>


class BDirectory;
class BEntry;


class LocalLibraryScanner : public BLooper {
public:
	LocalLibraryScanner(const BMessenger& target);

	virtual void MessageReceived(BMessage* message);
	void LoadLibrary();
	void Scan(const entry_ref& folder);
	void Search(const char* searchText, int32 requestId);

private:
	status_t _EnsureDatabase();
	bool _IsAudioFile(const BEntry& entry) const;
	void _LoadLibrary();
	bool _ReadTrack(const BEntry& entry, const char* rootPath, LibraryTrack& track);
	int32 _ScanDirectory(BDirectory& directory, const char* rootPath, int64 scanToken);
	int32 _ScanQuery(const entry_ref& folder, const char* rootPath, int64 scanToken);
	bool _ScanTrack(const BEntry& entry, const char* rootPath, int64 scanToken);
	void _Search(const char* searchText, int32 requestId);
	void _SendError(const char* message);
	void _SendTrack(const LibraryTrack& track, int32 requestId = 0);

	LibraryDatabase fDatabase;
	BMessenger fTarget;
};


#endif
