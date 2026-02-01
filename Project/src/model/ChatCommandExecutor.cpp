#include <pch.h>
#include "model/ChatSession.h"
#include "model/ChatCommandExecutor.h"
#include "model/AppState.h"
#include "llm/LLMInstance.h"
#include "util/Common.h"
#include "util/StringUtility.h"
#include "llm/LLMUtility.h"
#include "gui/MainFrame.h"
#include "gui/ChatFrame.h"
#include "gui/ChatScroll.h"
#include "gui/TextBox.h"
#include <map>
#include <queue>
#include <cwctype>
#include <ranges>

using namespace fig::gui;
using namespace fig::common_util;
using namespace fig::string_util;
using namespace fig::llm;

template<typename T>
void queue_clear(std::queue<T>& q)
{
	std::queue<T> empty;
	std::swap(q, empty);
}

struct ChatCommandFunctionContext
{
	ChatScroll* pChatScroll;
	TextBox* pTextBox;
	fig::llm::LLMInstancePtr pLLM;
	ParsedChatCommandQueue& commandQueue;
};

using Ctx = ChatCommandFunctionContext;
using ChatCommandFunction = std::function<bool(ParsedChatCommand, Ctx)>;

enum class Requirement : uint32_t
{
	None	= 0,
	LLM		= 1 << 0,
	GUI		= 1 << 1,
};
using Requirements = EnumFlags<Requirement>;

struct ChatCommandFunctionInfo
{
	ChatCommandFunction func = nullptr;
	Requirements req = Requirements::None;
};


static std::vector<fig::string> FilterMessageIDs(std::vector<RemovedMessage> msgs)
{
	return msgs
		| std::views::transform([](RemovedMessage msg) { return msg.responseId; })
		| std::ranges::to<std::vector<fig::string>>();
};

static Role RoleFromName(fig::string text, LLMInstancePtr pLLM) 
{
	if (empty_or_whitespace(text))
		return Role::Undefined;

	if (std::iswdigit(from_utf8(text)[0]))
		return bot_from_index(std::stoi(text) - 1);

	for (auto const& kvp : pLLM->GetSession().GetCharactersByRole())
	{
		if (begins_with(kvp.second.shortName, text, true))
			return kvp.first;
	}
	return pLLM->GetSession().GetRoleOf(text);
};

static bool cmdUserMessage(ParsedChatCommand cmd, Ctx ctx)
{
#if _DEBUG
	if (!(ctx.pLLM && ctx.pLLM->IsReady()))
	{
		static int turn = 0;
		auto [msgType, complete] = fig::llm_util::detect_message_type(fig::llm_util::process_message(cmd.text, ""));
		ctx.pChatScroll->AddDummyMessage(turn % 2 == 0 ? "@USR" : "@BOT", turn % 2 == 0 ? Role::User : Role::Bot1, msgType, cmd.text);
		++turn;
		return true;
	}
#else
	if (!ctx.pLLM)
		return false;
#endif
	return ctx.pLLM->SendMessage(cmd.text);
}

static bool cmdSystemMessage(ParsedChatCommand cmd, Ctx ctx)
{
	return ctx.pLLM->PushMessage(Role::System, cmd.text, MessageType::SystemMessage);
}

static bool cmdInstigateDialogue(ParsedChatCommand cmd, Ctx ctx)
{
	Role targetRole = RoleFromName(cmd.text, ctx.pLLM);
	return ctx.pLLM->Instigate(targetRole, MessageType::Dialogue, 1);
}

static bool cmdInstigateAction(ParsedChatCommand cmd, Ctx ctx)
{
	Role targetRole = RoleFromName(cmd.text, ctx.pLLM);
	return ctx.pLLM->Instigate(targetRole, MessageType::Action, 1);
}

static bool cmdPassTurn(ParsedChatCommand cmd, Ctx ctx)
{
	Role targetRole = RoleFromName(cmd.text, ctx.pLLM);
	return ctx.pLLM->Instigate(targetRole, MessageType::Undefined, 3);
}

static bool cmdImpersonate(ParsedChatCommand cmd, Ctx ctx)
{
	return ctx.pLLM->Instigate(Role::User, MessageType::Dialogue, 1);
}

static bool cmdNarrate(ParsedChatCommand cmd, Ctx ctx)
{
	if (cmd.text.empty())
		return ctx.pLLM->Instigate(Role::Narrator, MessageType::Narration, 1);
	else
		return ctx.pLLM->PushMessage(Role::Narrator, "[" + cmd.text + "]", MessageType::Narration);
}

static bool cmdInstruct(ParsedChatCommand cmd, Ctx ctx)
{
	if (!cmd.text.empty())
		return ctx.pLLM->Instruct(cmd.text);
	return false;
}

static bool cmdErase(ParsedChatCommand cmd, Ctx ctx)
{
	int n = atoi(cmd.text.c_str());
	auto removedMsgs = ctx.pLLM->EraseMessages(std::max(n, 1));
	auto removedIds = FilterMessageIDs(removedMsgs);
	return ctx.pChatScroll->RemoveMessages(removedIds);
}

static bool cmdRedoResponse(ParsedChatCommand cmd, Ctx ctx)
{
	auto removedMsgs = ctx.pLLM->EraseMessages(1);
	if (removedMsgs.empty())
		return false;

	Role responder = removedMsgs.front().role;
	if (!is_bot(responder))
		responder = Role::Undefined;

	auto removedIds = FilterMessageIDs(removedMsgs);
	ctx.pChatScroll->RemoveMessages(removedIds);
	return ctx.pLLM->Instigate(responder, MessageType::Undefined, 3);
}

static bool cmdRollbackUserMessage(ParsedChatCommand cmd, Ctx ctx)
{
	auto removedMsgs = ctx.pLLM->RollbackUserMessage();
	if (removedMsgs.empty())
		return false;

	auto removedIds = FilterMessageIDs(removedMsgs);
	ctx.pChatScroll->RemoveMessages(removedIds);
	ctx.pTextBox->SetText(removedMsgs[0].content);
	return true;
}

static bool cmdReset(ParsedChatCommand cmd, Ctx ctx)
{
	uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
	if (not ctx.pLLM or not ctx.pLLM->IsReady() or ctx.pLLM->ResetChat(seed))
		ctx.pChatScroll->ClearMessages();
	queue_clear(ctx.commandQueue);
	return true;
}

static bool cmdReseed(ParsedChatCommand cmd, Ctx ctx)
{
	uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
	if (seed == 0)
		seed = 0xFFFFFFFF;
	ctx.pLLM->Reseed(seed);
	return true;
}

static bool cmdLook(ParsedChatCommand cmd, Ctx ctx)
{
	if (!cmd.text.empty())
	{
		ctx.pLLM->PushMessage(Role::Narrator, "[{{user}} takes a moment to examine " + cmd.text + ".]", MessageType::Narration, false, 1);
		ctx.pLLM->PushMessage(Role::Director, "{{Describe " + cmd.text + " from {{user}}'s perspective and pay attention to visual details.}}", MessageType::Direction, false, 1);
	}
	else
	{
		ctx.pLLM->PushMessage(Role::Narrator, "[{{user}} takes a moment to observe their surroundings.]", MessageType::Narration, false, 1);
		ctx.pLLM->PushMessage(Role::Director, "{{Describe what {{user}} can clearly see, including points of interest, interactable objects, and anyone who is present.}}", MessageType::Direction, false, 1);
	}
	return ctx.pLLM->Instigate(Role::Narrator, MessageType::Narration, 1);
}

static bool cmdExamine(ParsedChatCommand cmd, Ctx ctx)
{
	if (!cmd.text.empty())
	{
		ctx.pLLM->PushMessage(Role::Narrator, "[{{user}} examines the " + cmd.text + ".]", MessageType::Narration, false, 1);
		ctx.pLLM->PushMessage(Role::Director, "{{Describe what {{user}} is able to find, if anything, " + cmd.text + " in minute detail.}}", MessageType::Direction, false, 1);
		return ctx.pLLM->Instigate(Role::Narrator, MessageType::Narration, 1);
	}
	return false;
}

static bool cmdGenerateEmbedding(ParsedChatCommand cmd, Ctx ctx)
{
#if _DEBUG
	return ctx.pLLM->GenerateEmbedding(cmd.text);
#else
	return false;
#endif
}

static bool cmdNewStateVariable(ParsedChatCommand cmd, Ctx ctx)
{
	if (!cmd.text.empty())
	{
		size_t pos_eq = cmd.text.find('=');
		if (pos_eq != fig::npos)
		{
			fig::string name = trim(cmd.text.substr(0, pos_eq));
			fig::string value = trim(cmd.text.substr(pos_eq + 1));
			if (ctx.pLLM->SetStateVariable(name, value, true))
			{
				ctx.pChatScroll->AddSystemMessage(std::format("{} = {}", name, value));
				return true;
			}
		}
	}
	return false;
}

static bool cmdSetStateVariable(ParsedChatCommand cmd, Ctx ctx)
{
	if (!cmd.text.empty())
	{
		size_t pos_eq = cmd.text.find('=');
		if (pos_eq != fig::npos)
		{
			fig::string name = trim(cmd.text.substr(0, pos_eq));
			fig::string value = trim(cmd.text.substr(pos_eq + 1));
			if (ctx.pLLM->SetStateVariable(name, value, false))
			{
				ctx.pChatScroll->AddSystemMessage(std::format("{} = {}", name, value));
				return true;
			}
		}
	}
	return false;
}

static std::map<ChatCommand, ChatCommandFunctionInfo> functions
{
	{ ChatCommand::UserMessage,			 	{ cmdUserMessage,			{ Requirement::GUI }					} },
	{ ChatCommand::SystemMessage,		 	{ cmdSystemMessage,			{ Requirement::LLM }					} },
	{ ChatCommand::PassTurn,			 	{ cmdPassTurn,				{ Requirement::LLM }					} },
	{ ChatCommand::InstigateDialogue,	 	{ cmdInstigateDialogue,		{ Requirement::LLM }					} },
	{ ChatCommand::InstigateAction,		 	{ cmdInstigateAction,		{ Requirement::LLM }					} },
	{ ChatCommand::Impersonate,			 	{ cmdImpersonate,			{ Requirement::LLM }					} },
	{ ChatCommand::Narrate,				 	{ cmdNarrate,				{ Requirement::LLM }					} },
	{ ChatCommand::Instruct,			 	{ cmdInstruct,				{ Requirement::LLM }					} },
	{ ChatCommand::Reset,				 	{ cmdReset,					{ Requirement::GUI }					} },
	{ ChatCommand::Erase,			 		{ cmdErase,					{ Requirement::LLM, Requirement::GUI }	} },
	{ ChatCommand::RollbackUserMessage,	 	{ cmdRollbackUserMessage,	{ Requirement::LLM, Requirement::GUI }	} },
	{ ChatCommand::RedoResponse,		 	{ cmdRedoResponse,			{ Requirement::LLM, Requirement::GUI }	} },
	{ ChatCommand::Reseed,				 	{ cmdReseed,				{ Requirement::LLM }					} },
	{ ChatCommand::Look,				 	{ cmdLook,					{ Requirement::LLM }					} },
	{ ChatCommand::Examine,				 	{ cmdExamine,				{ Requirement::LLM }					} },
	{ ChatCommand::GenerateEmbedding,	 	{ cmdGenerateEmbedding,		{ Requirement::LLM }					} },
	{ ChatCommand::NewStateVariable,	 	{ cmdNewStateVariable,		{ Requirement::LLM }					} },
	{ ChatCommand::SetStateVariable,	 	{ cmdSetStateVariable,		{ Requirement::LLM }					} },
};

bool ChatCommandExecutor::Execute(ParsedChatCommand command, ChatCommandExecutor::Context context)
{
	if (command.command == ChatCommand::Invalid)
	{
		LogLn("Invalid command");
		return false;
	}

	auto itFind = functions.find(command.command);
	if (itFind == functions.cend())
	{
		LogLn("Command is not implemented");
		return false;
	}

	// Validate requirements
	auto& info = itFind->second;
	if (info.req.IsSet(Requirement::LLM) && !context.pLLM)
		return false; // Requires non-null pointer to LLM
	if (info.req.IsSet(Requirement::GUI) && !context.pChatFrame)
		return false; // Requires non-null pointer to GUI

	// Pick out gui controls for commands to access
	auto functionCtx = Ctx
	{
		.pChatScroll = context.pChatFrame->_pChatScroll,
		.pTextBox = context.pChatFrame->_pTextBox,
		.pLLM = context.pLLM,
		.commandQueue = context.pChatFrame->_commandQueue,
	};

	return info.func(command, functionCtx);
}