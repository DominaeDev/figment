#include <pch.h>
#include "gui/DebugScreen.h"
#include "gui/CardImage.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithLabel.h"
#include "gui/Slider.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/DropList.h"
#include "gui/ComboBox.h"
#include "gui/CheckBox.h"
#include "gui/TextBox.h"

namespace fig::gui
{
	DebugScreen::DebugScreen(Frame* pParent) : Screen(pParent)
	{
		if constexpr (false)
		{
			auto area = CreateControl<Panel>();
			area->SetBackgroundColor(Color::Black);
			area->SetPosition(200, 200);
			area->SetSize(500, 500);

			auto left = area->CreateControl<Panel>();
			left->SetBackgroundColor(fig::color { 0xC0, 0, 0, 0xFF });
			left->SetSize(50, 50);

			auto center = area->CreateControl<Panel>();
			center->SetBackgroundColor(fig::color { 0, 0xC0, 0, 0xFF });
			center->SetSize(100, 100);

			auto right = area->CreateControl<Panel>();
			right->SetBackgroundColor(fig::color { 0, 0, 0xC0, 0xFF });
			right->SetSize(100, 100);

			auto h = right->SetSizer<HorizontalSizer>();

			auto a = right->CreateControl<Panel>();
			a->SetBackgroundColor(Color::Red);
			a->SetSize(50, 50);
			auto b = right->CreateControl<Panel>();
			b->SetBackgroundColor(Color::Green);
			b->SetSize(50, 50);
			auto c = right->CreateControl<Panel>();
			c->SetBackgroundColor(Color::Blue);
			c->SetSize(50, 50);

			h->Add(a, -1, SizerFlag::AlignRight | SizerFlag::AlignTop, 4);
			h->Add(b, 0, SizerFlag::AlignCenterHorizontal | SizerFlag::AlignCenterVertical, 4);
			h->Add(c, -1, SizerFlag::AlignLeft | SizerFlag::AlignBottom, 4);

			auto sizer = area->SetSizer<VerticalSizer>();
			sizer->Add(left, -1, SizerFlag::AlignRight | SizerFlag::AlignCenterVertical);
			sizer->AddSpacer(4);
			sizer->Add(center, -1, SizerFlag::Fill);
			sizer->AddSpacer(4);
			sizer->Add(right, 0, SizerFlag::Expand | SizerFlag::AlignCenterVertical | SizerFlag::FixedSize, 100);
		}

		auto pSlider = CreateControl<Slider>();
		pSlider->SetPosition(100, 50);

		auto pSlider2 = CreateControl<Slider>();
		pSlider2->SetPosition(500, 50);
		pSlider2->SetEnabled(false);

		auto pButton = CreateControl<ButtonWithLabel>("Invalidate");
		pButton->SetPosition(100, 100);
		pButton->SetHeight(35);
		pButton->SetDelegate([this]() { InvalidateLayout(); });

		auto pButton2 = CreateControl<ButtonWithLabel>("Invalidate");
		pButton2->SetPosition(500, 100);
		pButton2->SetHeight(35);
		pButton2->SetDelegate([this]() { InvalidateLayout(); });
		pButton2->SetEnabled(false);

		auto pDropList = CreateControl<DropList>();
		pDropList->SetPosition(100, 150);
		pDropList->AddItem("Item #1");
		pDropList->AddItem("Item #2");
		pDropList->AddItem("Item #3");
		pDropList->AddItem("Item #4");
		pDropList->Select(0);

		auto pDropList2 = CreateControl<DropList>();
		pDropList2->SetPosition(500, 150);
		pDropList2->AddItem("Item #1");
		pDropList2->AddItem("Item #2");
		pDropList2->AddItem("Item #3");
		pDropList2->AddItem("Item #4");
		pDropList2->Select(0);
		pDropList2->SetEnabled(false);

		auto pComboBox = CreateControl<ComboBox>();
		pComboBox->SetPosition(100, 200);
		pComboBox->AddItem("Item #1");
		pComboBox->AddItem("Item #2");
		pComboBox->AddItem("Item #3");
		pComboBox->AddItem("Item #4");
		pComboBox->Select(0);

		auto pComboBox2 = CreateControl<ComboBox>();
		pComboBox2->SetPosition(500, 200);
		pComboBox2->AddItem("Item #1");
		pComboBox2->AddItem("Item #2");
		pComboBox2->AddItem("Item #3");
		pComboBox2->AddItem("Item #4");
		pComboBox2->Select(0);
		pComboBox2->SetEnabled(false);

		auto pCheckBox = CreateControl<CheckBox>("Checkbox", false);
		pCheckBox->SetPosition(100, 250);

		auto pCheckBox2 = CreateControl<CheckBox>("Checkbox", true);
		pCheckBox2->SetPosition(500, 250);
		pCheckBox2->SetEnabled(false);

		auto pTextBox = CreateControl<TextBox>();
		pTextBox->SetText("Text box");
		pTextBox->SetPosition(100, 300);

		auto pTextBox2 = CreateControl<TextBox>();
		pTextBox2->SetText("Text box");
		pTextBox2->SetPosition(500, 300);
		pTextBox2->SetEnabled(false);

		auto pTextBox3 = CreateControl<TextBox>(FontFace::Default, Constants::GUI::DefaultFontSize, TextBox::Flags { TextBox::Flag::Multi });
		pTextBox3->SetText("Text box (multi)");
		pTextBox3->SetFixedRows(3);
		pTextBox3->SetPosition(100, 350);

		auto pTextBox4 = CreateControl<TextBox>(FontFace::Default, Constants::GUI::DefaultFontSize, TextBox::Flags { TextBox::Flag::Multi });
		pTextBox4->SetText("Text box (multi)");
		pTextBox4->SetFixedRows(3);
		pTextBox4->SetPosition(500, 350);
		pTextBox4->SetEnabled(false);

	}

	void DebugScreen::OnUpdate(float fElapsed)
	{
	}

	void DebugScreen::OnRender(fig::renderer_ptr pRenderer)
	{
	}

	bool DebugScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed) // Press
		{
		}
		else // Release
		{
		}
		return false;
	}

}