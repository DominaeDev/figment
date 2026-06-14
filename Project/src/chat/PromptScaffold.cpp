#include <pch.h>
#include "chat/PromptScaffold.h"
#include "io/XmlSerializable.h"

using namespace fig::io;

namespace fig::chat
{
	auto PromptBlock::SerializeInfo()
	{
		return XmlFields(
			AsAttribute { "condition",	&PromptBlock::condition,
				[](auto&& value) -> fig::string { return value.to_string(); },
				[](auto&& value) -> Condition { return Condition { value }; }
			}	.Default(Condition::Always),
			AsAttribute { "order",		&PromptBlock::order },
			AsText		{				&PromptBlock::content }
		);
		
		static_assert(XmlSerializable<PromptBlock>);
	}

	auto PromptScaffold::SerializeInfo()
	{
		return XmlFields(
			AsElement { "Name",		&PromptScaffold::name },
			AsElement { "Block",	&PromptScaffold::blocks }
				.Collection("Blocks")
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

		auto rootNode = xml.GetRoot();

		if (!XmlDeserialize(rootNode, *this))
			return FileError::ReadError;

		// Read options
		if (auto optionsNode = xml.GetFirstElement("Options"))
		{
			auto optionNode = optionsNode.value().GetFirstElementAny();
			while (optionNode.has_value())
			{
				auto& node = optionNode.value();

				if (auto try_id = node["id"].TryGet<fig::handle>())
				{
					if (node.Is("Toggle"))
					{
						options.emplace_back(PromptOptionToggle {
							.id = try_id.value(),
							.label = node.TryGetValue<fig::string>().value_or((fig::string)try_id.value()),
							.hint = node["hint"].TryGet<fig::string>().value_or({}),
							.defaultValue = node["default"].TryGet<bool>().value_or({})
						});
					}
					else if (node.Is("Number"))
					{
						options.emplace_back(PromptOptionNumber {
							.id = try_id.value(),
							.label = node.TryGetValue<fig::string>().value_or((fig::string)try_id.value()),
							.hint = node["hint"].TryGet<fig::string>().value_or({}),
							.defaultValue = node["default"].TryGet<int32_t>().value_or({})
						});
					}
					else if (node.Is("Text"))
					{
						options.emplace_back(PromptOptionText {
							.id = try_id.value(),
							.label = node.TryGetValue<fig::string>().value_or((fig::string)try_id.value()),
							.hint = node["hint"].TryGet<fig::string>().value_or({}),
							.defaultValue = node["default"].TryGet<fig::string>().value_or({})
						});
					}
				}
				optionNode = optionNode.value().GetNextSiblingAny();
			}
		}

		return FileError::NoError;
	}
}