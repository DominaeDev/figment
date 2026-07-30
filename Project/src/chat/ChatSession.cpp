#include <pch.h>
#include "chat/ChatSession.h"
#include "chat/MessagePoller.h"
#include "data/Character.h"

using namespace fig::io;
using namespace fig::gui;

namespace fig::chat
{
	ChatSession::ChatSession()
	{}

	ChatSession::~ChatSession()
	{
		Shutdown();
	}

	void ChatSession::Initialize(const ChatStaging& staging, ChatOptions options, fig::uuid chatInstanceId, fig::uuid chatLogId)
	{
		if (_bInitialized)
			return;

		_options = options;
		_staging = staging;
		_staging.RefreshContext();

		_messagePoller = std::make_unique<MessagePoller>();
		_logger = std::make_unique<ChatLogger>(chatLogId, chatInstanceId, *this);
		_bInitialized = true;
	}

	void ChatSession::Shutdown()
	{
		if (_bInitialized)
		{
			_logger->Save();
			_logger.reset();
			_messagePoller.reset();
			_bInitialized = false;
		}
	}

	void ChatSession::Save()
	{
		if (_bInitialized)
			_logger->Save();
	}

	fig::uuid ChatSession::GetCharacterIdOf(Role role) const
	{
		if (auto try_id = _staging.GetCharacterIdByRole(role))
			return *try_id;
		return {};
	}

	fig::string ChatSession::GetIdentifierOf(Role role) const
	{
		if (role == Role::User)
			return "USR";

		if (auto try_character = _staging.GetCharacterByRole(role))
			return ucase((*try_character).chatId);
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

		if (auto try_character = _staging.GetCharacterByRole(role))
			return (*try_character).shortName;

		return fig::string { Constants::Chat::Names::Unknown };
	}

	fig::color_pair ChatSession::GetColorsOf(Role role) const
	{
		if (auto try_character = _staging.GetCharacterByRole(role))
		{
			auto& character = *try_character;
			if (character.bgColor.IsDefined() && character.borderColor.IsDefined())
			{
				return fig::color_pair {
					.foreground = character.borderColor,
					.background = character.bgColor,
				};
			}
		}

		return GetDefaultColorsOf(role);
	}

	fig::color_pair ChatSession::GetDefaultColorsOf(Role role)
	{
		if (is_bot(role))
		{
			return fig::color_pair {
				.foreground = Color::DefaultBotMessageBorders[get_bot_index(role) % 8],
				.background = Color::DefaultBotMessageBackgrounds[get_bot_index(role) % 8],
			};
		}
		else if (role == Role::User)
		{
			return fig::color_pair {
				.foreground = Color::DefaultUserMessageBorder,
				.background = Color::DefaultUserMessageBackground,
			};
		}
		else if (role == Role::System)
		{
			return fig::color_pair {
				.foreground = Color::MessageBorderNavy,
				.background = Color::MessageBackgroundNavy,
			};
		}
		else
		{
			return fig::color_pair {
				.foreground = Color::MessageBorderDefault,
				.background = Color::MessageBackgroundDefault,
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
				pattern += " | ";
			if (useCharacterIds)
				pattern += std::format("\"@{}\"", GetIdentifierOf(bot_from_index(i)));
			else
				pattern += std::format("\"{}\"", GetNameOf(bot_from_index(i)));
		}
		if (bIncludeUser)
		{
			if (not pattern.empty())
				pattern += " | ";
			if (useCharacterIds)
				pattern += std::format("\"@{}\"", GetIdentifierOf(Role::User));
			else
				pattern += std::format("\"{}\"", GetNameOf(Role::User));
		}
		return pattern;
	}

	fig::optional_ref<MessagePoller> ChatSession::GetPoller() noexcept
	{
		if (_messagePoller)
			return *_messagePoller;
		return nullref;
	}

	void ChatSession::Update(float fElapsed) noexcept
	{
		if (_messagePoller)
			_messagePoller->Update(fElapsed);
	}

} // namespace