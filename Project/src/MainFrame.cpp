#include "MainFrame.h"
#include "Panel.h"
#include "StaticLabel.h"
#include "HorizontalSizer.h"
#include "VerticalSizer.h"
#include "TextBox.h"
#include "Constants.h"
#include "Color.h"

MainFrame::MainFrame(SDL_Window* pWindow) : Frame(pWindow)
{
	SetForegroundColor(SDL_Color { 0, 0, 0, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(SDL_Color { 30, 30, 30, SDL_ALPHA_OPAQUE });

	auto leftPanel = new Panel();
	leftPanel->SetSize(200, -1);

	auto centerPanel = new Panel();
	centerPanel->SetBackgroundColor(SDL_Color { 40, 40, 40, SDL_ALPHA_OPAQUE });
	centerPanel->SetSize(800, -1);

	auto rightPanel = new Panel();
	rightPanel->SetSize(200, -1);
	rightPanel->SetMinSize(200, -1);

	AddChild(leftPanel);
	AddChild(centerPanel);
	AddChild(rightPanel);

	auto pStaticLabel = new StaticLabel("Hello, how are you? iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii", FontFace::Default, Constants::DefaultFontSize);
	pStaticLabel->SetAlignment(TextAlignment::Middle_Center);
	pStaticLabel->SetSize(80, 80);
	pStaticLabel->SetMinSize(-1, 80);
	pStaticLabel->SetBackgroundColor(SDL_Color { 255, 255, 0, SDL_ALPHA_OPAQUE });
	pStaticLabel->SetVisible(false);

	auto pTextBox = new TextBox(FontFace::Default, Constants::DefaultFontSize);
	pTextBox->SetSize(-1, 88);
	pTextBox->SetBackgroundColor(Color::White);
	pTextBox->SelectAll();
	centerPanel->AddChild(pTextBox);
	centerPanel->AddChild(pStaticLabel);

	auto pCenterSizer = new VerticalSizer();
	pCenterSizer->AddStretchSpacer();
	pCenterSizer->Add(pTextBox, 0, Sizer::AlignBottom | Sizer::Expand);
	centerPanel->SetSizer(pCenterSizer);

	auto pTopSizer = new HorizontalSizer();
	pTopSizer->Add(leftPanel, -1, Sizer::Expand);
	pTopSizer->Add(centerPanel, 0, Sizer::Expand | Sizer::Bottom, 8);
	pTopSizer->Add(rightPanel, -1, Sizer::Expand);
	SetSizer(pTopSizer);

	pTextBox->SetFocus(true);
}

MainFrame::~MainFrame()
{
}

void MainFrame::OnUpdate(float fDeltaTime)
{
}

void MainFrame::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}