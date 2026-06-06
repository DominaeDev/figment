#pragma once

#include "chat/ChatStaging.h"

namespace fig::chat
{
	class ChatSession
	{
	public:
		void Initialize(const ChatStaging& staging, ChatOptions options);	

		inline const ChatStaging& GetStaging() const noexcept { return _staging; }
		inline ChatStaging& GetStaging() noexcept { return _staging; }

		fig::uuid GetCharacterIdOf(Role role) const;
		fig::string GetIdentifierOf(Role role) const;
		fig::string GetNameOf(Role role) const;
		fig::string GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const;
		fig::gui::ColorPair GetColorsOf(Role role) const;

		inline const Context& GetContext() const noexcept { return _context; }

	protected:
		ChatStaging _staging {};
		ChatOptions _options {};
		Context _context {};
	};
}