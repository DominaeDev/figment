#include <pch.h>
#include "model/ScenarioData.h"
#include "util/StringUtility.h"
#include "fs/Xml.h"
#include <cassert>

using namespace fig::util;

namespace fig::io
{
	static constexpr std::array<std::pair<const char*, ScenarioData::PromptType>, 6> elements {
		std::pair { "System",				ScenarioData::PromptType::System },
		std::pair { "Scenario",				ScenarioData::PromptType::Scenario },
		std::pair { "InitialMessage",		ScenarioData::PromptType::FirstMessage },
		std::pair { "InitialUserMessage",	ScenarioData::PromptType::UserMessage },
		std::pair { "InitialInstruction",	ScenarioData::PromptType::Instruction },
		std::pair { "InitialNarration",		ScenarioData::PromptType::Narration },
	};

	static bool ReadXml(XmlReaderElement& node, ScenarioData::Prompt& prompt) noexcept
	{
		auto prompt_type = node.GetElementText("Type").value_or("");
		if (prompt_type.empty())
			return false;

		auto itType = std::find_if(elements.cbegin(), elements.cend(), [&prompt_type](auto& e) { return std::strcmp(e.first, prompt_type.c_str()) == 0; });
		if (itType == elements.cend())
			return false; // Unknown type
		
		prompt.type = itType->second;
		prompt.value = node.GetElementText("Value").value_or("");
		prompt.condition = node.GetElementText("Rule").value_or("");

		if (prompt.type == ScenarioData::PromptType::System)
			prompt.is_static = true;
		else
		{
			bool implicit_static;
			switch (prompt.type)
			{
			case ScenarioData::PromptType::Scenario:
				implicit_static = true;
				break;
			default:
				implicit_static = false;
				break;
			};

			prompt.is_static = node.GetElementBool("Static").value_or(implicit_static);
		}
		return not prompt.value.empty();
	}

	static bool ReadXml(XmlReader& xml, ScenarioData& scenario) noexcept
	{
		auto rootNode = xml.GetRootElement();

		// Identifier
		scenario.title = trim(rootNode.GetElementText("Title").value_or(""));

		// Image
		scenario.imageFilename = trim(rootNode.GetElementText("Image").value_or(""));

		// Prompt(s)
		if (auto node = rootNode.GetFirstElement("Prompt"))
		{
			while (node.has_value())
			{
				ScenarioData::Prompt prompt {};
				if (ReadXml(node.value(), prompt))
					scenario.prompts.emplace_back(prompt);
				node = node.value().GetNextSibling();
			}
		}

		// Roles
		if (auto roleNode = rootNode.GetFirstElement("Role"); roleNode.has_value())
		{
			while (roleNode.has_value())
			{
				auto& role = roleNode.value();
				ScenarioData::RoleSlot slot {
					.id = role.GetElementText("ID").value_or(""),
					.label = role.GetElementText("Label").value_or(""),
					.relationship = role.GetElementText("Relationship").value_or(""),
					.is_required = role.GetElementBool("Required").value_or(false),
					.is_user = role.GetElementBool("User").value_or(false),
				};

				if (auto validationNode = roleNode.value().GetFirstElement("Validation"); validationNode.has_value())
				{
					slot.validation = ScenarioData::RoleValidation {
						.rule = validationNode.value().GetElementText("Rule").value_or(""),
						.errorMessage = validationNode.value().GetElementText("ErrorMessage").value_or(""),
					};
				}

				if (slot.is_valid())
					scenario.role_slots.emplace_back(slot);

				roleNode = roleNode.value().GetNextSibling();
			}
		}

		return scenario.is_valid();
	}

	FileError ScenarioData::LoadFromXml(const fig::path& path)
	{
		if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
			return FileError::FileNotFound;

		XmlReader xml(path, "Scenario");
		if (not xml.IsOk())
			return FileError::UnrecognizedFormat; // Invalid document type

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError ScenarioData::LoadFromXml(const fig::string& doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRootElement().GetName() != "Scenario")
			return FileError::UnrecognizedFormat; // Invalid document type

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	void ScenarioData::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml("Scenario");

		auto root = xml.GetRoot();
		if (not title.empty())
			root.SetElementValue("Title", title);

		// Roles
		for (auto& role : role_slots)
		{
			auto roleNode = root.AddChild("Role");
			roleNode.SetElementValue("ID", role.id);
			if (not role.label.empty())
				roleNode.SetElementValue("Label", role.label);
			if (not role.relationship.empty())
				roleNode.SetElementValue("Relationship", role.relationship);
			roleNode.SetElementValue("Required", true);
			roleNode.SetElementValue("User", true);

			if (not role.validation.rule.empty())
			{
				auto validationNode = roleNode.AddChild("Validation");
				validationNode.SetElementValue("Rule", role.validation.rule);
				validationNode.SetElementValue("ErrorMessage", role.validation.errorMessage);
			}
		}

		auto fnEnumName = [](ScenarioData::PromptType type) -> fig::string {
			auto itType = std::find_if(elements.cbegin(), elements.cend(), [&type](auto& e) { return e.second == type; });
			if (itType != elements.cend())
				return itType->first;
			assert(false && "Unknown type");
			return "";
		};

		// Prompts
		for (auto& prompt : prompts)
		{
			if (prompt.value.empty())
				continue;

			auto promptNode = root.AddChild("Prompt");
			promptNode.SetElementValue("Type", fnEnumName(prompt.type));
			promptNode.SetElementValue("Static", prompt.is_static);
			if (prompt.is_conditional())
				promptNode.SetElementValue("Rule", prompt.condition);
			promptNode.SetElementValue("Value", prompt.value);
		}

		xml.SaveToMemory(buffer);
	}

	fig::string ScenarioData::GetSystemPrompt(ChatOptions options) const
	{
		return "";
	}

	fig::string ScenarioData::GetScenarioPrompt(ChatOptions options) const
	{
		return "";
	}

	constexpr bool ScenarioData::is_valid() const
	{
		if (prompts.empty() or title.empty() or role_slots.empty())
			return false;
		
		// Ensure the scenario can assign roles to at least one bot and one user
		int32_t idx_bot { -1 };
		int32_t idx_user { -1 };
		for (size_t i = 0; i < role_slots.size(); ++i)
		{
			if (idx_user < 0 and role_slots[i].is_user)
			{
				idx_user = static_cast<int32_t>(i);
				continue;
			}
			if (idx_bot < 0 and i != idx_user)
			{
				idx_bot = static_cast<int32_t>(i);
				continue;
			}
		}

		if (idx_bot == -1 or idx_user == -1)
			return false; // No exclusive roles
		return true;
	}
}