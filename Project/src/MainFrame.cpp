#include "MainFrame.h"
#include "Panel.h"
#include "HorizontalSizer.h"

MainFrame::MainFrame()
{
	SetBackgroundColor(SDL_Color { 255, 255, 255, SDL_ALPHA_OPAQUE });

	auto panel1 = new Panel();
	panel1->SetBackgroundColor(SDL_Color { 200, 50, 50, SDL_ALPHA_OPAQUE });
	panel1->SetSize(200, -1);
	panel1->SetMinSize(200, -1);
	AddChild(panel1);

	auto panel2 = new Panel();
	panel2->SetBackgroundColor(SDL_Color { 50, 200, 50, SDL_ALPHA_OPAQUE });
	panel2->SetMinSize(500, -1);
	panel2->SetPreferredSize(900, -1);
	AddChild(panel2);

	auto panel3 = new Panel();
	panel3->SetBackgroundColor(SDL_Color { 50, 50, 200, SDL_ALPHA_OPAQUE });
	panel3->SetMinSize(200, -1);
	AddChild(panel3);

	auto child = new Panel();
	child->SetBackgroundColor(SDL_Color { 200, 200, 50, SDL_ALPHA_OPAQUE });
	child->SetPosition(0, 100);
	child->SetMinSize(-1, 200);
	child->SetSize(2000, 200);
	child->SetPreferredSize(2000, 200);
	panel2->AddChild(child);

	auto pSizer = new HorizontalSizer();
	pSizer->Add(panel1, -1, Sizer::Expand);
	pSizer->Add(panel2, 0, Sizer::Expand | Sizer::All, 8);
	pSizer->Add(panel3, -1, Sizer::Expand);
	SetSizer(pSizer);
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