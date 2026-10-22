<p align="center">
  <img src="resources/icon.png" alt="Sakura - Haiku Media Player" width="256">
</p>

<h1 align="center">Sakura</h1>

<p align="center">
  <img src="https://img.shields.io/badge/version-v0.1-f5a623" alt="Version v0.1">
  <img src="https://img.shields.io/badge/platform-Haiku-ffcc00" alt="Platform: Haiku">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599c" alt="C++17">
  <img src="https://img.shields.io/badge/licence-MIT-4c8c2b" alt="MIT licence">
</p>

Sakura (Japanese for “Cherry Blossom”) is a native C++ BApplication for Haiku that is a music library first and a player second.

The project is in its first local-library phase. It currently provides native navigation for the planned Library, Sources, Now Playing, Playlists and Settings screens, a persistent player bar, saved music folders, an incremental SQLite library, metadata search, background indexing of local audio files, TagLib metadata extraction and basic Media Kit playback. Cover artwork, seeking and queue management are the next implementation milestones.

## Building on Haiku

Sakura uses Haiku's standard Makefile Engine. From the repository root:

```sh
pkgman install sqlite_devel taglib_devel
make
```

TagLib supplies consistent title, artist, album, genre, track-number and duration metadata across the supported local audio formats. SQLite stores the indexed library under Sakura's user settings directory so folders and tracks return immediately on the next launch. The later provider phases described in [DESIGN.md](DESIGN.md) will add their dependencies as each feature becomes real.

Run the repository checks with:

```sh
./scripts/check.sh
```
