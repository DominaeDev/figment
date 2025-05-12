#include "MainFrame.h"
#include "Panel.h"
#include "StaticLabel.h"
#include "HorizontalSizer.h"
#include "VerticalSizer.h"
#include "Constants.h"

MainFrame::MainFrame(SDL_Window* pWindow) : Frame(pWindow)
{
	SetForegroundColor(SDL_Color { 0, 0, 0, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(SDL_Color { 255, 255, 255, SDL_ALPHA_OPAQUE });

	auto panel1 = new Panel();
	panel1->SetBackgroundColor(SDL_Color { 200, 50, 50 });
	panel1->SetSize(200, -1);
	panel1->SetMinSize(200, -1);
	AddChild(panel1);

	auto panel2 = new Panel();
	panel2->SetBackgroundColor(SDL_Color { 50, 200, 50 });
	panel2->SetSize(900, -1);
//	panel2->SetMinSize(500, -1);
	AddChild(panel2);

	auto panel3 = new Panel();
	panel3->SetBackgroundColor(SDL_Color { 50, 50, 200 });
	panel3->SetSize(200, -1);
	panel3->SetMinSize(200, -1);
	AddChild(panel3);

	auto child = new Panel();
	child->SetBackgroundColor(SDL_Color { 0, 0, 0, SDL_ALPHA_OPAQUE });
	child->SetPosition(50, 100);
	child->SetSize(200, 200);
	panel2->AddChild(child);

	auto label = new StaticLabel("Hello, how are you?", FontFace::Default, Constants::DefaultFontSize);

	panel2->AddChild(label);

	auto pSizer = new HorizontalSizer();
	pSizer->Add(panel1, 0, Sizer::Expand);
	pSizer->Add(panel2, -1, Sizer::Expand | Sizer::All, 8);
	pSizer->Add(panel3, 0, Sizer::Expand);
	SetSizer(pSizer);

	auto panelA = new Panel();
	panelA->SetBackgroundColor(SDL_Color { 0, 50, 50 });
	panelA->SetSize(-1, 100);
	panel1->AddChild(panelA);

	auto panelB = new Panel();
	panelB->SetBackgroundColor(SDL_Color { 50, 0, 50 });
	panelB->SetSize(-1, 100);
	panel1->AddChild(panelB);

	auto panelC = new Panel();
	panelC->SetBackgroundColor(SDL_Color { 50, 50, 0 });
	panelC->SetSize(-1, 100);
	panel1->AddChild(panelC);

	auto pSizer2 = new VerticalSizer();
	pSizer2->Add(panelA, 0, Sizer::Expand);
	pSizer2->Add(panelB, -1, Sizer::Expand | Sizer::All, 8);
	pSizer2->Add(panelC, 0, Sizer::Expand);
	panel1->SetSizer(pSizer2);


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