#include <pch.h>
#include "chat/PromptScaffold.h"
#include "io/XmlSerializable.h"
#include "text/TextEvaluator.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::chat
{
	static const std::map<PromptBlockInfo::Type, fig::string> TypeMapping {
		{ PromptBlockInfo::Type::Static,	"static" },
		{ PromptBlockInfo::Type::Persona,	"persona" },
		{ PromptBlockInfo::Type::User,		"user" },
	};

	static const std::map<PromptPriority, fig::string> PriorityMapping {
		{ PromptPriority::Normal,	"normal" },
		{ PromptPriority::Low,		"low" },
		{ PromptPriority::High,		"high" },
	};

	auto PromptBlockInfo::SerializeInfo() noexcept
	{
		return Fields(
			AsAttribute { "type",		&PromptBlockInfo::type,
				[](auto& value) { return enum_serialize(value, TypeMapping); },
				[](auto& value) { return enum_deserialize(value, TypeMapping); }
			},
			AsAttribute { "priority",	&PromptBlockInfo::priority,
				[](auto& value) { return enum_serialize(value, PriorityMapping); },
				[](auto& value) { return enum_deserialize(value, PriorityMapping); }
			},
			AsAttribute { "condition",	&PromptBlockInfo::condition }
				.Default(Condition::Always),
			AsAttribute { "order",		&PromptBlockInfo::order },
			AsText		{				&PromptBlockInfo::content }
		);
		
		static_assert(XmlSerializable<PromptBlockInfo>);
	}

	auto PromptScaffold::SerializeInfo() noexcept
	{
		return Fields(
			AsElement { "Name",		&PromptScaffold::name },
			AsElement { "Block",	&PromptScaffold::blocks }
				.Collection("Blocks"),
			AsElement { "Grammar",	&PromptScaffold::grammar },
			AsElement { "Options",	&PromptScaffold::options }
		);

		static_assert(XmlSerializable<PromptScaffold>);
	}

	FileError PromptScaffold::LoadFromXml(const fig::path& path)
	{
		if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
			return FileError::NotFound;

		XmlReader xml(path, "PromptScaffold");
		if (not xml.IsOk())
			return FileError::UnrecognizedFormat;

		if (!XmlDeserialize(xml.GetRoot(), *this))
			return FileError::ReadError;

		// Sort blocks by order
		std::ranges::stable_sort(blocks, std::ranges::less {}, &PromptBlockInfo::order);

		return FileError::NoError;
	}

}