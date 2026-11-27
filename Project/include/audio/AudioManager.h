#pragma once

#include "Figment.h"

namespace fig::audio
{
	class AudioManager
	{
	public:
		AudioManager();

		bool PlaySound(fig::bytes&& data);
		int32_t EnqueueSound(fig::byte_span data, float fVolume = -1.0f, float fDelay = Constants::TTS::DefaultDialogueDelay);
		bool IsPlaying() const noexcept;
		bool StopSound(int32_t clipId);

		void StopAllSounds();
		void SetVolume(float fVolume);
		float GetVolume() const noexcept;

		void Update(float fElapsed);

	private:
		void StopCurrentSound();
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
				volume = other.volume;
				delay = other.delay;
				clipId = other.clipId;
				other.audioData = NULL;
			}

			~AudioClip()
			{
				SDL_free(audioData);
			}

			AudioClip& operator=(AudioClip&& other) noexcept
			{
				if (this == &other)
					return *this;

				SDL_free(audioData);
				spec = other.spec;
				audioData = other.audioData;
				audioLength = other.audioLength;
				volume = other.volume;
				delay = other.delay;
				clipId = other.clipId;
				other.audioData = nullptr;
				return *this;
			}

			SDL_AudioSpec spec {};
			Uint8* audioData {};
			Uint32 audioLength {};
			float volume { 1.0f };
			float delay {};
			int32_t clipId {};
		};

		bool PlayClip(const AudioClip& clip);

		std::list<AudioClip> _clips;
		float _fDelay = -1.0f;
		int32_t _currentClipId {};
		static int32_t _nextClipId;
	};
}