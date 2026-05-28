#include <pch.h>
#include "gui/GUICommon.h"
#include "gui/ChatScroll.h"
#include "gui/ChatMessage.h"
#include "gui/GUIUtility.h"
#include "app/AppState.h"
#include "chat/ChatSession.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "data/CharacterData.h"
#include <format>
#include <set>

using namespace fig::llm;
using namespace fig::chat;

#define POLL_INTERVAL 0.1f
#define ANIMATED_SCROLL_SPEED 15.0f
#define GRADIENT_HEIGHT 40

namespace fig::gui
{
	ChatScroll::ChatScroll(LayoutElement* pParent) : Control(pParent)
	{
		_pScrollSizer = new VerticalScrollSizer();
		_pScrollSizer->SetBottomMargin(50);
		_pScrollSizer->SetSpacing(12);
		SetSizer(_pScrollSizer);

		_pBottomGradient = new VerticalGradient(this, Colors::ChatBackground.WithAlpha(0.0f), Colors::ChatBackground);
		EnableClipping(true);
		EnableCulling(true);
	}

	void ChatScroll::AddDummyMessage(StringCRef name, Role role, MessageType msgType, StringCRef message)
	{
		ChatMessage* pMessage = AddMessage({}, role, msgType, message, true);
		pMessage->SetName(name);
		pMessage->SetActive(false);
		_messages.push_back(MessageEntry {
			.chatId = "dummy",
			.role = role,
			.responseId {},
			.subMessageId {},
			.msgType = msgType,
			.pChatMessage = pMessage,
		});
	}

	void ChatScroll::AddSystemMessage(StringCRef message)
	{
		ChatMessage* pMessage = AddMessage({}, Role::System, MessageType::SystemMessage, message, true);
		pMessage->SetActive(false);
		pMessage->SetColors(Colors::MessageBackgroundNavy, Colors::MessageBorderNavy);
		_messages.push_back(MessageEntry {
			.chatId = "dummy",
			.role = Role::System,
			.responseId {},
			.subMessageId {},
			.msgType = MessageType::SystemMessage,
			.pChatMessage = pMessage,
		});
	}

	ChatMessage* ChatScroll::AddMessage(const fig::uuid& characterId, Role role, MessageType msgType, StringCRef message, bool complete)
	{
		if (msgType == MessageType::Narration)
			role = Role::Narrator;
		else if (msgType == MessageType::SystemMessage)
			role = Role::System;

		bool bShowAvatar = !is_npc(role);
		auto itLast = std::find_if(_messages.crbegin(), _messages.crend(), [](const MessageEntry& entry) {
			return entry.pChatMessage != nullptr;
		});

		Role lastRole = Role::Undefined;
		fig::uuid lastId {};
		if (itLast != _messages.crend())
		{
			if ((*itLast).msgType == MessageType::Narration)
				lastRole = Role::Narrator;
			else
			{
				lastRole = (*itLast).role;
				lastId = (*itLast).characterId;
			}
		}
		bShowAvatar &= (role != lastRole) || (characterId != lastId);
		bool bShowName = bShowAvatar;

		fig::string name = _session.GetNameOf(role);
		if (role == Role::System)
		{
			bShowName = true;
			name = "System message";
		}

		if (msgType == MessageType::Narration)
		{
			name = _session.GetNameOf(Role::Narrator);
			bShowName = true;
			bShowAvatar = false;
		}
		else if (auto try_character = _session.GetStaging().GetCharacterById(characterId))
			name = (*try_character).get().fullName;
		else
			name = "Unknown";

		auto pMessage = new ChatMessage(this, role, characterId, bShowName ? name : "", msgType, bShowAvatar);
		pMessage->SetY(-1000); // Move off-screen
		pMessage->SetMessage(message, complete);
		pMessage->SetColors(_session.GetColorsOf(role));
		GetSizer()->Add(pMessage, 0, Sizer::Expand);
		return pMessage;
	}

	int ChatScroll::RemoveMessages(std::span<fig::uuid> ids)
	{
		int removed = 0;
		std::set<fig::uuid> removedIds;
		for (int i = (int32_t)_messages.size() - 1; i >= 0; --i)
		{
			MessageEntry& entry = _messages[i];
			if (std::ranges::find(ids, entry.responseId) == ids.cend())
				continue;

			if (entry.pChatMessage)
			{
				GetSizer()->Remove(entry.pChatMessage);
				RemoveChild(entry.pChatMessage);
				delete entry.pChatMessage;
				entry.pChatMessage = nullptr;
			}

			_messagesById.erase(entry.subMessageId);
			_messages.erase(_messages.begin() + i);
			++removed;
		}

		InvalidateLayout();
		return removed;
	}

	void ChatScroll::ClearMessages()
	{
		for (auto& entry : _messages)
		{
			RemoveChild(entry.pChatMessage);
			delete entry.pChatMessage;
		}
		_messages.clear();
		_messagesById.clear();
		_fScrollY = 0.0f;
		_fAnimatedScroll = 0.0f;
		GetSizer()->Clear();
	}

	std::tuple<fig::uuid, fig::uuid> ChatScroll::GetLastMessage() const
	{
		if (_messages.empty())
			return {};
		auto& message = _messages[_messages.size() - 1];
		return std::make_tuple(message.responseId, message.subMessageId);
	}

	void ChatScroll::OnUpdate(float fElapsed)
	{
		// Poll LLM for new messages
		_fPollTimer += fElapsed;
		if (_fPollTimer >= POLL_INTERVAL)
		{
			_fPollTimer = 0.0f;
			Poll();
		}

		// Animated scrolling
		if (_fAnimatedScroll > 0.0f)
		{
			_fAnimatedScroll -= _fAnimatedScroll * fElapsed * ANIMATED_SCROLL_SPEED;
			if (_fAnimatedScroll < 1.0f)
				_fAnimatedScroll = 0.0f;
			_pScrollSizer->SetOffset(toI(_fScrollY + _fAnimatedScroll));
		}
	}

	void ChatScroll::EnablePolling(bool bEnable)
	{
		_bPolling = bEnable;
	}

	void ChatScroll::Poll()
	{
		auto pLLM = Global::GetLLMInstance();
		if (!pLLM)
			return;

		bool bNewResponse = false;

		MessagePiece piece;
		while (pLLM->PollResponse(piece))
		{
			auto itMsg = _messagesById.find(piece.subMessageId);
			if (itMsg != _messagesById.end())
			{
				// Append piece
				MessageEntry* pEntry = itMsg->second;

				if (pEntry->pChatMessage != nullptr)
					pEntry->pChatMessage->AppendMessage(piece.content, piece.isComplete);
				else if (!empty_or_whitespace(piece.content))
				{
					(*itMsg).second->role = piece.role;
					(*itMsg).second->msgType = piece.msgType;
					auto characterId = _session.GetCharacterIdOf(piece.role);
					pEntry->pChatMessage = AddMessage(characterId, piece.role, piece.msgType, piece.content, piece.isComplete);
				}
				else
					continue; // Skip until we receive some text
			}
			else if (empty_or_whitespace(piece.content) && piece.isComplete)
			{
				// Ignore complete empty messages
				continue;
			}
			else
			{
				auto characterId = _session.GetCharacterIdOf(piece.role);
				fig::string text = trim(piece.content);
				if (text.empty() || (text.length() == 1 && (text[0] == '"' || text[0] == '*' || text[0] == '['))) // Empty or scaffolding
				{
					_messages.push_back(MessageEntry {
						.characterId = characterId,
						.chatId = piece.identifier,
						.role = piece.role,
						.responseId = piece.responseId,
						.subMessageId = piece.subMessageId,
						.msgType = piece.msgType,
						.pChatMessage = nullptr,
					});
					_messagesById[piece.subMessageId] = &_messages.back();
				}
				else
				{

					// Create new message to hold the piece
					ChatMessage* pMessage = AddMessage(characterId, piece.role, piece.msgType, piece.content, piece.isComplete);
					_messages.push_back(MessageEntry {
						.characterId = characterId,
						.chatId = piece.identifier,
						.role = piece.role,
						.responseId = piece.responseId,
						.subMessageId = piece.subMessageId,
						.msgType = piece.msgType,
						.pChatMessage = pMessage,
					});
					_messagesById[piece.subMessageId] = &_messages.back();
				}
			}

			// Clean up empty
			if (piece.isComplete && itMsg != _messagesById.end() && itMsg->second->pChatMessage == nullptr)
			{
				_messagesById.erase(itMsg);
				for (int i = (int32_t)_messages.size() - 1; i >= 0; --i)
				{
					if (_messages[i].subMessageId == itMsg->first)
						_messages.erase(_messages.begin() + (ptrdiff_t)i);
				}
				continue;
			}
			bNewResponse |= piece.isComplete;
		}

		if (bNewResponse)
			RefreshActive();
	}

	EventResult ChatScroll::OnEvent(Event& event)
	{
		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			return HandleMouseWheel(event.wheel) ? EventResult::Handled : EventResult::Pass;
		}
		return EventResult::Pass;
	}

	bool ChatScroll::HandleMouseWheel(SDL_MouseWheelEvent event)
	{
		auto& rect = GetRect();
		Point pt = { toI(event.mouse_x), toI(event.mouse_y) };
		if (!SDL_PointInRect(&pt, &rect))
			return false;

		_fScrollY = std::max(_fScrollY + toF(event.integer_y) * Constants::GUI::MouseScrollSpeed, 0.0f);
		_pScrollSizer->SetOffset(toI(_fScrollY + _fAnimatedScroll));
		return true;
	}

	void ChatScroll::OnAfterLayout()
	{
		float listHeight = _pScrollSizer->GetListHeight();
		if (listHeight > _fLastListHeight) // Near bottom
		{
			// Animate scroll
			_fAnimatedScroll += (listHeight - _fLastListHeight);
			_pScrollSizer->SetOffset(toI(_fAnimatedScroll));
		}
		_fLastListHeight = listHeight;

		_pBottomGradient->SetPosition(0, GetHeight() - GRADIENT_HEIGHT);
		_pBottomGradient->SetSize(GetWidth(), GRADIENT_HEIGHT);
	}

	void ChatScroll::OnAddedChild(LayoutElement* pChild)
	{
		MoveChildToTop(_pBottomGradient);
	}

	void ChatScroll::RefreshActive()
	{
		auto pLLM = Global::GetLLMInstance();
		if (!pLLM)
			return;

		// Check which messages are present in the context
		auto activeMessages = pLLM->GetActiveMessages();

		for (auto& message : _messages)
		{
			if (!message.pChatMessage)
				continue;

			bool bActive = message.responseId.empty()
				|| activeMessages.find(message.responseId) != activeMessages.end();
			message.pChatMessage->SetActive(bActive);
		}
	}
}