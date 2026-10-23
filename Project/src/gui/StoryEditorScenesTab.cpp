#include <pch.h>
#include "gui/StoryEditorScenesTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/StoryEventWidget.h"
#include "gui/Menu.h"
#include "data/Scenario.h"
#include "util/StringUtils.h"

using namespace fig::chat;
using namespace fig::data;

namespace fig::gui
{
	size_t StoryEditorScenesTab::_nextEventIndex {};

	StoryEditorScenesTab::StoryEditorScenesTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool StoryEditorScenesTab::Initialize(StoryEditorArgs args)
	{
		if (not (bool)args.pScenario)
			return false;

		_pScenario = args.pScenario;

		if (_pScenario->scenes.empty())
			_pScenario->CreateScene();
		_pScene = &_pScenario->scenes.back();

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Scene");

		// Name
		CreateLabel(this, pSizer, "Scene name");
		auto pName = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScene->name));
		pName->SetMaxWidth(300);

		// Title
		CreateLabel(this, pSizer, "Scene description");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScene->content), 4);
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(18);

		// Events
		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Events");
		
		_pEventSizer = new VerticalSizer();
		pSizer->Add(_pEventSizer.get(), 0, SizerFlag::Expand | SizerFlag::Bottom, 8);

		for (auto& storyEvent : _pScene->events)
			AddEvent(storyEvent);

		pSizer->AddSpacer(6);

		auto pAddEventButton = CreateControl<ButtonWithLabel>("Add event");
		pAddEventButton->SetHeight(35);
		pAddEventButton->SetDelegate([this] { ShowEventsMenu(); });
		pSizer->Add(pAddEventButton, 0);

		// Triggers
		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Triggers");

		pSizer->AddSpacer(6);

		auto pAddTriggerButton = CreateControl<ButtonWithLabel>("Add trigger");
		pAddTriggerButton->SetHeight(35);
		pAddTriggerButton->SetDelegate([this] { ShowTriggersMenu(); });
		pSizer->Add(pAddTriggerButton, 0);

		CreateTabGroup();

		return true;
	}

	void StoryEditorScenesTab::ShowEventsMenu()
	{
		auto& menu = CreateMenu();
		menu.AddItem("Display user message")
			.SetDelegate([this] { AddEvent(Scenario::Event::Type::ShowMessage); });
		menu.AddItem("Instigate response")
			.SetDelegate([this] { AddEvent(Scenario::Event::Type::Instigate); });
		menu.AddItem("Give instruction")
			.SetDelegate([this] { AddEvent(Scenario::Event::Type::Instruct); });
		menu.AddItem("Chat message")
			.SetDelegate([this] { AddEvent(Scenario::Event::Type::ChatMessage); });
		menu.Show();
	}

	void StoryEditorScenesTab::ShowTriggersMenu()
	{
		auto& menu = CreateMenu();
		menu.AddItem("Keyword");
		menu.AddItem("Phrase / sentence");
		menu.AddItem("Counter");
		menu.AddItem("Compare variable");
		menu.AddItem("Condition");
		menu.Show();
	}

	void StoryEditorScenesTab::AddEvent(Scenario::Event::Type type)
	{
		if (not (bool)_pScene or type == Scenario::Event::Type::Invalid)
			return;

		_pScene->events.emplace_back(Scenario::Event {
			.type = type,
		});
		auto& storyEvent = _pScene->events.back();
		
		if (type == Scenario::Event::Type::Instigate
			or type == Scenario::Event::Type::ChatMessage)
			storyEvent.role = Role::Bot1;
		AddEvent(storyEvent);
	}

	fig::observer_ptr<StoryEventWidget> StoryEditorScenesTab::AddEvent(const Scenario::Event& storyEvent)
	{
		auto item = EventItem {
			.index = _nextEventIndex++,
		};

		item.pControl = AppendEventWidget(storyEvent, item.index);
		_eventItems.push_back(std::move(item));
		return item.pControl;
	}

	fig::observer_ptr<StoryEventWidget> StoryEditorScenesTab::AppendEventWidget(const Scenario::Event& storyEvent, size_t index)
	{
		auto pEvent = CreateControl<StoryEventWidget>(storyEvent, _pScenario.get());
		pEvent->SetButtonDelegate([this, index] { return; /*OnEventSettingsMenu(index);*/ });
		pEvent->SetMoveDelegate([this, index](auto&& dir) { /*OnMoveEvent(index, dir, IsShiftDown()); */ });
		pEvent->SetValueChangedDelegate([this](auto&& _) { SetDirty(); });
		_pEventSizer->Add(pEvent, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);

//		if (_pTabGroup)
//			_pTabGroup->AddControl(pEvent);

		return pEvent;
	}

	EditorTabBase::SaveResult StoryEditorScenesTab::OnSave() noexcept
	{
		// Write events
		_pScene->events.clear();

		for (auto& item : _eventItems)
		{
			auto value = item.pControl->GetValue();
			if (value.type != Scenario::Event::Type::Invalid)
				_pScene->events.push_back(std::move(value));
		}

		return {};
	}
}