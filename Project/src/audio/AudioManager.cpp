#include <pch.h>
#include "audio/AudioManager.h"

namespace fig::audio
{
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
		
		SDL_SetAudioStreamGain(_stream.get(), std::clamp(fVolume, 0.0f, 1.0f));
	}

	float AudioManager::GetVolume() const noexcept
	{
		if (_stream.empty())
			return -1.0f;

		return SDL_GetAudioStreamGain(_stream.get());
	}

	bool AudioManager::EnqueueSound(const fig::bytes& data, float fDelay)
	{
		if (_stream.empty())
			return false;

		SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());

		AudioClip clip;
		clip.delay = fDelay;

		if (not SDL_LoadWAV_IO(io, true, &clip.spec, &clip.audioData, &clip.audioLength))
			return false; // Error

		_clips.emplace(std::move(clip));
		return true;
	}

	void AudioManager::StopAllSounds()
	{
		if (_stream.empty())
			return;

		queue_clear(_clips);
		SDL_ClearAudioStream(_stream.get());
		_fDelay = 0.0f;
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
		_clips.pop();
	}

	bool AudioManager::PlayClip(const AudioClip& clip)
	{
		if (_stream.empty())
			return false;

		if (not SDL_SetAudioStreamFormat(_stream.get(), &clip.spec, nullptr))
			return false;
		if (not SDL_PutAudioStreamData(_stream.get(), clip.audioData, static_cast<int>(clip.audioLength)))
			return false;
		return true;
	}
}