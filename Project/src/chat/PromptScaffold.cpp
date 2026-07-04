#include <pch.h>
#include "chat/PromptScaffold.h"
#include "io/XmlSerialize.h"
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

	auto PromptBlockInfo::XmlFields() noexcept
	{
		return Fields(
			Attribute { "type",		&PromptBlockInfo::type,
				[](auto& value) { return enum_serialize(value, TypeMapping); },
				[](auto& value) { return enum_deserialize(value, TypeMapping); }
			},
			Attribute { "priority",	&PromptBlockInfo::priority,
				[](auto& value) { return enum_serialize(value, PriorityMapping); },
				[](auto& value) { return enum_deserialize(value, PriorityMapping); }
			},
			Attribute { "condition",	&PromptBlockInfo::condition }
				.Default(Condition::Always),
			Attribute { "order",		&PromptBlockInfo::order },
			Text		{				&PromptBlockInfo::content }
		);
		
		static_assert(IsXmlSerializable<PromptBlockInfo>);
	}

	auto PromptScaffold::XmlFields() noexcept
	{
		return Fields(
			Element { "Name",		&PromptScaffold::name },
			Element { "Block",	&PromptScaffold::blocks }
				.Collection("Blocks"),
			Element { "Grammar",	&PromptScaffold::grammar },
			Element { "Options",	&PromptScaffold::options }
		);

		static_assert(IsXmlSerializable<PromptScaffold>);
	}

	FileError PromptScaffold::LoadFromXml(const fig::path& path)
	{
		if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
			return FileError::NotFound;

		XmlReader xml(path, "PromptScaffold");
		if (not xml.IsOk())
			return FileError::UnrecognizedFormat;

		if (!Deserialize(xml.GetRoot(), *this))
			return FileError::ReadError;

		// Sort blocks by order
		std::ranges::stable_sort(blocks, std::ranges::less {}, &PromptBlockInfo::order);

		return FileError::NoError;
	}

}