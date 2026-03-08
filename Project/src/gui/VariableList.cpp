#include <pch.h>
#include "gui/VariableList.h"
#include "gui/StaticText.h"
#include "gui/NineGridRenderer.h"
#include "gui/AppResources.h"
#include "Constants.h"
#include "util/StringUtility.h"
#include <format>

using namespace fig::gui;
using namespace fig::util;

constexpr float Margin { 8.0f };

VariableList::VariableList(LayoutElement* pParent) : Control(pParent)
{
	auto pBG = new NineGridRenderer({ 30, 72, 64, 30 });
	pBG->SetTexture(AppResources::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BG));
	pBG->SetColor(Colors::MessageBackgroundDefault);
	pBG->SetCornerSize(6);
	pBG->SetExtend(5);
	SetBackgroundRenderer(pBG);

	auto pBorder = new NineGridRenderer({ 30, 72, 64, 30 });
	pBorder->SetTexture(AppResources::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BORDER));
	pBorder->SetColor(Colors::MessageBorderDefault);
	pBorder->SetCornerSize(6);
	pBorder->SetExtend(5);
	SetBorderRenderer(pBorder);

	SetForegroundColor(Colors::TextForeground);
	SetBackgroundColor(Colors::MessageBackgroundDefault);

	_pText = new StaticText(this, "", FontFace::Default, Constants::GUI::StatusBarFontSize, false);
	_pText->SetPosition(Margin, Margin);
	_pText->SetMaxSize(250, -1);
}

void VariableList::OnRender(Renderer* pRenderer)
{
	if (_pText->GetText().empty())
		return;
	Control::OnRender(pRenderer);
}

void VariableList::SetVariables(const std::map<fig::string, fig::string>& variables)
{
	fig::string text;
	text.reserve(512);
	for (auto& kvp : variables)
		text = text + std::format("{} = {}\n", kvp.first, kvp.second);
	text = rtrim(text);

	float w, h;
	_pText->SetTextAndResize(text, w, h);
	
	SetSize(w + Margin * 2.0f, h + Margin * 2.0f);
}

bool VariableList::IsEmpty() const
{
	return _pText->GetText().empty();
}