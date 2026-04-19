#ifndef SCENARIO_DATA_H__
#define SCENARIO_DATA_H__
#pragma once

#include "Types.h"
#include "model/ChatOptions.h"

namespace fig::io
{
	class ScenarioData
	{
	public:
		fig::string GetSystemPrompt(ChatOptions options) const;
		fig::string GetScenarioPrompt(ChatOptions options) const;

		FileError LoadFromXml(const fig::path& filename);
		FileError LoadFromXml(const fig::string& doc);
		void SaveToXml(fig::bytes& buffer) const;

		constexpr bool is_valid() const;

	public:
		enum class PromptType {
			Undefined,
			System,
			Scenario,
			FirstMessage,	// <IntroMessage>
			Narration,		// <IntroNarration>
			Instruction,	// <IntroInstruction>
			UserMessage,	// <IntroUserMessage>
		};

		struct Prompt
		{
			PromptType type {};
			fig::string condition {};
			fig::string value {};
			bool is_static {};

			constexpr bool is_conditional() const { return not condition.empty(); }
		};

		fig::string title;
		fig::string description;
		fig::string imageFilename; //! @temp
		std::vector<Prompt> prompts;

		struct RoleValidation
		{
			fig::string rule;
			fig::string errorMessage;
		};

		struct RoleSlot
		{
			fig::string id;
			fig::string label;
			fig::string relationship;
			RoleValidation validation {};
			bool is_required {};	// Must hold a character?
			bool is_user {};		// Available to the user?

			constexpr bool is_valid() const
			{
				return not (id.empty() or label.empty());
			}
		};
		std::vector<RoleSlot> role_slots {};
	};
}

#endif