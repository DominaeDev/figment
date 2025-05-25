#include "ChatScroll.h"
#include "VerticalListSizer.h"
#include "ChatMessage.h"

ChatScroll::ChatScroll(Control* pParent) : Control(pParent)
{
	auto pTopSizer = new VerticalListSizer();
	pTopSizer->SetBottomMargin(8);
	pTopSizer->SetSpacing(8);
	SetSizer(pTopSizer);
}

ChatMessage* ChatScroll::AddMessage(string name, string message)
{
	auto pMessage = new ChatMessage(this, name, message);
	_pSizer->Add(pMessage, 0, Sizer::Expand);
	return pMessage;
}