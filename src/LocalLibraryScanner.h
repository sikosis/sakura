#ifndef SAKURA_LOCAL_LIBRARY_SCANNER_H
#define SAKURA_LOCAL_LIBRARY_SCANNER_H


#include <Entry.h>
#include <Looper.h>
#include <Messenger.h>


class BDirectory;
class BEntry;


class LocalLibraryScanner : public BLooper {
public:
	LocalLibraryScanner(const BMessenger& target);

	virtual void MessageReceived(BMessage* message);
	void Scan(const entry_ref& folder);

private:
	bool _IsAudioFile(const BEntry& entry) const;
	int32 _ScanDirectory(BDirectory& directory, const char* rootPath);
	int32 _ScanQuery(const entry_ref& folder, const char* rootPath);
	void _SendTrack(const BEntry& entry, const char* rootPath);

	BMessenger fTarget;
};


#endif
