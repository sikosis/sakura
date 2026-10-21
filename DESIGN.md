# Haiku Music Library & Player: Design Document

Oct 21, 2026 · @Sikosis

A native C++ BApplication for Haiku that is a music library first and a player second, with a Dopamine-inspired look and a pluggable source layer. The working title is "Sakura".

## Overview

The app should feel like a first-class Haiku citizen while inspired by Dopamine's calm, cover-art-forward design. Local files and the Haiku Media Kit come first; every remote source (Plex, Jellyfin, Subsonic, Google Drive and so on) is added later behind one provider interface, so the UI and player never know where a track came from.

**Goals**

- Native C++17 with the Haiku API (Interface, Media, Storage, Network, Locale Kits) wherever it is adequate.
- A polished, Dopamine-like UI: collection browsing, cover-art grid, now-playing view, mini player, light/dark themes, accent colour.
- One unified library model so local files and remote items browse, search and queue identically.
- Gapless playback, ReplayGain, queue and playlist management, keyboard-first control.
- Provider plug-ins as Haiku add-ons or statically linked modules, so each of the 16 sources can ship independently.

**Non-goals (for now)**

- Video playback, podcasts, and music-store features.
- Cross-platform builds. Portability is welcome but never at the cost of native behaviour.
- Writing tags back to remote sources.

**Target platform.** Haiku R1/beta6, released on 26 August 2026. Packaged as an `.hpkg` and distributed through umami or source on GitHub.

## Technology decisions

The recommendation is native Haiku APIs for UI, playback, storage and messaging, plus three well-established libraries (SQLite, TagLib, libcurl) from HaikuPorts. Items marked *verify* are from memory of the Haiku API and need a check against the beta6 headers before Phase 0 ends.

| Concern | Recommended | Alternatives considered | Why |
| --- | --- | --- | --- |
| UI toolkit | Interface Kit (BWindow, BView, BLayout) with custom-drawn views | Qt 6 from HaikuPorts | A Dopamine look needs custom drawing either way; native keeps Haiku theming, Tracker drag-and-drop, Deskbar and low footprint |
| Audio decode | Media Kit: BMediaFile + BMediaTrack | libavformat/libavcodec directly; libmpv; miniaudio | Media Kit is the platform's path and Haiku's FFmpeg plug-in already backs it ([source](https://git.haiku-os.org/haiku.git/tree/src/add-ons/media/plugins/ffmpeg/FFmpegPlugin.cpp)); keep a decoder interface so FFmpeg-direct can be swapped in |
| Audio output | BSoundPlayer into the system mixer | Direct BMediaRoster node graph; SDL audio | BSoundPlayer gives a pull callback, which suits gapless and crossfade mixing; a direct hardware-node connection is a later "bit-perfect" option |
| Remote byte access | Custom BPositionIO subclass doing ranged, cached reads | Download whole file first | BMediaFile accepts a BPositionIO, so every provider can stream through the same decoder (*verify* the FFmpeg reader seeks correctly on it) |
| Tags and cover art | TagLib (LGPL) | BMediaFile metadata / BMetaData (*verify* availability); FFmpeg | Uniform reading for MP3, FLAC, Ogg, M4A, Opus, and embedded art |
| Library database | SQLite with FTS5 | BFS attributes plus BQuery | One schema must span local and remote items; BFS attributes cannot hold remote tracks. BFS can still be mirrored later for Tracker integration |
| HTTP client | Haiku Network Kit (BUrlRequest, newer BHttpSession *verify*) behind our own wrapper | libcurl | WebDAV needs PROPFIND and Range requests; the wrapper lets us pick the backend per platform without touching providers |
| TLS | BSecureSocket (OpenSSL) | Direct OpenSSL | Comes with the Network Kit |
| Credentials | BKeyStore (Keystore Kit) | Encrypted file in settings | System keystore avoids rolling our own secret storage |
| JSON / XML | BJson (*verify* header visibility) and a small XML parser such as expat or pugixml | nlohmann/json, simdjson | Plex, Subsonic and WebDAV return XML; most others return JSON |
| Concurrency | BLooper/BMessage per subsystem | std::thread and futures | Matches Haiku idioms and gives free cross-thread messaging into the UI |

**Licensing.** Dopamine is GPL-3.0 ([repository](https://github.com/digimezzo/dopamine)), so we take design inspiration only, copy no code, and do not reuse its logo or icons. Our own code should be MIT, which is the Haiku convention. That rules out GPL libraries such as Samba's libsmbclient; prefer LGPL libsmb2 for SMB if a HaikuPorts recipe exists, otherwise write the port ourselves.

**Alternatives worth knowing about.** Qt 6 would give faster UI iteration and a ready model/view framework, but it brings a large dependency, non-native widgets and a non-BMessage event model. A hybrid (headless C++ core library plus a thin Haiku UI) is the safest hedge: it keeps the provider, library and playback code free of Interface Kit types so a different front end remains possible.

## Architecture

The app is five layers with one rule: nothing below the user interface knows that Interface Kit exists. That keeps the core testable from the command line and makes a second front end (or a headless daemon) possible later.

&#91;embedded content: architecture · 5 layers\]

The user interface talks to the core by BMessage. The core owns playback, the library database and the source registry, and reaches every source through the provider interface. Providers share the HTTP, auth, cache and tag services, and only those services touch the Network, Storage and Keystore Kits. The playback engine also calls the Media Kit directly for decoding and output.

**Threads**

- Main thread: windows and views only; no blocking I/O.
- Playback: one real-time BSoundPlayer callback fed by one decode thread per active track.
- Library: a single writer thread for SQLite, with read-only connections for UI queries.
- Providers: one BLooper per source, so a slow server never stalls another source.

**Source layout.** `src/ui`, `src/core`, `src/providers/<name>`, `src/services` and `src/platform`, built with Jam or Make using Haiku's standard package layout, producing an `.hpkg` through a HaikuPorts-style recipe.

## UI and UX design

The interface keeps Dopamine's idea of a quiet, cover-art-led player, rebuilt from BViews and tuned to Haiku conventions. The cues below are my reading of Dopamine's general design and should be checked against its [screenshots](https://github.com/digimezzo/dopamine) before the visual spec is frozen.

**Design cues taken from Dopamine**

- Collection tabs for Artists, Albums, Genres, Songs, Playlists and Folders, with the content area changing beneath them.
- A cover-art grid for albums, large type, generous spacing, few borders.
- A persistent playback bar with cover, title, artist, transport, seek bar and volume.
- A full-window Now Playing view with large artwork and the queue.
- A compact mini-player mode.
- Light and dark themes with a single accent colour.

**Haiku-specific choices**

- Colours come from `ui_color()` so the app follows the user's system palette and dark settings; the accent defaults to the system control-highlight colour with an optional override (or one derived from the current cover).
- Lists are custom virtualized views (the public Interface Kit has BListView and BOutlineListView but no multi-column list); only visible rows and thumbnails are drawn, so 100,000-track libraries stay responsive.
- Windows use the BLayout API; all strings go through the Locale Kit catalogs from day one.
- Native integrations: Tracker drag-and-drop to queue, a handler for the `audio` supertype so double-clicking a file in Tracker opens it, a Deskbar replicant or tray control for transport, and a BApplication scripting suite so the player can be driven from the `hey` command-line tool.
- Cover thumbnails are generated at two sizes (about 256 px for grids, 512 px for Now Playing) and cached under the app's settings directory, never recomputed on scroll.
- Own icons are drawn in HVIF with Icon-O-Matic; Dopamine's artwork is not reused.

**Core screens**

| Screen | Purpose | Notes |
| --- | --- | --- |
| Library | Browse collection by artist, album, genre, song | Source filter chips let the user show all sources or one |
| Sources | Add, authenticate and scan a provider | Per-source status, last sync, error text |
| Now Playing | Large art, queue, upcoming tracks | Drag to reorder; save queue as playlist |
| Playlists | Local playlists, mixed across sources | Stored in SQLite; M3U8 import/export |
| Search | Instant search across all sources | FTS5-backed, results grouped by type |
| Mini player | Small always-on-top window | Art, title, transport only |
| Settings | Audio, theme, scanning, accounts, cache | Per-provider pages added by each provider |

## Playback engine

Playback is a pull pipeline that ends in a BSoundPlayer callback, with our own mixing stage in front of it so gapless, crossfade and ReplayGain work identically for every source. The Media Kit supplies the decoders; the Media Kit documentation lists BMediaFile, BMediaTrack and BSoundPlayer as the relevant classes ([Be Book index](https://datatime.sourceforge.net/Be%20Book/Release%20Notes/MediaKit.html)).

1. A provider hands the engine a `BPositionIO` for the track (a local BFile, or a ranged-HTTP stream with read-ahead cache).
2. `BMediaFile` opens it, and the first audio `BMediaTrack` is selected and decoded to raw audio.
3. A decode thread fills a lock-free ring buffer of float32 PCM (a few seconds deep), applying ReplayGain and resampling to the output format.
4. The `BSoundPlayer` callback, which runs at real-time priority, only copies from the ring buffer and never blocks or allocates.
5. The system mixer handles final routing, volume and device selection.

**Key behaviours**

- **Gapless.** The next track is opened and decoded ahead of time, and its first samples are appended to the same ring buffer with no silence. MP3 encoder delay and padding must be trimmed from the LAME/Xing header; Haiku's FFmpeg reader may already do this (*verify*); if not, the engine trims it itself.
- **Crossfade.** Optional, done in the mix stage by overlapping the tail of one track with the head of the next.
- **Seeking.** `BMediaTrack` seek calls for local files; for remote files a new ranged request at the seek offset, with the UI showing a buffering state.
- **ReplayGain.** Read from tags (TagLib) or taken from the server when it supplies it, with track and album modes and a pre-amp.
- **Server transcoding fallback.** If a format cannot be decoded locally, providers that support it (Subsonic-family, Jellyfin, Emby, Plex) can be asked for an MP3 or Opus stream instead.
- **State machine.** Stopped, Buffering, Playing, Paused, driven by BMessages from the UI; queue, shuffle and repeat live in a separate Queue class that the engine only reads from.

**Format risk.** What plays depends on Haiku's FFmpeg plug-in build, not on our code. An old ticket reported that MediaPlayer failed on Opus and VP9 ([#12696](https://dev.haiku-os.org/ticket/12696)); it is closed and years old, so treat it only as a reminder to test. The first spike must open real samples of MP3, AAC/M4A, FLAC, Ogg Vorbis, Opus, WAV, AIFF, ALAC, WavPack and APE on beta6, and record the results in a support matrix.

**Later additions.** Parametric equaliser (biquad filters in the mix stage), a direct hardware-node connection for bit-perfect output, and scrobbling to ListenBrainz or Last.fm.

## Library and data model

One SQLite database holds every item from every source, keyed by `(source_id, remote_id)`, so the UI queries a single schema and never branches on provider type. The database lives in the app's settings directory and is treated as a rebuildable cache: deleting it only costs a rescan.

| Table | Purpose | Key columns |
| --- | --- | --- |
| sources | One row per configured provider instance | id, kind, display name, base URI, credential reference, sync cursor, last sync |
| artists | Artist entities | id, source\_id, remote\_id, name, sort name, MusicBrainz id |
| albums | Album entities | id, source\_id, remote\_id, title, album artist, year, art key |
| tracks | Playable items | id, source\_id, remote\_id, album\_id, title, artist, track and disc number, genre, duration, codec, bitrate, sample rate, ReplayGain, size, modified time, URI or path |
| playlists and playlist\_items | User playlists across sources | id, name, position, track id |
| plays | Local history for counts and ratings | track id, played at, completed |
| art\_cache | Thumbnail index | art key, size, file path, last used |
| tracks\_fts | FTS5 virtual table | title, artist, album, genre |

**Local scanning** uses Haiku's strengths rather than a blind directory crawl:

- An initial BQuery on the audio MIME type finds every audio file on the chosen volumes without walking directories, and is far cheaper than recursion on large drives.
- Node monitoring (BLooper plus `watch_node`) keeps the library live when files are added, renamed or deleted in Tracker.
- Re-reading is incremental: a file is re-tagged only when its size or modified time changed.
- Tags and embedded art are read with TagLib; folder images (`cover`, `folder`, `front`) are the fallback, decoded through the Translation Kit.

**Remote sync** is the provider's job: it returns batches of normalised artist, album and track records plus a resume cursor, and the library layer upserts them in a transaction. Providers with change feeds use them for incremental updates; the rest fall back to a cheap re-list on demand.

**Identity and merging.** The same album on two sources is shown twice by default, each with a small source badge. A setting can merge albums that share a MusicBrainz id or a normalised artist and title key; the merge is a view-level grouping, so no source data is altered.

**Offline cache.** Remote tracks can be kept in a size-capped LRU cache on disk, either automatically after playing or by explicit pin.

## Providers

All 16 requested sources fit behind one `MediaProvider` interface, but they split into two families that behave very differently. **Catalogue providers** (Plex, Emby, Jellyfin, Navidrome, Subsonic, OpenSubsonic, Kodi, Audiobookshelf) already know artists, albums and tracks, so we ask them. **File providers** (local, SMB, WebDAV, Google Drive, OneDrive, Dropbox, Box, pCloud) only expose folders and files, so we must read tags ourselves over ranged requests.

```cpp
class MediaProvider {
public:
    virtual uint32     Capabilities() const = 0;   // catalogue, search, transcode, changes, art
    virtual status_t   Connect(const SourceConfig&, AuthUI&) = 0;
    virtual status_t   Sync(SyncSink&, SyncCursor* resume) = 0;   // batches of artists/albums/tracks
    virtual BPositionIO* OpenStream(const TrackRef&, const StreamOptions&) = 0;
    virtual status_t   FetchArt(const ArtRef&, BDataIO* out) = 0;
    virtual status_t   Report(const TrackRef&, PlayEvent) { return B_UNSUPPORTED; }  // scrobble / progress
};
```

The providers, in the order they are suggested for implementation:

| Provider | Family | Protocol and auth | Notes for the design | Phase |
| --- | --- | --- | --- | --- |
| Local files | File | BFS, BQuery, node monitor | Reference implementation; defines the scan pipeline | 1 |
| Subsonic | Catalogue | REST; token auth sends MD5(password + salt) and the salt ([client source](https://docs.rs/crate/opensubsonic/0.2.0/source/src/auth.rs)); API version 1.16.1 | Simplest catalogue API; best first remote target | 3 |
| OpenSubsonic | Catalogue | Same as Subsonic plus extensions: API-key auth and a `getOpenSubsonicExtensions` endpoint ([OpenSubsonic notes](https://github.com/nc-music/oc-music/wiki/OpenSubsonic-API)) | Same code as Subsonic, enabling features by detecting extensions | 3 |
| Navidrome | Catalogue | Speaks the Subsonic/OpenSubsonic API ([client README](https://docs.rs/crate/opensubsonic/0.1.0)) | No separate provider needed at first; its native API can add richer data later | 3 |
| Jellyfin | Catalogue | REST; `MediaBrowser` Authorization header with a token from `/Users/AuthenticateByName`; `/Audio/{id}/universal` streams with optional transcode; images at `/Items/{id}/Images/{type}` ([overview](https://jmshrv.com/posts/jellyfin-api/)) | Quick Connect login is a good fit for a desktop app | 4 |
| Emby | Catalogue | Jellyfin's ancestor; API is similar (*verify* differences) | Share a base class with Jellyfin, split where they diverge | 4 |
| Plex | Catalogue | plex.tv PIN login: create a PIN with a client identifier, user enters it at plex.tv/link, app polls for the token; PINs expire after about 15 minutes ([Plex forum](https://forums.plex.tv/t/authenticating-with-plex/609370)); music items are artist, album, track types | Needs server discovery (local and remote connections); XML or JSON | 4 |
| Audiobookshelf | Catalogue | REST with the user's API token as a Bearer token; `/api/authorize` validates it; progress is stored server-side ([API docs](https://api.audiobookshelf.org)) | Adds an audiobook item type: chapters, resume position, playback speed, progress sync | 5 |
| Kodi | Catalogue | JSON-RPC over HTTP; the user must enable HTTP control; `AudioLibrary.GetSongs` and `Files.GetDirectory` exist ([Kodi wiki](https://kodi.wiki/view/JSON-RPC_API/v13)) | Treat Kodi as a library to browse, not a remote to control; streaming URLs come from the Files namespace | 5 |
| WebDAV | File | HTTP with PROPFIND and Range; Basic, Digest or token auth | Needs libcurl-style custom verbs; the generic remote-file baseline for Nextcloud and others | 5 |
| SMB | File | SMB2/3 client library, not a mounted filesystem | Prefer LGPL libsmb2 over GPL Samba (see Licensing); verify a HaikuPorts recipe exists | 6 |
| Dropbox | File | OAuth 2.0 with PKCE, so no client secret is needed; path-based API with cursor-based change listing ([overview](https://github.com/ciberkids/cloud-drive-sync/wiki/Home/c51a3c9d5616732660f93cb06329bdcb378affc7)); temporary download links | Safest cloud drive to build first | 6 |
| OneDrive | File | Microsoft Graph; Azure AD browser or device-code login; delta queries for changes ([overview](https://github.com/ciberkids/cloud-drive-sync/wiki/Home/c51a3c9d5616732660f93cb06329bdcb378affc7)) | Needs an Azure app registration; delta sync suits incremental scans | 6 |
| Google Drive | File | OAuth 2.0 for installed apps, loopback redirect ([Google docs mirror](https://googleapis.github.io/google-api-python-client/docs/oauth-installed.html)); Drive API v3 | Read-only Drive scopes are likely "restricted" and need Google app verification (*verify*); bring-your-own client ID is the fallback | 7 |
| Box | File | OAuth 2.0 app; one downstream project notes Box needs the app's client secret ([notes](https://github.com/degoya/rDownloader/wiki/hosters-and-accounts)) | A secret cannot ship safely in an open-source binary, so use bring-your-own credentials | 7 |
| pCloud | File | OAuth 2.0 app; `getfilelink` returns download hosts and a path ([pCloud docs](https://docs.pcloud.com/methods/streaming/getfilelink.html)); links expire after a few hours and no refresh token is issued ([notes](https://github.com/degoya/rDownloader/wiki/hosters-and-accounts)) | Regional API hosts (US and EU); re-fetch the link on every play or resume | 7 |

**File-provider scanning.** Reading tags over the network is slow and often rate-limited, so file providers start in folder-browse mode and index a folder only when the user asks. A tag read fetches the head of the file (ID3v2, FLAC blocks) and, where the format needs it, the tail (ID3v1, and MP4 files whose index sits at the end). Results are cached by file ID and modified time or content hash.

**Streaming URL expiry.** Several services hand out short-lived download links. The `BPositionIO` stream wrapper must be able to re-resolve its URL mid-playback and resume at the current byte offset without audible interruption.

**Two-way features.** Where a server supports it, play counts, ratings, favourites and audiobook progress are reported back through `Report()`. This is optional per provider and off by default.

## Authentication, security and networking

A single `AuthManager` and a single `HttpClient` serve every provider, so credentials, TLS decisions and retry behaviour are implemented once.

**Credential storage.** Passwords, API keys and OAuth tokens go into BKeyStore, one entry per source, and only a reference is stored in the database. Nothing secret is written to settings files or logs. Subsonic-style URLs carry the token and salt in the query string, so request URLs are never logged in full.

**Login flows by provider type**

| Flow | Used by | How it works on Haiku |
| --- | --- | --- |
| Username and password, or API key | Subsonic family, Jellyfin, Emby, Audiobookshelf, Kodi, WebDAV, SMB | Entered in the Sources screen, stored in BKeyStore |
| PIN link | Plex | App shows a code, user enters it at plex.tv/link in a browser, app polls until a token arrives |
| OAuth with PKCE and loopback redirect | Dropbox, OneDrive, Google Drive | App opens the system browser, listens on `127.0.0.1` on a free port, exchanges the code with a PKCE verifier |
| OAuth with a client secret | Box, pCloud | Bring-your-own app credentials entered in Settings, since a secret cannot be hidden in an open-source binary |
| Device code | OneDrive (optional) | For setups where a local listener is awkward |

**Token handling.** The `AuthManager` refreshes access tokens ahead of expiry and serialises refreshes per source so two requests cannot race. pCloud issues no refresh token, so an expired session sends the user back through login with a clear message.

**TLS.** Certificate checks use the system store through the Network Kit. Self-hosted servers often use self-signed certificates, so a source may opt in to a pinned fingerprint after the user confirms it; the pin is stored with the source and any later change raises a warning.

**HTTP behaviour.** One wrapper provides connection reuse, timeouts, exponential backoff with jitter, respect for `Retry-After` on rate limits, Range requests and gzip. Provider code never touches sockets.

**Caches.**

- Metadata and the library: SQLite.
- Artwork: thumbnails on disk with an index table and an LRU cap.
- Audio: optional LRU cache of whole tracks with a user-set size limit and per-track pinning.

**Network loss.** Playback keeps draining the ring buffer, then shows a buffering state and retries; a source that fails repeatedly is marked offline in the Sources screen without blocking the rest of the library.

**Privacy.** The app has no telemetry. It talks only to the sources the user configures and, for artwork lookups, to nothing unless a metadata service is explicitly enabled later.

## Roadmap

Build order is local playback first, then the shared provider framework, then providers from simplest to hardest. No dates are set; each phase ends at a gate that must pass before the next starts.

&#91;embedded content: roadmap · 8 phases with exit gates\]

Phase 0 exists to retire the two biggest technical unknowns, format support and streamed seeking, before any UI work depends on them.

## Risks and open questions

The largest risks are platform facts that only a spike can settle: what Haiku's decoders play and whether a streamed source seeks cleanly. Both are scheduled first.

| Risk | Effect | Mitigation |
| --- | --- | --- |
| Haiku's FFmpeg plug-in lacks or mishandles a format (Opus, ALAC, WavPack, APE) | Some tracks will not play | Phase 0 format matrix; keep a decoder interface so FFmpeg can be called directly; use server transcoding for remote sources |
| Media Kit does not trim MP3 encoder delay | Audible gaps between tracks | Trim from the LAME/Xing header in the engine; build a gapless test corpus |
| BMediaFile seeks badly over a custom BPositionIO | Remote seeking fails or stalls | Phase 0 spike; fall back to caching the file and playing the local copy |
| No usable SMB client library under a compatible licence | SMB source delayed | Port libsmb2; otherwise defer SMB and rely on the local provider over any mounted path |
| Google Drive restricted-scope verification, and client secrets for Box and pCloud | Cloud providers unusable in a default build | Bring-your-own credentials with a step-by-step setup guide |
| Proprietary APIs (Plex, Emby) change without notice | Provider breaks after a server update | Contract tests with recorded responses; detect server version and degrade gracefully |
| Very large libraries make views sluggish | Poor first impression | Virtualised views, thumbnail cache, synthetic large-library test in the Phase 1 gate |
| Sixteen sources is a long tail | Project never reaches a usable release | Phase 1 is a complete product on its own; every later phase is optional |

**Open questions for you**

- Name and application signature: is "Lilt" acceptable as a placeholder?
- Licence: MIT is proposed to match Haiku; any objection?
- Architectures: x86\_64 only, or also ARM64, which Haiku is working on ([Phoronix](https://phoronix.com/news/Haiku-R1-Beta-6))?
- Audiobooks: a separate section from music, or mixed into one library?
- Extras: do you want lyrics, MusicBrainz lookups or scrobbling in the plan, and in which phase?
- Qt: is a hybrid with a Qt front end ever acceptable, or is Interface Kit a fixed requirement?

## Sources

Only the Dopamine README was opened in full; the other pages were read as search excerpts, so API details marked *verify* should be confirmed against the official documentation or Haiku headers.

- [Dopamine repository](https://github.com/digimezzo/dopamine): Electron, Angular and TypeScript, GPL-3.0
- [Haiku R1/beta6 release, Phoronix](https://phoronix.com/news/Haiku-R1-Beta-6)
- [Be Book Media Kit index](https://datatime.sourceforge.net/Be%20Book/Release%20Notes/MediaKit.html)
- [Haiku FFmpeg media plug-in source](https://git.haiku-os.org/haiku.git/tree/src/add-ons/media/plugins/ffmpeg/FFmpegPlugin.cpp)
- [Haiku ticket #12696, Opus and VP9 in MediaPlayer](https://dev.haiku-os.org/ticket/12696)
- [OpenSubsonic extension notes](https://github.com/nc-music/oc-music/wiki/OpenSubsonic-API)
- [opensubsonic client auth source](https://docs.rs/crate/opensubsonic/0.2.0/source/src/auth.rs)
- [Jellyfin API overview](https://jmshrv.com/posts/jellyfin-api/)
- [Authenticating with Plex](https://forums.plex.tv/t/authenticating-with-plex/609370)
- [Audiobookshelf API documentation](https://api.audiobookshelf.org)
- [Kodi JSON-RPC API v13](https://kodi.wiki/view/JSON-RPC_API/v13)
- [Google OAuth 2.0 for installed applications](https://googleapis.github.io/google-api-python-client/docs/oauth-installed.html)
- [Dropbox and OneDrive auth overview](https://github.com/ciberkids/cloud-drive-sync/wiki/Home/c51a3c9d5616732660f93cb06329bdcb378affc7)
- [Box and pCloud account notes](https://github.com/degoya/rDownloader/wiki/hosters-and-accounts)
- [pCloud getfilelink](https://docs.pcloud.com/methods/streaming/getfilelink.html)
