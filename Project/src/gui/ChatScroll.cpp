#include "gui/ChatScroll.h"
#include "gui/VerticalScrollSizer.h"
#include "gui/VerticalGradient.h"
#include "gui/ChatMessage.h"
#include "gui/Color.h"
#include "model/AppState.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include <format>
#include <set>

#define POLL_INTERVAL 0.1f
#define ANIMATED_SCROLL_SPEED 15.0f
#define GRADIENT_HEIGHT 40.0f
#define MOUSE_SCROLL_SPEED 80.0f

ChatScroll::ChatScroll(Control* pParent) : Control(pParent)
{
	_pScrollSizer = new VerticalScrollSizer();
	_pScrollSizer->SetBottomMargin(50);
	_pScrollSizer->SetSpacing(12);
	SetSizer(_pScrollSizer);

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

ChatMessage* ChatScroll::AddMessage(string identifier, Role role, MessageType msgType, string message, bool complete)
{
	if (msgType == MessageType::Narration)
		role = Role::Narrator;

	bool bShowAvatar = !is_npc(role);
	auto itLast = std::find_if(std::crbegin(_messages), std::crend(_messages), [](const MessageEntry& entry) {
		return entry.pChatMessage != nullptr;
	});
	
	Role lastRole = Role::Undefined;
	string lastId = "";
	if (itLast != std::crend(_messages))
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
	
	string name = "Unknown";
	if (_pSession)
	{
		if (auto character = _pSession->GetCharacterById(identifier))
			name = character.value().name;
	}
	bool bShowName = (role != lastRole) || (identifier != lastId);
	if (msgType == MessageType::Narration)
		name = _pSession->GetNameOf(Role::Narrator);

	auto pMessage = new ChatMessage(this, role, identifier, bShowName ? name : "", msgType, bShowAvatar);
	pMessage->SetY(-1000); // Move off-screen
	pMessage->SetMessage(message, complete);
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
		if (std::find(std::cbegin(ids), std::cend(ids), entry.responseId) == std::cend(ids))
			continue;

		if (entry.pChatMessage)
		{
			GetSizer()->Remove(entry.pChatMessage);
			RemoveChild(entry.pChatMessage);
			delete entry.pChatMessage;
			entry.pChatMessage = nullptr;
		}

		removedIds.insert(entry.subMessageId);
		_messages.erase(std::begin(_messages) + (ptrdiff_t)i);
		++removed;
	}

	for (auto id : removedIds)
		_messagesById.erase(id);

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
		if (itMsg != std::end(_messagesById))
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
		if (piece.isComplete && itMsg != std::end(_messagesById) && itMsg->second->pChatMessage == nullptr)
		{
			_messagesById.erase(itMsg);
			for (int i = (int32_t)_messages.size() - 1; i >= 0; --i)
			{
				if (_messages[i].subMessageId == itMsg->first)
					_messages.erase(std::begin(_messages) + (ptrdiff_t)i);
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
			|| activeMessages.find(message.responseId) != std::end(activeMessages);
		message.pChatMessage->SetActive(bActive);
	}
}