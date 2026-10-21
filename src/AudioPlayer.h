#ifndef SAKURA_AUDIO_PLAYER_H
#define SAKURA_AUDIO_PLAYER_H


#include <Entry.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <Messenger.h>


class BMediaFile;
class BMediaTrack;
class BSoundPlayer;


class AudioPlayer {
public:
	explicit AudioPlayer(const BMessenger& target);
	~AudioPlayer();

	bool IsPlaying() const;
	status_t Play(const entry_ref& ref);
	void Stop();

private:
	static void _PlayBuffer(void* cookie, void* buffer, size_t size,
		const media_raw_audio_format& format);
	void _FillBuffer(void* buffer, size_t size,
		const media_raw_audio_format& format);
	void _Finish();

	mutable BLocker fLock;
	BMessenger fTarget;
	BMediaFile* fMediaFile;
	BMediaTrack* fMediaTrack;
	BSoundPlayer* fSoundPlayer;
	bool fFinishSent;
	bool fPlaying;
};


#endif
