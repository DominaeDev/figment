#include <pch.h>
#include "gui/GUICommon.h"
#include "gui/StatusBar.h"
#include "llm/LLMStatus.h"

using namespace fig::gui;
using namespace fig::llm;

StatusBar::StatusBar(ControlPtr pParent) : Control(pParent)
{
	SetSize(pParent->GetWidth(), 24);

	SetForegroundColor(fig::color { 0x51, 0x4a, 0x2f, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(fig::color { 0xde, 0xd9, 0xc5, SDL_ALPHA_OPAQUE });

	_pMessage = CreateControl<StaticText>("", FontFace::Default, Constants::GUI::StatusBarFontSize, false);
	_pMessage->SetPosition(8, 2);

	_pModelInfo = CreateControl<StaticText>("", FontFace::Default, Constants::GUI::StatusBarFontSize, false);
	_pModelInfo->SetPosition(400, 2);
}

void StatusBar::SetMessage(const fig::string& message)
{
	_pMessage->SetText(message);
}

void StatusBar::SetModelInfo(const fig::llm::LLMStatus& status)
{
	if (!status.IsReady())
		_pModelInfo->SetText("");
	
	if (!status.modelName.empty())
	{
		static const int64_t MiB = 1024 * 1024;
		_pModelInfo->SetText(std::format("Model: {0} Ctx: {1:d}/{2:d} ({3:.2f} t/s) VRAM: {4:.2f} GiB RAM: {5:.2f} GiB", 
			status.modelName, 
			status.usedCtxSize, 
			status.allocCtxSize, 
			status.tokensPerSec,
			static_cast<float>(status.usedVRAM / MiB) / 1024.0f,
			static_cast<float>(status.usedRAM / MiB) / 1024.0f
		));
	}
}
