#pragma once

#include "Figment.h"
#include "tts/AudioServerConfiguration.h"

namespace fig::tts
{
	class IAudioServerProcess
	{
	public:
		virtual ~IAudioServerProcess() {}

		virtual bool Start(const AudioServerConfiguration& config) = 0;
		virtual void Stop() = 0;
		virtual bool IsRunning(int32_t& exitCode) = 0;
	};
}