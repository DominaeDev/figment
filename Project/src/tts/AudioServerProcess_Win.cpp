#include <pch.h>
#include "tts/AudioServerProcess_Win.h"
#include "io/FileUtility.h"

using namespace fig::gui;

namespace fig::tts
{
	AudioServerProcess_Win::~AudioServerProcess_Win()
	{
		Stop();
	}

	std::expected<void, std::string> AudioServerProcess_Win::Start(const AudioServerConfiguration& config)
	{
		fig::string serverJson = config.ToJson();

		if (auto error = fig::io::WriteTextFile(fig::path { "tts/server.json" }, serverJson); error != fig::io::FileError::NoError)
			return {}; // Write error

		// Start server
		_hJob = CreateJobObjectW(nullptr, nullptr);
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
		limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

		SetInformationJobObject(
			_hJob,
			JobObjectExtendedLimitInformation,
			&limits,
			sizeof(limits));

		STARTUPINFOW startupInfo = { sizeof(startupInfo) };
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;

		BOOL bInheritHandles = TRUE;
		if (auto writeHandle = CreateWriteHandle())
		{
			startupInfo.hStdOutput = *writeHandle;
			startupInfo.hStdError = *writeHandle;
		}

		_processInfo = PROCESS_INFORMATION {};

		auto exePath = fig::path { Constants::Paths::TTSServer };
		auto commandLine = from_utf8(std::format("{} --config \"tts/server.json\"", exePath.filename().u8string())); //! @todo -> tts/server.json

		if (CreateProcessW(
			exePath.wstring().c_str(),
			commandLine.data(),
			nullptr,
			nullptr,
			bInheritHandles,
			CREATE_SUSPENDED | CREATE_NO_WINDOW,
			nullptr,
			nullptr,
			&startupInfo,
			&_processInfo))
		{
			// Init io pipe
			CloseHandle(_writeHandle);
			_writeHandle = nullptr;
			_readThread = std::jthread(std::bind_front(&AudioServerProcess_Win::ReadLoop, this));

			// Process exit callback
			_cbCtx = { this, _processInfo.hProcess };
			_cbWait = CreateThreadpoolWait(ProcessEndedCallback, &_cbCtx, NULL);
			SetThreadpoolWait(_cbWait, _processInfo.hProcess, NULL);

			// Assign job and resume
			AssignProcessToJobObject(_hJob, _processInfo.hProcess);
			ResumeThread(_processInfo.hThread);

			LogLn("audiocpp_server process started.");
			return {};
		}

		// Failed
		LogLn("Failed to launch audiocpp_server process.");
		CloseHandle(_hJob);
		_hJob = 0;
		return {};
	}

	void AudioServerProcess_Win::Stop()
	{
		// Release io pipe
		if (_readHandle)
		{
			CancelIoEx(_readHandle, nullptr);
			_readHandle = nullptr;
		}

		if (_readThread.joinable())
			_readThread.join();

		if (_writeHandle)
		{
			CloseHandle(_writeHandle);
			_writeHandle = nullptr;
		}

		// Shut down server
		if (_hJob)
		{
			if (_processInfo.hProcess and _processInfo.hProcess != INVALID_HANDLE_VALUE)
				CloseHandle(_processInfo.hProcess);
			if (_processInfo.hThread and _processInfo.hThread != INVALID_HANDLE_VALUE)
				CloseHandle(_processInfo.hThread);
			CloseHandle(_hJob);

			_hJob = NULL;
			_processInfo = {};

			LogLn("audiocpp_server process stopped.");
		}

		if (_cbWait)
		{
			CloseThreadpoolWait(_cbWait);
			_cbWait = NULL;
		}
	}

	bool AudioServerProcess_Win::IsRunning(int32_t& exitCode)
	{
		if (_processInfo.hProcess)
		{
			DWORD exitCode;
			GetExitCodeProcess(_processInfo.hProcess, &exitCode);
			if (exitCode == STILL_ACTIVE)
				return true; // Still running

			LogLn(std::format("audiocpp_server exited with code {}", exitCode));
			PushEvent(UserEvent::TTSServerShutdown);
		}
		return false;
	}

	std::optional<HANDLE> AudioServerProcess_Win::CreateWriteHandle()
	{
		SECURITY_ATTRIBUTES pipeAttributes = {};
		pipeAttributes.nLength = sizeof(pipeAttributes);
		pipeAttributes.bInheritHandle = TRUE;

		if (not CreatePipe(&_readHandle, &_writeHandle, &pipeAttributes, 0))
			return std::nullopt;

		if (not SetHandleInformation(_readHandle, HANDLE_FLAG_INHERIT, 0))
			return std::nullopt;

		return _writeHandle;
	}

	void AudioServerProcess_Win::ReadLoop(std::stop_token stopToken)
	{
		char buffer[4096];
		DWORD bytesRead;

		while (ReadFile(_readHandle, buffer, sizeof(buffer), &bytesRead, nullptr) and bytesRead > 0)
		{
			fig::string log { buffer, bytesRead };
			Log(log);

			if (log.contains("ggml_cuda_init"))
				PushEvent(UserEvent::TTSServerLoadingModel);
			else if (log.contains("ggml_backend_cuda_graph_compute"))
				PushEvent(UserEvent::TTSServerGenerating);
			else if (log.contains("audiocpp_server failed"))
				PushEvent(UserEvent::TTSServerError);
		}

		CloseHandle(_readHandle);
		_readHandle = nullptr;
	}

	void CALLBACK AudioServerProcess_Win::ProcessEndedCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT Result)
	{
		CallbackContext* pCtx = static_cast<CallbackContext*>(Context);
		DWORD exitCode;
		GetExitCodeProcess(pCtx->hProcess, &exitCode);
		LogLn(std::format("audiocpp_server exited with code {}", exitCode));
		pCtx->pInstance->Stop();

		PushEvent(UserEvent::TTSServerShutdown);
	}

}