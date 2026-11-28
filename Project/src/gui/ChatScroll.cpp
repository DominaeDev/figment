#include <pch.h>
#include "gui/GUICommon.h"
#include "gui/ChatScroll.h"
#include "gui/ChatMessage.h"
#include "gui/GUIUtility.h"
#include "app/AppState.h"
#include "chat/ChatSession.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "data/Character.h"
#include "audio/AudioManager.h"
#include "user/UserSettings.h"
#include "tts/TTSBackend.h"

using namespace fig::llm;
using namespace fig::chat;

static constexpr float kAnimatedScrollSpeed = 15.0f;
static constexpr fig::coord kGradientHeight = 36;

namespace fig::gui
{
	ChatScroll::ChatScroll(ControlPtr pParent) : RenderTargetControl(pParent)
	{
		_pScrollSizer = SetSizer<VerticalScrollSizer>();
		_pScrollSizer->SetBottomMargin(50);
		_pScrollSizer->SetSpacing(12);

		_pBottomGradient = std::make_unique<VerticalGradient>(nullptr, Color::White, fig::color_ref(Color::White).WithAlpha(0.0f));
		_pBottomGradient->SetTexture(nullptr);
		EnableClipping(true);
		EnableCulling(true);

		_audioResultQueue.SetDelegate([this](auto&& r) { OnAudioResult(std::move(r)); });
	}

	void ChatScroll::SetSession(std::weak_ptr<fig::chat::ChatSession> wpSession)
	{ 
		_pSession = wpSession;

		// Register callback
		if (auto pSession = wpSession.lock())
		{
			pSession->GetPoller()->RegisterObserver(std::bind_front(&ChatScroll::OnMessage, this), std::bind_front(&ChatScroll::OnRemovedMessage, this));

			// Add messages...
			if (auto pLogger = pSession->GetLogger())
			{
				auto& messages = pLogger->GetLogMessages();
				for (auto& message : messages)
				{
					bool bShowAvatar = message.subTurn <= 0 and (is_bot(message.role) or is_user(message.role));
					auto pMessage = AddMessage(message.speakerId, message.role, message.msgType, message.content, bShowAvatar, true);
					pMessage->SetName(pSession->GetNameOf(message.role));
					_messages.push_back(MessageEntry {
						.chatId = pSession->GetIdentifierOf(message.role),
						.role = message.role,
						.responseId = message.responseId,
						.subMessageId = message.subMessageId,
						.msgType = message.msgType,
						.pChatMessage = pMessage,
					});
				}
			}
		}
	}

	void ChatScroll::AddDummyMessage(string_cref name, Role role, MessageType msgType, string_cref message)
	{
		ChatMessage* pMessage = AddMessage({}, role, msgType, message, is_bot(role) or is_user(role), true);
		pMessage->SetName(name);
		_messages.push_back(MessageEntry {
			.chatId = "dummy",
			.role = role,
			.responseId {},
			.subMessageId {},
			.msgType = msgType,
			.pChatMessage = pMessage,
		});
	}

	void ChatScroll::AddSystemMessage(string_cref message)
	{
		ChatMessage* pMessage = AddMessage({}, Role::System, MessageType::SystemMessage, message, false, true);
		pMessage->SetActive(false);
		pMessage->SetColors(Color::MessageBackgroundNavy, Color::MessageForegroundNavy, Color::MessageBorderNavy);
		_messages.push_back(MessageEntry {
			.chatId = "dummy",
			.role = Role::System,
			.responseId {},
			.subMessageId {},
			.msgType = MessageType::SystemMessage,
			.pChatMessage = pMessage,
		});
	}

	ChatMessage* ChatScroll::AddMessage(const fig::uuid& characterId, Role role, MessageType msgType, string_cref message, bool bShowAvatar, bool bIsComplete)
	{
		if (msgType == MessageType::Narration)
			role = Role::Narrator;
		else if (msgType == MessageType::SystemMessage)
			role = Role::System;

		auto itLast = std::find_if(_messages.crbegin(), _messages.crend(), [](const MessageEntry& entry) {
			return entry.pChatMessage != nullptr;
		});

		if (auto pSession = _pSession.lock())
		{
			auto& session = *pSession;

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
			bool bShowName = bShowAvatar;

			fig::string name = session.GetNameOf(role);
			if (role == Role::System)
			{
				bShowName = true;
				name = "System message";
			}

			if (msgType == MessageType::Narration)
			{
				name = session.GetNameOf(Role::Narrator);
				bShowName = true;
				bShowAvatar = false;
			}
			else if (auto try_character = session.GetStaging().GetCharacterById(characterId))
				name = (*try_character).name.GetFullName();
			else
				name = "Unknown";

			auto pMessage = CreateControl<ChatMessage>(role, characterId, bShowName ? name : "", msgType, bShowAvatar);
			pMessage->SetY(-1000); // Move off-screen
			pMessage->SetMessage(message, bIsComplete);
			pMessage->SetColors(session.GetColorsOf(role));
			GetSizer()->Add(pMessage, 0, SizerFlag::Expand);
			return pMessage;
		}
		else
		{
			auto pMessage = CreateControl<ChatMessage>(role, characterId, "", msgType, bShowAvatar);
			pMessage->SetY(-1000); // Move off-screen
			pMessage->SetMessage(message, bIsComplete);
			pMessage->SetColors(color_triple { Color::MessageBackgroundDefault, Color::MessageForegroundDefault, Color::MessageBorderDefault });
			GetSizer()->Add(pMessage, 0, SizerFlag::Expand);
			return pMessage;
		}
	}

	int32_t ChatScroll::RemoveMessages(const std::vector<fig::uuid>& msgIds)
	{
		int32_t removed = 0;
		std::set<fig::uuid> removedIds;

		auto fnDestroyMessage = [this](std::vector<MessageEntry>::const_iterator itWhere) {
			const MessageEntry& entry = *itWhere;

			if (entry.pChatMessage)
			{
				GetSizer()->Remove(entry.pChatMessage);
				RemoveChild(entry.pChatMessage);
				delete entry.pChatMessage;
			}

			Global::GetAudioManager().StopSound(entry.audioId);

			_messagesById.erase(entry.subMessageId);
			_messages.erase(itWhere);
		};

		for (auto& msgId : msgIds)
		{
			for (;;)
			{
				if (auto itFind = std::ranges::find(_messages, msgId, [](auto&& m) { return m.responseId; }); itFind != std::ranges::end(_messages))
				{
					fnDestroyMessage(itFind);
					++removed;
					continue;
				}
				break;
			}
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
		_audioResultQueue.Update();

		if (_fAnimatedScroll > 0.0f)
		{
			_fAnimatedScroll -= _fAnimatedScroll * fElapsed * kAnimatedScrollSpeed;
			if (_fAnimatedScroll < 1.0f)
				_fAnimatedScroll = 0.0f;
			_pScrollSizer->SetOffset(toI(_fScrollY + _fAnimatedScroll));
		}
	}

	EventResult ChatScroll::OnEvent(fig::event& event)
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
		fig::point pt = { toI(event.mouse_x), toI(event.mouse_y) };
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

		_pBottomGradient->SetPosition(0, GetHeight() - kGradientHeight);
		_pBottomGradient->SetSize(GetWidth(), kGradientHeight);
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

	void ChatScroll::OnMessage(const MessagePoller::Message& piece)
	{
		// Ignore empty messages
		if (empty_or_whitespace(piece.content) && piece.complete)
			return;

		bool bShowAvatar = piece.subTurn <= 0 and (is_bot(piece.role) or is_user(piece.role));

		if (auto pSession = _pSession.lock())
		{
			auto itMsg = _messagesById.find(piece.subMessageId);
			if (itMsg != _messagesById.end())
			{
				// Append piece
				MessageEntry* pEntry = itMsg->second;

				if (pEntry->pChatMessage != nullptr)
				{
					// Update bubble
					pEntry->pChatMessage->SetMessage(piece.content, piece.complete);
				}
				else if (!empty_or_whitespace(piece.content))
				{
					// New bubble
					(*itMsg).second->role = piece.role;
					(*itMsg).second->msgType = piece.msgType;
					auto characterId = pSession->GetCharacterIdOf(piece.role);
					pEntry->pChatMessage = AddMessage(characterId, piece.role, piece.msgType, piece.content, bShowAvatar, piece.complete);
				}
			}
			else
			{
				auto characterId = pSession->GetCharacterIdOf(piece.role);
				auto identifier = pSession->GetIdentifierOf(piece.role);
				fig::string text = trim(piece.content);
				if (text.empty() || (text.length() == 1 && (text[0] == '"' || text[0] == '*' || text[0] == '['))) // Empty or scaffolding
				{
					_messages.emplace_back(MessageEntry {
						.characterId = characterId,
						.chatId = identifier,
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
					ChatMessage* pMessage = AddMessage(characterId, piece.role, piece.msgType, piece.content, bShowAvatar, piece.complete);
					_messages.emplace_back(MessageEntry {
						.characterId = characterId,
						.chatId = identifier,
						.role = piece.role,
						.responseId = piece.responseId,
						.subMessageId = piece.subMessageId,
						.msgType = piece.msgType,
						.pChatMessage = pMessage,
					});
					_messagesById[piece.subMessageId] = &_messages.back();
				}
			}

			if (piece.complete)
			{
				RefreshActive();

				// Enqueue TTS
				if (piece.msgType == MessageType::Dialogue and piece.allowTTS and is_bot(piece.role))
				{
					auto characterId = pSession->GetCharacterIdOf(piece.role);
					if (auto results = Global::GetTTSBackend().Speak(characterId, piece.content, piece.subMessageId))
					{
						for (auto& result : results.value())
							_audioResultQueue.Add(std::move(result));
					}
				}
			}
		}
	}

	void ChatScroll::OnRemovedMessage(const MessagePoller::RemovedMessage& removed)
	{
		RemoveMessages({ removed.responseId });
	}

	void ChatScroll::OnRenderMask(fig::renderer_ptr pRenderer, fig::sdl::Texture& texture)
	{
		SDL_BlendMode blendMode;
		SDL_GetRenderDrawBlendMode(pRenderer, &blendMode);

		SDL_BlendMode writeAlpha = SDL_ComposeCustomBlendMode(
			SDL_BLENDFACTOR_ZERO, 
			SDL_BLENDFACTOR_SRC_ALPHA, 
			SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_DST_ALPHA, 
			SDL_BLENDFACTOR_ZERO, 
			SDL_BLENDOPERATION_ADD);
		
		SDL_SetRenderDrawBlendMode(pRenderer, writeAlpha);
		_pBottomGradient->OnRender(pRenderer);
		SDL_SetRenderDrawBlendMode(pRenderer, blendMode); //! @todo: Do this rigorously before every render instead
	}

	void ChatScroll::OnAudioResult(fig::tts::TTSResult&& payload)
	{
		if (not SDL_IsMainThread())
		{
			assert(false);
		}

		if (payload.has_value())
		{
			if (auto itFind = std::ranges::find(_messages, payload->sourceId, [](auto&& m) { return m.subMessageId; }); itFind != std::ranges::cend(_messages))
			{
				itFind->audioId = Global::GetAudioManager().EnqueueSound(payload->audioData.AsBytes());
			}
			else
				LogLn("Audio result discarded. Source message was removed.");
		}
	}
}