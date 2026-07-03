#include <pch.h>
#include "chat/UserDefinedOptions.h"

using namespace fig::data;

namespace fig::chat
{
	bool UserDefinedOptions::LoadFromXml(XmlReaderElement xml)
	{
		options.clear();

		auto optionNode = xml.GetFirstElementAny();
		while (optionNode.has_value())
		{
			auto& node = optionNode.value();

			if (auto try_id = node["id"].TryGet<fig::handle>())
			{
				if (node.Is("Toggle"))
				{
					options.emplace_back(UserDefinedToggle {
						.id = try_id.value(),
						.label = node.TryGetValue<fig::string>().value_or((fig::string)try_id.value()),
						.hint = node["hint"].TryGet<fig::string>().value_or({}),
						.defaultValue = node["default"].TryGet<bool>().value_or({})
					});
				}
				else if (node.Is("Number"))
				{
					options.emplace_back(UserDefinedNumber {
						.id = try_id.value(),
						.label = node.TryGetValue<fig::string>().value_or((fig::string)try_id.value()),
						.hint = node["hint"].TryGet<fig::string>().value_or({}),
						.defaultValue = node["default"].TryGet<int32_t>().value_or({})
					});
				}
				else if (node.Is("Text"))
				{
					options.emplace_back(UserDefinedText {
						.id = try_id.value(),
						.label = node.TryGetValue<fig::string>().value_or((fig::string)try_id.value()),
						.hint = node["hint"].TryGet<fig::string>().value_or({}),
						.defaultValue = node["default"].TryGet<fig::string>().value_or({})
					});
				}
			}
			optionNode = optionNode.value().GetNextSiblingAny();
		}

		return not options.empty();
	}

	void UserDefinedOptions::SaveToXml(XmlWriterElement xml) const
	{
		for (auto& option : options)
		{
			if (auto pToggle = std::get_if<UserDefinedToggle>(&option))
			{
				auto element = xml.AddChild("Toggle");
				element["id"] = pToggle->id;
				element["hint"] = pToggle->hint;
				element["default"] = pToggle->defaultValue;
				element.SetValue(pToggle->defaultValue);
			}
			else if (auto pNumber = std::get_if<UserDefinedNumber>(&option))
			{
				auto element = xml.AddChild("Number");
				element["id"] = pNumber->id;
				element["hint"] = pNumber->hint;
				element["default"] = pNumber->defaultValue;
				element.SetValue(pNumber->defaultValue);
			}
			else if (auto pText = std::get_if<UserDefinedText>(&option))
			{
				auto element = xml.AddChild("Text");
				element["id"] = pText->id;
				element["hint"] = pText->hint;
				element["default"] = pText->defaultValue;
				element.SetValue(pText->defaultValue);
			}
		}
	}
}