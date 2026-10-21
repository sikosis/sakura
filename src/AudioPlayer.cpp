#include "AudioPlayer.h"

#include "Messages.h"

#include <Autolock.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <SoundPlayer.h>

#include <algorithm>
#include <cstring>


AudioPlayer::AudioPlayer(const BMessenger& target)
	: fLock("audio player"), fTarget(target), fMediaFile(NULL), fMediaTrack(NULL),
		fSoundPlayer(NULL), fFinishSent(false), fPlaying(false) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


AudioPlayer::~AudioPlayer() {
	Stop();
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool AudioPlayer::IsPlaying() const {
	BAutolock locker(&fLock);
	return fPlaying;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t AudioPlayer::Play(const entry_ref& ref) {
	Stop();

	BAutolock locker(&fLock);
	fMediaFile = new BMediaFile(&ref);
	status_t status = fMediaFile->InitCheck();
	if (status != B_OK) {
		delete fMediaFile;
		fMediaFile = NULL;
		return status;
	}

	media_format decodedFormat;
	decodedFormat.type = B_MEDIA_RAW_AUDIO;
	decodedFormat.u.raw_audio = media_raw_audio_format::wildcard;

	for (int32 index = 0; index < fMediaFile->CountTracks(); index++) {
		BMediaTrack* candidate = fMediaFile->TrackAt(index);
		if (candidate == NULL)
			continue;

		media_format candidateFormat = decodedFormat;
		if (candidate->DecodedFormat(&candidateFormat) == B_OK
			&& candidateFormat.type == B_MEDIA_RAW_AUDIO) {
			fMediaTrack = candidate;
			decodedFormat = candidateFormat;
			break;
		}
		fMediaFile->ReleaseTrack(candidate);
	}

	if (fMediaTrack == NULL) {
		delete fMediaFile;
		fMediaFile = NULL;
		return B_MEDIA_BAD_FORMAT;
	}

	fSoundPlayer = new BSoundPlayer(&decodedFormat.u.raw_audio, "Sakura playback",
		_PlayBuffer, NULL, this);
	status = fSoundPlayer->InitCheck();
	if (status != B_OK) {
		fMediaFile->ReleaseTrack(fMediaTrack);
		fMediaTrack = NULL;
		delete fSoundPlayer;
		fSoundPlayer = NULL;
		delete fMediaFile;
		fMediaFile = NULL;
		return status;
	}

	fFinishSent = false;
	fPlaying = true;
	status = fSoundPlayer->Start();
	if (status != B_OK) {
		fPlaying = false;
		fMediaFile->ReleaseTrack(fMediaTrack);
		fMediaTrack = NULL;
		delete fSoundPlayer;
		fSoundPlayer = NULL;
		delete fMediaFile;
		fMediaFile = NULL;
		return status;
	}
	fSoundPlayer->SetHasData(true);
	return B_OK;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void AudioPlayer::Stop() {
	BSoundPlayer* soundPlayer = NULL;
	{
		BAutolock locker(&fLock);
		soundPlayer = fSoundPlayer;
		fSoundPlayer = NULL;
		fPlaying = false;
	}

	if (soundPlayer != NULL) {
		soundPlayer->SetHasData(false);
		soundPlayer->Stop();
		delete soundPlayer;
	}

	BAutolock locker(&fLock);
	if (fMediaFile != NULL && fMediaTrack != NULL)
		fMediaFile->ReleaseTrack(fMediaTrack);
	fMediaTrack = NULL;
	delete fMediaFile;
	fMediaFile = NULL;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void AudioPlayer::_PlayBuffer(void* cookie, void* buffer, size_t size,
	const media_raw_audio_format& format) {
	AudioPlayer* player = static_cast<AudioPlayer*>(cookie);
	player->_FillBuffer(buffer, size, format);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void AudioPlayer::_FillBuffer(void* buffer, size_t size,
	const media_raw_audio_format& format) {
	BAutolock locker(&fLock);
	if (!fPlaying || fMediaTrack == NULL) {
		std::memset(buffer, 0, size);
		return;
	}

	size_t bytesPerSample = format.format & 0x0f;
	size_t frameSize = bytesPerSample * format.channel_count;
	if (frameSize == 0) {
		std::memset(buffer, 0, size);
		_Finish();
		return;
	}

	int64 requestedFrames = (int64)(size / frameSize);
	int64 decodedFrames = requestedFrames;
	status_t status = fMediaTrack->ReadFrames(buffer, &decodedFrames);
	size_t decodedBytes = (size_t)std::max<int64>(decodedFrames, 0) * frameSize;
	if (decodedBytes < size) {
		std::memset(static_cast<uint8*>(buffer) + decodedBytes, 0,
			size - decodedBytes);
	}

	if (status != B_OK || decodedFrames < requestedFrames)
		_Finish();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void AudioPlayer::_Finish() {
	if (fFinishSent)
		return;

	fFinishSent = true;
	fPlaying = false;
	fTarget.SendMessage(kMessagePlaybackFinished);
}
//---------------------------------------------------------------------------------------------------------------------------------//
