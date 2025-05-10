#include "MainFrame.h"
#include "Panel.h"
#include "HorizontalSizer.h"

MainFrame::MainFrame()
{
	SetBackgroundColor(SDL_Color { 255, 255, 255, SDL_ALPHA_OPAQUE });

	auto panel1 = new Panel();
	panel1->SetBackgroundColor(SDL_Color { 200, 50, 50, SDL_ALPHA_OPAQUE });
	AddChild(panel1);

	auto panel2 = new Panel();
	panel2->SetBackgroundColor(SDL_Color { 50, 200, 50, SDL_ALPHA_OPAQUE });
	panel2->SetMinSize(800, -1);
	AddChild(panel2);

	auto panel3 = new Panel();
	panel3->SetBackgroundColor(SDL_Color { 50, 50, 200, SDL_ALPHA_OPAQUE });
	AddChild(panel3);

	_pSizer = new HorizontalSizer();
	_pSizer->Add(panel1, -1, Sizer::Expand);
	_pSizer->Add(panel2, 0, Sizer::Expand | Sizer::All, 8);
	_pSizer->Add(panel3, -1, Sizer::Expand);
}

MainFrame::~MainFrame()
{
	delete _pSizer;
}

void MainFrame::OnUpdate(float fDeltaTime)
{
	_pSizer->Layout(_rect);
}

void MainFrame::OnRender(SDL_Renderer* pRenderer)
{
	ClearBackground(pRenderer);
}