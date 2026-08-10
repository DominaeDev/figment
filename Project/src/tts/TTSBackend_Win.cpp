#include <pch.h>
#include "tts/TTSBackend_Win.h"

namespace fig::tts
{
	void CALLBACK TTSBackend_Win::ProcessEndedCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT Result)
	{
		CallbackContext* pCtx = static_cast<CallbackContext*>(Context);
		DWORD exitCode;
		GetExitCodeProcess(pCtx->hProcess, &exitCode);
		LogLn(std::format("audiocpp_server exited with code {}", exitCode));

		pCtx->pInstance->Shutdown();
	}

	TTSBackend_Win::TTSBackend_Win() : ITTSBackend()
	{
	}

	TTSBackend_Win::~TTSBackend_Win()
	{
		Shutdown();
	}

	bool TTSBackend_Win::Initialize()
	{
		if (_status != TTSStatus::Uninitialized)
		{
			if (CheckHealth())
				return true; // Already initialized
			Shutdown(); // Restart
		}

		// Start server
		_hJob = CreateJobObjectW(nullptr, nullptr);
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
		limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

		SetInformationJobObject(
			_hJob,
			JobObjectExtendedLimitInformation,
			&limits,
			sizeof(limits));

		_logger.Stop();

		STARTUPINFOW startupInfo = { sizeof(startupInfo) };
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
			
		if (auto writeHandle = _logger.CreateWriteHandle())
		{
			startupInfo.hStdOutput = *writeHandle;
			startupInfo.hStdError = *writeHandle;
		}

		_processInfo = PROCESS_INFORMATION {};

		std::wstring exePath = L"./bin/audiocpp/audiocpp_server.exe";
		std::wstring commandLine = L"audiocpp_server.exe --config \"bin/audiocpp/server.json\"";

		if (CreateProcessW(
			exePath.c_str(),
			commandLine.data(),
			nullptr,
			nullptr,
			TRUE,
			CREATE_SUSPENDED,
			nullptr,
			nullptr,
			&startupInfo,
			&_processInfo))
		{
			_logger.Start();

			// Process exit callback
			_cbCtx = { this, _processInfo.hProcess };
			_cbWait = CreateThreadpoolWait(ProcessEndedCallback, &_cbCtx, NULL);
			SetThreadpoolWait(_cbWait, _processInfo.hProcess, NULL);

			// Assign job and resume
			AssignProcessToJobObject(_hJob, _processInfo.hProcess);
			ResumeThread(_processInfo.hThread);

			_status = TTSStatus::ServerStarted;
			LogLn("audiocpp_server process started.");
			return true;
		}

		// Failed
		LogLn("Failed to launch audiocpp_server process.");
		CloseHandle(_hJob);
		_hJob = 0;
		return false;
	}

	void TTSBackend_Win::Shutdown()
	{
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
		
		_status = TTSStatus::Uninitialized;
	}

	bool TTSBackend_Win::Restart()
	{
		if (_status == TTSStatus::Uninitialized)
			return false;
		
		Shutdown();
		return Initialize();
	}

	bool TTSBackend_Win::CheckHealth()
	{
		if (_status == TTSStatus::Uninitialized)
			return false;

		if (_processInfo.hProcess)
		{
			DWORD exitCode;
			GetExitCodeProcess(_processInfo.hProcess, &exitCode);
			if (exitCode == STILL_ACTIVE)
				return true; // Still running
			LogLn(std::format("audiocpp_server exited with code {}", exitCode));
		}
		return false;
	}

	std::expected<TTSData, TTSError> TTSBackend_Win::SendRequest(TTSTask task, fig::string_view text)
	{
		if (not _http.IsConnected())
		{
			if (not _http.Connect(L"127.0.0.1", 8080))
				return std::unexpected(TTSError::Unavailable);
		}

		fig::string request = std::format("{{ \"model\": \"{}\", \"input\": \"{}\" }}", "pocket-tts", text);
		if (auto response = _http.Post(L"/v1/audio/speech", request))
		{
			auto& data = response.value();
			return std::move(data);
		}
		else
			return std::unexpected(TTSError::Failed); //! @todo
	}
}