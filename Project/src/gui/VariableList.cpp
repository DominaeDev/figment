#include <pch.h>
#include "gui/VariableList.h"
#include "gui/StaticText.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include <format>

using namespace fig::gui;

constexpr Coord Margin = 8;

VariableList::VariableList(ParentPtr pParent) : Control(pParent)
{
	auto pBG = SetBackgroundRenderer<TexturedBorderRenderer>(TextureType::SPEECH_BUBBLE_CENTER_BG, Corners { 30, 72, 64, 30 });
	pBG->SetColor(Colors::MessageBackgroundDefault);
	pBG->SetCornerScale(0.3f);
	pBG->SetExtend(5);

	auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(TextureType::SPEECH_BUBBLE_CENTER_BORDER, Corners { 30, 72, 64, 30 });
	pBorder->SetColor(Colors::MessageBorderDefault);
	pBorder->SetCornerScale(0.3f);
	pBorder->SetExtend(5);

	SetForegroundColor(Colors::TextForeground);
	SetBackgroundColor(Colors::MessageBackgroundDefault);

	_pText = CreateControl<StaticText>("", FontFace::Default, Constants::GUI::StatusBarFontSize, false);
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

	Coord w, h;
	_pText->SetTextAndResize(text, w, h);
	
	SetSize(w + Margin * 2, h + Margin * 2);
}

bool VariableList::IsEmpty() const
{
	return _pText->GetText().empty();
}