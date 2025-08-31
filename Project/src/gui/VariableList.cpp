#include "gui/VariableList.h"
#include "gui/StaticText.h"
#include "gui/NineGridBackgroundRenderer.h"
#include "gui/TextureStore.h"
#include "gui/Color.h"
#include "Constants.h"
#include "util/StringUtility.h"
#include <format>

#define MARGIN 8

VariableList::VariableList(Control* pParent) : Control(pParent)
{
	auto pBG = new NineGridBackgroundRenderer({ 30, 72, 64, 30 });
	pBG->SetTextures(TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BG), TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BORDER));
	pBG->SetColors(Colors::MessageBackgroundDefault, Colors::MessageBorderDefault);
	pBG->SetCornerSize(6);
	SetBackgroundRenderer(pBG);

	SetForegroundColor(Colors::TextForeground);
	SetBackgroundColor(Colors::MessageBackgroundDefault);

	_pText = new StaticText(this, "", FontFace::Default, Constants::GUI::StatusBarFontSize, false);
	_pText->SetPosition(MARGIN, MARGIN);
	_pText->SetMaxSize(250, -1);
}

void VariableList::OnRender(Renderer* pRenderer)
{
	if (_pText->GetText().empty())
		return;
	Control::OnRender(pRenderer);
}

void VariableList::SetVariables(const std::map<string, string>& variables)
{
	string text;
	text.reserve(512);
	for (auto kvp : variables)
		text = text + std::format("{} = {}\n", kvp.first, kvp.second);
	text = string_util::rtrim(text);

	int w, h;
	_pText->SetTextAndResize(text, w, h);
	
	SetSize(w + MARGIN * 2, h + MARGIN * 2);
}

bool VariableList::IsEmpty() const
{
	return _pText->GetText().empty();
}