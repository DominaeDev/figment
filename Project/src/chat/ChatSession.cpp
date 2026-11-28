#include <pch.h>
#include "chat/ChatSession.h"
#include "chat/MessagePoller.h"
#include "data/Character.h"

using namespace fig::io;
using namespace fig::data;
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
		if (is_npc(role))
			return "";
		
		if (_options.flags.IsSet(ChatOptions::Flag::UseCharacterIds))
		{
			if (is_user(role))
				return "@USR";
			else if (is_bot(role))
			{
				if (auto try_character = _staging.GetCharacterByRole(role))
					return std::format("@{}", ucase((*try_character).chatId));
				return "@UNK";
			}
		}
		return GetNameOf(role);
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
			return (*try_character).GetSpokenName();

		return fig::string { Constants::Chat::Names::Unknown };
	}

	fig::gui::color_triple ChatSession::GetColorsOf(Role role) const
	{
		return _staging.GetColorsOf(role);
	}

	fig::string ChatSession::GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const
	{
		fig::string pattern;
		int32_t botCount = _staging.GetBotCount();
		for (int32_t i = 0; i < botCount; ++i)
		{
			if (i > 0)
				pattern += " | ";
			pattern += std::format("\"{}\"", GetIdentifierOf(bot_from_index(i)));
		}
		if (bIncludeUser)
		{
			if (not pattern.empty())
				pattern += " | ";
			pattern += std::format("\"{}\"", GetIdentifierOf(Role::User));
		}
		return pattern;
	}

	fig::optional_ref<MessagePoller> ChatSession::GetPoller() noexcept
	{
		if (_messagePoller)
			return *_messagePoller;
		return fig::nullref;
	}

	fig::optional_cref<ChatLogger> ChatSession::GetLogger() const
	{
		if (_logger)
			return *_logger;
		return fig::nullref;
	}

	std::vector<fig::data::ChatMessage> ChatSession::GetMessages(size_t maxMessages) const noexcept
	{
		auto& log = _logger->GetLogMessages();
		
		if (maxMessages == 0uz)
			maxMessages = std::numeric_limits<size_t>::max();

		std::list<ChatMessage> messages;
		for (int32_t i = toI(log.size()) - 1; i >= 0; --i)
		{
			if (messages.size() >= maxMessages)
				break;

			messages.push_front(log[toUZ(i)]);
		}
		return messages
			| std::ranges::to<std::vector>();
	}

	void ChatSession::Update(float fElapsed) noexcept
	{
		if (_messagePoller)
			_messagePoller->Update(fElapsed);
	}
	
} // namespace