#include <pch.h>
#include "model/ChatSession.h"

using namespace fig::util;
using namespace fig::io;
using namespace fig::gui;

namespace fig::io
{
	void ChatSession::Initialize(const ChatStaging& staging, ChatOptions options)
	{
		_options = options;
		_staging = staging;
	}

	fig::uuid ChatSession::GetCharacterIdOf(Role role) const
	{
		if (auto try_character = _staging.GetCharacter(role))
			return try_character.value().assetId;
		return {};
	}

	fig::string ChatSession::GetIdentifierOf(Role role) const
	{
		if (role == Role::User)
			return "USR";

		if (auto try_character = _staging.GetCharacter(role))
			return ucase(try_character.value().chatId);
		return "_UNK";
	}

	fig::string ChatSession::GetNameOf(Role role) const
	{
		if (role == Role::System)
			return fig::string { Constants::Chat::Names::System };
		if (role == Role::Narrator)
			return fig::string { Constants::Chat::Names::Narrator };
		if (role == Role::Director)
			return fig::string { Constants::Chat::Names::Director };

		auto optCharacter = _staging.GetCharacter(role);
		if (optCharacter.has_value())
			return optCharacter.value().shortName;

		return fig::string { Constants::Chat::Names::Unknown };
	}

	fig::gui::ColorPair ChatSession::GetColorsOf(Role role) const
	{
		if (auto character = _staging.GetCharacter(role))
		{
			if (character.value().bgColor.IsDefined() && character.value().borderColor.IsDefined())
			{
				return ColorPair {
					.foreground = character.value().borderColor,
					.background = character.value().bgColor,
				};
			}
		}

		if (is_bot(role))
		{
			return ColorPair {
				.foreground = Colors::DefaultBotMessageBorders[get_bot_index(role) % 8],
				.background = Colors::DefaultBotMessageBackgrounds[get_bot_index(role) % 8],
			};
		}
		else if (role == Role::User)
		{
			return ColorPair {
				.foreground = Colors::DefaultUserMessageBorder,
				.background = Colors::DefaultUserMessageBackground,
			};
		}
		else if (role == Role::System)
		{
			return ColorPair {
				.foreground = Colors::MessageBorderNavy,
				.background = Colors::MessageBackgroundNavy,
			};
		}
		else
		{
			return ColorPair {
				.foreground = Colors::MessageBorderDefault,
				.background = Colors::MessageBackgroundDefault,
			};
		}
	}

	fig::string ChatSession::GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const
	{
		fig::string pattern;
		int32_t botCount = _staging.GetBotCount();
		for (int32_t i = 0; i < botCount; ++i)
		{
			if (i > 0)
				pattern += "| ";
			if (useCharacterIds)
				pattern += std::format("| \"@{}\"", GetIdentifierOf(bot_from_index(i)));
			else
				pattern += std::format("| \"{}\"", GetNameOf(bot_from_index(i)));
		}
		if (bIncludeUser)
		{
			if (useCharacterIds)
				pattern += std::format("| \"@{}\"", GetIdentifierOf(Role::User));
			else
				pattern += std::format("| \"{}\"", GetNameOf(Role::User));
		}
		return pattern;
	}

	fig::string ChatSession::ApplyNames(const fig::string& text) const
	{
		return _staging.ApplyNames(text);
	}

	fig::string ChatSession::ApplyNames(const fig::string& text, Role role) const
	{
		return _staging.ApplyNames(text, role);
	}
} // namespace