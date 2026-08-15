#pragma once

#include "Figment.h"
#include <thread>

namespace fig::tts
{
    class AudioServerProcess
    {
    public:
        ~AudioServerProcess();

        std::expected<void, std::string> Start(std::span<const char* const> arguments);
        void Stop();
        bool IsRunning(int32_t& exitCode);

    private:
        SDL_Process* _process = nullptr;

        void ReadLoop(SDL_IOStream* output, std::stop_token stopToken);
        std::jthread _logThread;
    };
}