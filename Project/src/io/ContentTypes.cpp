#include <pch.h>
#include "io/ContentTypes.h"

using namespace fig::data;

namespace fig::io
{
	std::expected<Character, FileError> AssetLoader<Character>::Load(const Asset& asset)
	{
		Character character;
		if (character.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return character;
		return std::unexpected(FileError::ReadError);
	}

	std::expected<Scenario, FileError> AssetLoader<Scenario>::Load(const Asset& asset)
	{
		Scenario scenario;
		if (scenario.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return scenario;
		return std::unexpected(FileError::ReadError);
	}

	std::expected<ChatInstance, FileError> AssetLoader<ChatInstance>::Load(const Asset& asset)
	{
		ChatInstance chatInstance;
		if (chatInstance.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return chatInstance;
		return std::unexpected(FileError::ReadError);
	}

	std::expected<ChatLog, FileError> AssetLoader<ChatLog>::Load(const Asset& asset)
	{
		ChatLog chatLog;
		if (chatLog.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return chatLog;
		return std::unexpected(FileError::ReadError);
	}
}