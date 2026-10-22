#include "LibraryDatabase.h"

#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>

#include <sqlite3.h>


static void ReadTrack(sqlite3_stmt* statement, LibraryTrack& track) {
	track.path = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
	track.sourcePath = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
	track.title = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
	track.artist = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3));
	track.album = reinterpret_cast<const char*>(sqlite3_column_text(statement, 4));
	track.genre = reinterpret_cast<const char*>(sqlite3_column_text(statement, 5));
	track.duration = sqlite3_column_int(statement, 6);
	track.trackNumber = sqlite3_column_int(statement, 7);
	track.size = sqlite3_column_int64(statement, 8);
	track.modified = sqlite3_column_int64(statement, 9);
}
//---------------------------------------------------------------------------------------------------------------------------------//


LibraryDatabase::LibraryDatabase() : fDatabase(NULL) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


LibraryDatabase::~LibraryDatabase() {
	if (fDatabase != NULL)
		sqlite3_close(fDatabase);
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::Open() {
	if (fDatabase != NULL)
		return B_OK;

	BPath settingsPath;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath, true);
	if (status != B_OK)
		return status;
	status = settingsPath.Append("Sakura");
	if (status != B_OK)
		return status;
	status = create_directory(settingsPath.Path(), 0755);
	if (status != B_OK && status != B_FILE_EXISTS)
		return status;
	status = settingsPath.Append("library.db");
	if (status != B_OK)
		return status;

	int result = sqlite3_open_v2(settingsPath.Path(), &fDatabase,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (result != SQLITE_OK) {
		if (fDatabase != NULL) {
			sqlite3_close(fDatabase);
			fDatabase = NULL;
		}
		return B_ERROR;
	}

	sqlite3_busy_timeout(fDatabase, 3000);
	if (_Execute("PRAGMA foreign_keys = ON;") != B_OK
		|| _Execute("PRAGMA journal_mode = WAL;") != B_OK
		|| _Execute("PRAGMA synchronous = NORMAL;") != B_OK
		|| _InitialiseSchema() != B_OK) {
		sqlite3_close(fDatabase);
		fDatabase = NULL;
		return B_ERROR;
	}
	return B_OK;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::AddSource(const char* path) {
	static const char* sql =
		"INSERT OR IGNORE INTO sources(path, added_at) VALUES(?1, strftime('%s', 'now'));";
	sqlite3_stmt* statement = NULL;
	if (sqlite3_prepare_v2(fDatabase, sql, -1, &statement, NULL) != SQLITE_OK)
		return B_ERROR;
	sqlite3_bind_text(statement, 1, path, -1, SQLITE_TRANSIENT);
	int result = sqlite3_step(statement);
	sqlite3_finalize(statement);
	return result == SQLITE_DONE ? B_OK : B_ERROR;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::BeginScan() {
	return _Execute("BEGIN IMMEDIATE TRANSACTION;");
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::CommitScan() {
	return _Execute("COMMIT;");
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LibraryDatabase::RollbackScan() {
	_Execute("ROLLBACK;");
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool LibraryDatabase::FindUnchangedTrack(const char* path, int64 size, int64 modified,
	LibraryTrack& track) {
	static const char* sql =
		"SELECT path, source_path, title, artist, album, genre, duration, track_number, "
		"size, modified FROM tracks WHERE path = ?1 AND size = ?2 AND modified = ?3;";
	sqlite3_stmt* statement = NULL;
	if (sqlite3_prepare_v2(fDatabase, sql, -1, &statement, NULL) != SQLITE_OK)
		return false;
	sqlite3_bind_text(statement, 1, path, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(statement, 2, size);
	sqlite3_bind_int64(statement, 3, modified);
	bool found = sqlite3_step(statement) == SQLITE_ROW;
	if (found)
		ReadTrack(statement, track);
	sqlite3_finalize(statement);
	return found;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::LoadSources(std::vector<BString>& sources) {
	static const char* sql = "SELECT path FROM sources ORDER BY path COLLATE NOCASE;";
	sqlite3_stmt* statement = NULL;
	if (sqlite3_prepare_v2(fDatabase, sql, -1, &statement, NULL) != SQLITE_OK)
		return B_ERROR;
	int result = SQLITE_ROW;
	while ((result = sqlite3_step(statement)) == SQLITE_ROW)
		sources.push_back(reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)));
	sqlite3_finalize(statement);
	return result == SQLITE_DONE ? B_OK : B_ERROR;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::LoadTracks(const char* searchText,
	std::vector<LibraryTrack>& tracks) {
	static const char* sql =
		"SELECT path, source_path, title, artist, album, genre, duration, track_number, "
		"size, modified FROM tracks WHERE ?1 = '' OR title LIKE ?2 ESCAPE '\\' "
		"OR artist LIKE ?2 ESCAPE '\\' OR album LIKE ?2 ESCAPE '\\' "
		"OR genre LIKE ?2 ESCAPE '\\' ORDER BY title COLLATE NOCASE, artist COLLATE NOCASE;";
	BString pattern("%");
	for (const char* cursor = searchText; *cursor != '\0'; cursor++) {
		if (*cursor == '%' || *cursor == '_' || *cursor == '\\')
			pattern.Append("\\");
		pattern.Append(cursor, 1);
	}
	pattern.Append("%");

	sqlite3_stmt* statement = NULL;
	if (sqlite3_prepare_v2(fDatabase, sql, -1, &statement, NULL) != SQLITE_OK)
		return B_ERROR;
	sqlite3_bind_text(statement, 1, searchText, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(statement, 2, pattern.String(), -1, SQLITE_TRANSIENT);
	int result = SQLITE_ROW;
	while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
		LibraryTrack track;
		ReadTrack(statement, track);
		tracks.push_back(track);
	}
	sqlite3_finalize(statement);
	return result == SQLITE_DONE ? B_OK : B_ERROR;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::MarkTrackSeen(const char* path, const char* sourcePath,
	int64 scanToken) {
	static const char* sql =
		"UPDATE tracks SET source_path = ?2, scan_token = ?3 WHERE path = ?1;";
	sqlite3_stmt* statement = NULL;
	if (sqlite3_prepare_v2(fDatabase, sql, -1, &statement, NULL) != SQLITE_OK)
		return B_ERROR;
	sqlite3_bind_text(statement, 1, path, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(statement, 2, sourcePath, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(statement, 3, scanToken);
	int result = sqlite3_step(statement);
	sqlite3_finalize(statement);
	return result == SQLITE_DONE ? B_OK : B_ERROR;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::RemoveUnseenTracks(const char* sourcePath, int64 scanToken) {
	static const char* sql =
		"DELETE FROM tracks WHERE source_path = ?1 AND scan_token <> ?2;";
	sqlite3_stmt* statement = NULL;
	if (sqlite3_prepare_v2(fDatabase, sql, -1, &statement, NULL) != SQLITE_OK)
		return B_ERROR;
	sqlite3_bind_text(statement, 1, sourcePath, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(statement, 2, scanToken);
	int result = sqlite3_step(statement);
	sqlite3_finalize(statement);
	return result == SQLITE_DONE ? B_OK : B_ERROR;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::StoreTrack(const LibraryTrack& track, int64 scanToken) {
	static const char* sql =
		"INSERT INTO tracks(path, source_path, title, artist, album, genre, duration, "
		"track_number, size, modified, scan_token) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, "
		"?8, ?9, ?10, ?11) ON CONFLICT(path) DO UPDATE SET source_path = excluded.source_path, "
		"title = excluded.title, artist = excluded.artist, album = excluded.album, "
		"genre = excluded.genre, duration = excluded.duration, "
		"track_number = excluded.track_number, size = excluded.size, "
		"modified = excluded.modified, scan_token = excluded.scan_token;";
	sqlite3_stmt* statement = NULL;
	if (sqlite3_prepare_v2(fDatabase, sql, -1, &statement, NULL) != SQLITE_OK)
		return B_ERROR;
	sqlite3_bind_text(statement, 1, track.path.String(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(statement, 2, track.sourcePath.String(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(statement, 3, track.title.String(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(statement, 4, track.artist.String(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(statement, 5, track.album.String(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(statement, 6, track.genre.String(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(statement, 7, track.duration);
	sqlite3_bind_int(statement, 8, track.trackNumber);
	sqlite3_bind_int64(statement, 9, track.size);
	sqlite3_bind_int64(statement, 10, track.modified);
	sqlite3_bind_int64(statement, 11, scanToken);
	int result = sqlite3_step(statement);
	sqlite3_finalize(statement);
	return result == SQLITE_DONE ? B_OK : B_ERROR;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::_Execute(const char* sql) {
	return sqlite3_exec(fDatabase, sql, NULL, NULL, NULL) == SQLITE_OK ? B_OK : B_ERROR;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t LibraryDatabase::_InitialiseSchema() {
	static const char* schema =
		"CREATE TABLE IF NOT EXISTS sources("
		"path TEXT PRIMARY KEY NOT NULL, added_at INTEGER NOT NULL);"
		"CREATE TABLE IF NOT EXISTS tracks("
		"path TEXT PRIMARY KEY NOT NULL, source_path TEXT NOT NULL, title TEXT NOT NULL, "
		"artist TEXT NOT NULL DEFAULT '', album TEXT NOT NULL DEFAULT '', "
		"genre TEXT NOT NULL DEFAULT '', duration INTEGER NOT NULL DEFAULT 0, "
		"track_number INTEGER NOT NULL DEFAULT 0, size INTEGER NOT NULL DEFAULT 0, "
		"modified INTEGER NOT NULL DEFAULT 0, scan_token INTEGER NOT NULL DEFAULT 0, "
		"FOREIGN KEY(source_path) REFERENCES sources(path) ON DELETE CASCADE);"
		"CREATE INDEX IF NOT EXISTS tracks_source_path ON tracks(source_path);"
		"CREATE INDEX IF NOT EXISTS tracks_title ON tracks(title COLLATE NOCASE);"
		"PRAGMA user_version = 1;";
	return _Execute(schema);
}
//---------------------------------------------------------------------------------------------------------------------------------//
