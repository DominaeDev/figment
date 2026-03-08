#include <pch.h>
#include "gui/GUICommon.h"
#include "gui/ChatScroll.h"
#include "gui/ChatMessage.h"
#include "gui/GUIUtility.h"
#include "model/AppState.h"
#include "model/ChatSession.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include <format>
#include <set>

using namespace fig::gui;
using namespace fig::gui::util;
using namespace fig::util;
using namespace fig::llm;
using namespace fig::io::data;

#define POLL_INTERVAL 0.1f
#define ANIMATED_SCROLL_SPEED 15.0f
#define GRADIENT_HEIGHT 40.0f

ChatScroll::ChatScroll(LayoutElement* pParent) : Control(pParent)
{
	_pScrollSizer = new VerticalScrollSizer();
	_pScrollSizer->SetBottomMargin(50);
	_pScrollSizer->SetSpacing(12);
	SetSizer(_pScrollSizer);

	_pBottomGradient = new VerticalGradient(this, with_alpha(Colors::ChatBackground, 0), Colors::ChatBackground);
	EnableClipping(true);
	EnableCulling(true);
}

void ChatScroll::AddDummyMessage(fig::string name, Role role, MessageType msgType, fig::string message)
{
	ChatMessage* pMessage = AddMessage(name, role, msgType, message, true);
	pMessage->SetActive(false);
	_messages.push_back(MessageEntry {
		"dummy",
		role,
		"",
		"",
		msgType,
		pMessage,
	});
}

void ChatScroll::AddSystemMessage(fig::string message)
{
	ChatMessage* pMessage = AddMessage("", Role::System, MessageType::SystemMessage, message, true );
	pMessage->SetActive(false);
	pMessage->SetColors(Colors::MessageBackgroundNavy, Colors::MessageBorderNavy);
	_messages.push_back(MessageEntry {
		"dummy",
		Role::System,
		"",
		"",
		MessageType::SystemMessage,
		pMessage,
	});
}

ChatMessage* ChatScroll::AddMessage(fig::string identifier, Role role, MessageType msgType, fig::string message, bool complete)
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
	fig::string lastId = "";
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
	bShowAvatar &= (role != lastRole) || (identifier != lastId);
	bool bShowName = bShowAvatar;
	
	fig::string id = identifier;
	fig::string name = _session.GetNameOf(role);
	if (role == Role::System)
	{
		bShowName = true;
		name = "System message";
	}

	if (auto character = _session.GetCharacterById(identifier))
	{
		id = character.value().characterId;
		name = character.value().fullName;
	}
	else if (auto character = _session.GetCharacterByName(identifier))
	{
		id = character.value().characterId;
		name = character.value().fullName;
	}
	
	if (msgType == MessageType::Narration)
	{
		name = _session.GetNameOf(Role::Narrator);
		bShowName = true;
		bShowAvatar = false;
	}

	auto pMessage = new ChatMessage(this, role, id, bShowName ? name : "", msgType, bShowAvatar);
	pMessage->SetY(-1000); // Move off-screen
	pMessage->SetMessage(message, complete);
	pMessage->SetColors(_session.GetColorsOf(role));
	GetSizer()->Add(pMessage, 0, Sizer::Expand);
	return pMessage;
}

int ChatScroll::RemoveMessages(std::span<fig::string> ids)
{
	int removed = 0;
	std::set<fig::string> removedIds;
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
	for (auto entry : _messages)
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

std::tuple<fig::string, fig::string> ChatScroll::GetLastMessage() const
{
	if (_messages.empty())
		return {};
	auto& message = _messages[_messages.size() - 1];
	return std::make_tuple(message.responseId, message.subMessageId);
}

void ChatScroll::OnUpdate(float fDeltaTime)
{
	// Poll LLM for new messages
	_fPollTimer += fDeltaTime;
	if (_fPollTimer >= POLL_INTERVAL)
	{
		_fPollTimer = 0.0f;
		Poll();
	}

	// Animated scrolling
	if (_fAnimatedScroll > 0.0f)
	{
		_fAnimatedScroll -= _fAnimatedScroll * fDeltaTime * ANIMATED_SCROLL_SPEED;
		if (_fAnimatedScroll < 1.0f)
			_fAnimatedScroll = 0.0f;
		_pScrollSizer->SetOffset(_fScrollY + _fAnimatedScroll);
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
				pEntry->pChatMessage = AddMessage(piece.identifier, piece.role, piece.msgType, piece.content, piece.isComplete);
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
			fig::string text = trim(piece.content);
			if (text.empty() || (text.length() == 1 && (text[0] == '"' || text[0] == '*' || text[0] == '['))) // Empty or scaffolding
			{
				_messages.push_back(MessageEntry {
					piece.identifier,
					piece.role,
					piece.responseId,
					piece.subMessageId,
					piece.msgType,
					nullptr,
				});
				_messagesById[piece.subMessageId] = &_messages.back();
			}
			else
			{
				// Create new message to hold the piece
				ChatMessage* pMessage = AddMessage(piece.identifier, piece.role, piece.msgType, piece.content, piece.isComplete);
				_messages.push_back(MessageEntry {
					piece.identifier,
					piece.role,
					piece.responseId,
					piece.subMessageId,
					piece.msgType,
					pMessage,
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

bool ChatScroll::OnEvent(Event& event)
{
	if (event.type == SDL_EVENT_MOUSE_WHEEL)
	{
		return HandleMouseWheel(event.wheel);
	}
	return false;
}

bool ChatScroll::HandleMouseWheel(SDL_MouseWheelEvent event)
{
	Pointf pt = { event.mouse_x, event.mouse_y };
	if (!SDL_PointInRectFloat(&pt, &_rect))
		return false;

	_fScrollY = std::max(_fScrollY + toF(event.integer_y) * Constants::GUI::MouseScrollSpeed, 0.0f);
	_pScrollSizer->SetOffset(_fScrollY + _fAnimatedScroll);
	return true;
}

void ChatScroll::OnAfterLayout()
{
	float listHeight = _pScrollSizer->GetListHeight();
	if (listHeight > _fLastListHeight) // Near bottom
	{
		// Animate scroll
		_fAnimatedScroll += (listHeight - _fLastListHeight);
		_pScrollSizer->SetOffset(_fAnimatedScroll);
	}
	_fLastListHeight = listHeight;

	_pBottomGradient->SetRect(0, GetHeight() - GRADIENT_HEIGHT, GetWidth(), GRADIENT_HEIGHT);
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