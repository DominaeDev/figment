#pragma once

#include "Figment.h"

namespace fig::audio
{
	class AudioManager
	{
	public:
		AudioManager();

		bool PlaySound(fig::bytes&& data);
		bool EnqueueSound(const fig::bytes& data, float fDelay = Constants::TTS::DefaultDelay);
		bool IsPlaying() const noexcept;

		void StopAllSounds();
		void SetVolume(float fVolume);
		float GetVolume() const noexcept;

		void Update(float fElapsed);

	private:
		fig::sdl::AudioStream _stream {};

		struct AudioClip
		{
			AudioClip() = default;
			AudioClip(const AudioClip& other) = delete;
			AudioClip(AudioClip&& other) noexcept
			{
				spec = other.spec;
				audioData = other.audioData;
				audioLength = other.audioLength;
				delay = other.delay;
				other.audioData = NULL;
			}

			~AudioClip()
			{
				SDL_free(audioData);
			}

			SDL_AudioSpec spec {};
			Uint8* audioData {};
			Uint32 audioLength {};
			float delay {};
		};

		bool PlayClip(const AudioClip& clip);

		std::queue<AudioClip> _clips;
		float _fDelay = -1.0f;
	};
}