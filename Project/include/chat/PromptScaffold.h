#pragma once

#include "Figment.h"
#include "text/Condition.h"
#include "chat/UserDefinedOptions.h"

namespace fig::chat
{
	enum class PromptPriority : int8_t
	{
		Low = -1,
		Normal = 0,
		High = 1,
	};

	struct PromptBlockInfo
	{
		enum class Type
		{
			Static,
			Persona,
			User,
		} type {};

		int32_t order = 0;
		PromptPriority priority { PromptPriority:: Normal };
		Condition condition;
		fig::string content;
		int32_t ttl = 0;
	public:
		static auto XmlFields() noexcept;
	};

	struct PromptBlock
	{
		size_t id;
		fig::string content;
		fig::hash hash;
		PromptBlockInfo::Type blockType;
		Role role = Role::Undefined;
		int32_t ttl = 0;
	};

	struct PromptScaffold
	{
		fig::string name;

		std::vector<PromptBlockInfo> blocks;
		UserDefinedOptions options;
		fig::string grammar;

		fig::io::FileError LoadFromXml(const fig::path& filename);
	public:
		static auto XmlFields() noexcept;
	};
}
