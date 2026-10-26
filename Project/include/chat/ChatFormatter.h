#pragma once

#include "Figment.h"
#include "chat/ChatTypes.h"

namespace fig::chat
{
	class ChatFormatter
	{
	public:
		static fig::string Dialogue(fig::string_view text) noexcept;
		static fig::string Action(fig::string_view text) noexcept;
		static fig::string Thought(fig::string_view text) noexcept;
		static fig::string Director(fig::string_view text) noexcept;
		static fig::string Narration(fig::string_view text) noexcept;

		static fig::string Wrap(fig::string_view text, MessageType msgType) noexcept;
		static fig::string_view Unwrap(fig::string_view text) noexcept;

		static std::pair<fig::string_view, fig::string_view> GetMessageTags(MessageType msgType) noexcept;
		static std::pair<fig::string, fig::string> GetTagAndName(fig::string_view text);

		static fig::string_list GetOpeningTags() noexcept;
		static fig::string_list GetClosingTags() noexcept;
		static fig::string_list GetFormattingTags() noexcept;
		static MessageType DetectMessageType(fig::string_view text) noexcept;
		static MessageType DetectMessageType(fig::string_view text, bool& out_bComplete) noexcept;
		static fig::string& SanitizeMessage(fig::string& text);
		static fig::string& CompleteMessage(fig::string& text);
		static MessageType GetMessageTypeForTag(fig::string_view tag);
		static void StripEndTag(fig::string& message) noexcept;

		static std::vector<Submessage> AnalyseMessage(fig::string_view message) noexcept;

		static fig::string FormatMessage(fig::string_view message, fig::string_view identifier, MessageType msgType) noexcept;
		static std::pair<fig::string, std::vector<Submessage>> ProcessMessage(fig::string_view message, fig::string_view identifier) noexcept;
		static fig::string CreateMessagePrefix(fig::string_view identifier, MessageType msgType) noexcept;
	};
}