#pragma once

#include "Figment.h"
#include "tts/IAudioServerProcess.h"
#include <thread>

namespace fig::tts
{
    class AudioServerProcess_SDL : public IAudioServerProcess
    {
    public:
        ~AudioServerProcess_SDL();

        bool Start(const AudioServerConfiguration& config) override;
        void Stop() override;
        bool IsRunning(int32_t& exitCode) override;

    private:
        SDL_Process* _process = nullptr;

        void ReadLoop(SDL_IOStream* output, std::stop_token stopToken);
        std::jthread _logThread;
    };
}