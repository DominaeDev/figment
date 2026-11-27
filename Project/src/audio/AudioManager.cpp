#include <pch.h>
#include "audio/AudioManager.h"
#include "user/UserSettings.h"
#include "user/UserManager.h"

namespace fig::audio
{
	int32_t AudioManager::_nextClipId { 1 };

	AudioManager::AudioManager()
	{
		SDL_AudioSpec spec {
			.format = SDL_AudioFormat::SDL_AUDIO_F32,
			.channels = 2,
			.freq = 44100,
		};

		_stream = fig::sdl::AudioStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
		SDL_ResumeAudioStreamDevice(_stream.get());
	}

	bool AudioManager::PlaySound(fig::bytes&& data)
	{
		if (_stream.empty())
			return false;

		SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());

		SDL_AudioSpec spec;
		Uint8* audioData;
		Uint32 audioLength;
		if (not SDL_LoadWAV_IO(io, true, &spec, &audioData, &audioLength))
			return false; // Error
		
		if (not SDL_SetAudioStreamFormat(_stream.get(), &spec, nullptr))
		{
			SDL_free(audioData);
			return false;
		}

		if (not SDL_PutAudioStreamData(_stream.get(), audioData, static_cast<int>(audioLength)))
		{
			SDL_free(audioData);
			return false;
		}

		SDL_free(audioData);
		return true;
	}

	void AudioManager::SetVolume(float fVolume)
	{
		if (_stream.empty())
			return;
		
		float gain = fVolume <= 0.0f ? 0.0f : std::pow(10.0f, (fVolume - 1.0f) * 40.0f / 20.0f);

		SDL_SetAudioStreamGain(_stream.get(), std::clamp(gain, 0.0f, 1.0f));
	}

	float AudioManager::GetVolume() const noexcept
	{
		if (_stream.empty())
			return -1.0f;

		return SDL_GetAudioStreamGain(_stream.get());
	}

	int32_t AudioManager::EnqueueSound(fig::byte_span data, float fVolume, float fDelay)
	{
		if (_stream.empty())
			return 0;

		if (fVolume < 0.0f)
		{
			if (Global::GetUserManager().IsSignedIn())
				fVolume = Global::GetUserSettings().GetFloat(fig::io::UserSetting::TTS::Volume);
			else
				fVolume = 1.0f;
		}

		if (SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size()))
		{
			AudioClip clip;
			clip.volume = fVolume;
			clip.delay = fDelay;
			int32_t clipId = clip.clipId = _nextClipId++;

			if (not SDL_LoadWAV_IO(io, true, &clip.spec, &clip.audioData, &clip.audioLength))
				return 0; // Error

			_clips.push_back(std::move(clip));
			return clipId;
		}
		return 0;
	}

	void AudioManager::StopAllSounds()
	{
		if (_stream.empty())
			return;

		_clips.clear();
		SDL_ClearAudioStream(_stream.get());
		_fDelay = 0.0f;
		_currentClipId = 0;
	}

	void AudioManager::StopCurrentSound()
	{
		if (_stream.empty())
			return;

		SDL_ClearAudioStream(_stream.get());
		_fDelay = 0.0f;
		_currentClipId = 0;
	}

	void AudioManager::Update(float fElapsed)
	{
		if (_stream.empty())
			return;
		if (_clips.empty())
			return;

		auto& top = _clips.front();

		// Is playing?
		if (SDL_GetAudioStreamAvailable(_stream.get()) > 0)
		{
			_fDelay = std::max(top.delay, 0.0f);
			_currentClipId = 0;
			return;
		}

		if (_fDelay > 0.0f)
		{
			_fDelay -= fElapsed;
			if (_fDelay > 0.0f)
				return;
		}

		// Play next
		PlayClip(top);
		_clips.pop_front();
	}

	bool AudioManager::PlayClip(const AudioClip& clip)
	{
		if (_stream.empty())
			return false;

		SetVolume(clip.volume);
		if (not SDL_SetAudioStreamFormat(_stream.get(), &clip.spec, nullptr))
			return false;
		if (not SDL_PutAudioStreamData(_stream.get(), clip.audioData, static_cast<int>(clip.audioLength)))
			return false;
		_currentClipId = clip.clipId;
		return true;
	}

	bool AudioManager::IsPlaying() const noexcept
	{
		if (_stream.empty())
			return false;

		return SDL_GetAudioStreamAvailable(_stream.get()) > 0;
	}

	bool AudioManager::StopSound(int32_t clipId)
	{
		if (_currentClipId == clipId)
		{
			StopCurrentSound();
			return true;
		}
		
		return _clips.remove_if([clipId](auto&& clip) {
			return clip.clipId == clipId;
		}) > 0uz;
	}
}