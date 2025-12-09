module;

#include <SDL3/SDL.h>

export module GUI.Controls.ChatScroll;
export import GUI.Control;

import Common;
import GUI.Controls.ChatMessage;
import GUI.Controls.VerticalGradient;

import LLMTypes;
import LLMInstance;
import ChatSession;
import Utility;

import AppState;

export
{
	class ChatScroll : public Control
	{
	public:
		ChatScroll(Control* pParent);

		void SetSession(ChatSession session) { _session = session; }

		void AddDummyMessage(string name, Role role, MessageType msgType, string message);
		void AddSystemMessage(string message);

		int RemoveMessages(std::vector<string> ids);
		void ClearMessages();

		std::tuple<std::string, std::string> GetLastMessage() const;

	protected:
		void OnUpdate(float fDeltaTime) override;
		bool OnEvent(SDL_Event* event) override;
		void OnAfterLayout() override;
		void OnAddedChild(LayoutElement* pChild) override;

	private:
		ChatMessage* AddMessage(string name, Role role, MessageType msgType, string message, bool complete);
		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void EnablePolling(bool bEnable);
		void Poll();
		void RefreshActive();

	private:
		Control* _pBottomGradient;
		bool _bPolling = true;
		float _fPollTimer = 0.0f;

		struct MessageEntry
		{
			string characterId;
			Role role;
			string responseId;
			string subMessageId;
			MessageType msgType;
			ChatMessage* pChatMessage;
		};
		std::vector<MessageEntry> _messages {};
		std::map<string, MessageEntry*> _messagesById {}; // Sub-message id

		// Scrolling
		VerticalScrollSizer* _pScrollSizer;
		float _fScrollY = 0.0f;
		float _fLastListHeight = 0.0f;
		float _fAnimatedScroll = 0.0f;

		ChatSession _session {};
	};
}

constexpr float POLL_INTERVAL = 0.1f;
constexpr float ANIMATED_SCROLL_SPEED = 15.0f;
constexpr float GRADIENT_HEIGHT = 40.0f;
constexpr float MOUSE_SCROLL_SPEED = 80.0f;

ChatScroll::ChatScroll(Control* pParent) : Control(pParent)
{
	_pScrollSizer = new VerticalScrollSizer();
	_pScrollSizer->SetBottomMargin(50);
	_pScrollSizer->SetSpacing(12);
	SetSizer((Sizer*)_pScrollSizer);

	_pBottomGradient = new VerticalGradient(this, color_util::with_alpha(Colors::ChatBackground, 0), Colors::ChatBackground);
	EnableClipping(true);
	EnableCulling(true);
}


void ChatScroll::AddDummyMessage(string name, Role role, MessageType msgType, string message)
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

void ChatScroll::AddSystemMessage(string message)
{
	ChatMessage* pMessage = AddMessage("", Role::System, MessageType::SystemMessage, message, true);
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

ChatMessage* ChatScroll::AddMessage(string identifier, Role role, MessageType msgType, string message, bool complete)
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
	string lastId = "";
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

	string id = identifier;
	string name = _session.GetNameOf(role);
	if (role == Role::System)
	{
		bShowName = true;
		name = "System message";
	}

	if (auto character = _session.GetCharacterById(identifier))
	{
		id = character.value().id;
		name = character.value().fullName;
	}
	else if (auto character = _session.GetCharacterByName(identifier))
	{
		id = character.value().id;
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

int ChatScroll::RemoveMessages(std::vector<string> ids)
{
	int removed = 0;
	std::set<string> removedIds;
	for (int i = (int32_t)_messages.size() - 1; i >= 0; --i)
	{
		MessageEntry& entry = _messages[i];
		if (std::find(ids.cbegin(), ids.cend(), entry.responseId) == ids.cend())
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

std::tuple<std::string, std::string> ChatScroll::GetLastMessage() const
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
	auto pLLM = Application::GetLLM();
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
			else if (!string_util::empty_or_whitespace(piece.content))
			{
				(*itMsg).second->role = piece.role;
				(*itMsg).second->msgType = piece.msgType;
				pEntry->pChatMessage = AddMessage(piece.identifier, piece.role, piece.msgType, piece.content, piece.isComplete);
			}
			else
				continue; // Skip until we receive some text
		}
		else if (string_util::empty_or_whitespace(piece.content) && piece.isComplete)
		{
			// Ignore complete empty messages
			continue;
		}
		else
		{
			string text = string_util::trim(piece.content);
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

bool ChatScroll::OnEvent(SDL_Event* event)
{
	if (event->type == SDL_EVENT_MOUSE_WHEEL)
	{
		return HandleMouseWheel(event->wheel);
	}
	return false;
}

bool ChatScroll::HandleMouseWheel(SDL_MouseWheelEvent event)
{
	Pointf pt = { event.mouse_x, event.mouse_y };
	if (!SDL_PointInRectFloat(&pt, &_rect))
		return false;

	_fScrollY = std::max(_fScrollY + toF(event.integer_y) * MOUSE_SCROLL_SPEED, 0.0f);
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
	auto pLLM = Application::GetLLM();
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