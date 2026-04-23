#pragma once

#include "llm/ChatStaging.h"

namespace fig::io
{
	class ChatSession
	{
	public:
		void Initialize(const ChatStaging& staging, ChatOptions options);	

		inline const ChatStaging& GetStaging() const noexcept { return _staging; }

		fig::string GetIdentifierOf(Role role) const;
		fig::string GetNameOf(Role role) const;
		fig::string GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const;
		fig::gui::ColorPair GetColorsOf(Role role) const;
		bool IsGroupChat() const noexcept;

		[[nodiscard]] fig::string ApplyNames(const fig::string& text) const;
		[[nodiscard]] fig::string ApplyNames(const fig::string& text, Role role) const;

	protected:
		ChatStaging _staging {};
		ChatOptions _options {};
	};
}