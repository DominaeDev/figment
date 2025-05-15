#include "MainFrame.h"
#include "Panel.h"
#include "StaticLabel.h"
#include "HorizontalSizer.h"
#include "VerticalSizer.h"
#include "TextBox.h"
#include "Constants.h"

MainFrame::MainFrame(SDL_Window* pWindow) : Frame(pWindow)
{
	SetForegroundColor(SDL_Color { 0, 0, 0, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(SDL_Color { 255, 255, 255, SDL_ALPHA_OPAQUE });

	auto leftPanel = new Panel();
	leftPanel->SetBackgroundColor(SDL_Color { 200, 50, 50 });
	leftPanel->SetSize(200, -1);

	auto centerPanel = new Panel();
	centerPanel->SetBackgroundColor(SDL_Color { 50, 200, 50 });
	centerPanel->SetSize(-1, -1);

	auto rightPanel = new Panel();
	rightPanel->SetBackgroundColor(SDL_Color { 50, 50, 200 });
	rightPanel->SetSize(200, -1);
	rightPanel->SetMinSize(200, -1);

	AddChild(leftPanel);
	AddChild(centerPanel);
	AddChild(rightPanel);

	auto pTextBox = new TextBox(FontFace::Default, Constants::DefaultFontSize);
	pTextBox->SetSize(300, 200);
	pTextBox->SetBackgroundColor(SDL_Color { 255, 255, 255, 255 });
	pTextBox->SelectAll();
	centerPanel->AddChild(pTextBox);

	auto pLabel = new StaticLabel("Hello, how are you?", FontFace::Default, Constants::DefaultFontSize);
	pLabel->SetAlignment(TextAlignment::Middle_Center);
	pLabel->SetSize(80, 80);
	pLabel->SetMinSize(-1, 80);
	pLabel->SetBackgroundColor(SDL_Color { 255, 255, 0, SDL_ALPHA_OPAQUE });

	centerPanel->AddChild(pLabel);
	auto pCenterSizer = new HorizontalSizer();
	pCenterSizer->Add(pLabel, 0, Sizer::AlignBottom);
	centerPanel->SetSizer(pCenterSizer);

	auto pTopSizer = new HorizontalSizer();
	pTopSizer->Add(leftPanel, 0, Sizer::Expand);
	pTopSizer->Add(centerPanel, -1, Sizer::Expand | Sizer::All, 8);
	pTopSizer->Add(rightPanel, 0, Sizer::Expand);
	SetSizer(pTopSizer);
}

MainFrame::~MainFrame()
{
}

void MainFrame::OnUpdate(float fDeltaTime)
{
}

void MainFrame::OnRender(SDL_Renderer* pRenderer)
{
	ClearBackground(pRenderer);
}