#ifndef SAKURA_LIBRARY_DATABASE_H
#define SAKURA_LIBRARY_DATABASE_H


#include <String.h>
#include <SupportDefs.h>

#include <vector>


struct sqlite3;


struct LibraryTrack {
	BString album;
	BString artist;
	int32 duration;
	BString genre;
	int64 modified;
	BString path;
	int64 size;
	BString sourcePath;
	BString title;
	int32 trackNumber;
};


class LibraryDatabase {
public:
	LibraryDatabase();
	~LibraryDatabase();

	status_t Open();
	status_t AddSource(const char* path);
	status_t BeginScan();
	status_t CommitScan();
	void RollbackScan();
	bool FindUnchangedTrack(const char* path, int64 size, int64 modified,
		LibraryTrack& track);
	status_t LoadSources(std::vector<BString>& sources);
	status_t LoadTracks(const char* searchText, std::vector<LibraryTrack>& tracks);
	status_t MarkTrackSeen(const char* path, const char* sourcePath, int64 scanToken);
	status_t RemoveUnseenTracks(const char* sourcePath, int64 scanToken);
	status_t StoreTrack(const LibraryTrack& track, int64 scanToken);

private:
	status_t _Execute(const char* sql);
	status_t _InitialiseSchema();

	sqlite3* fDatabase;
};


#endif
