#include <pch.h>
#include "tts/ProcessLogger_Win.h"

namespace fig::tts
{
	std::optional<HANDLE> ProcessLogger_Win::CreateWriteHandle()
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

	void ProcessLogger_Win::Start()
	{
		CloseHandle(_writeHandle);
		_writeHandle = nullptr;
		_readThread = std::jthread(std::bind_front(&ProcessLogger_Win::ReadLoop, this));
	}

	void ProcessLogger_Win::Stop()
	{
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
	}

	ProcessLogger_Win::~ProcessLogger_Win()
	{
		Stop();
	}

	void ProcessLogger_Win::ReadLoop(std::stop_token stopToken)
	{
		char buffer[4096];
		DWORD bytesRead;

		while (ReadFile(_readHandle, buffer, sizeof(buffer), &bytesRead, nullptr) and bytesRead > 0)
			Log(fig::string(buffer, bytesRead));

		CloseHandle(_readHandle);
		_readHandle = nullptr;
	}
}