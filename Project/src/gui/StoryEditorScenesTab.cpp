#include <pch.h>
#include "gui/StoryEditorScenesTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/StoryEventWidget.h"
#include "gui/Menu.h"
#include "gui/DefaultTextStyleProvider.h"
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
		auto pName = CreateTextBox(this, pSizer, ValueBinding(&_pScene->name));
		pName->SetMaxWidth(300);

		// Title
		CreateLabel(this, pSizer, "Scene description");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding(&_pScene->content), 2);
		pDescription->SetStyleProvider<DefaultTextStyleProvider>();
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(18);
		
		// TTL
		CreateLabel(this, pSizer, "Max duration (in turns)");
		auto pTTL = CreateTextBox(this, pSizer, ValueBinding<int32_t>(&_pScene->ttl));
		if (_pScene->ttl <= 0)
			pTTL->SetText("");
		pTTL->SetMaxWidth(180);

		pSizer->AddSpacer(10);

		// Starting scene
		CreateToggle(this, pSizer, "Entry point", ValueBinding<bool>(&_pScene->bEntryPoint));

		// Events
		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "When entering");
		
		_pEventSizer = new VerticalSizer();
		pSizer->Add(_pEventSizer.get(), 0, SizerFlag::Expand);

		for (auto& storyEvent : _pScene->events)
			AddEvent(storyEvent);

		pSizer->AddSpacer(10);

		auto pAddEventButton = CreateControl<ButtonWithLabel>("Add event");
		pAddEventButton->SetHeight(35);
		pAddEventButton->SetDelegate([this] { ShowEventsMenu(); });
		pSizer->Add(pAddEventButton, 0);

		// Triggers
		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Exit triggers");

		pSizer->AddSpacer(10);

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
		menu.AddItem("Give instruction")
			.SetDelegate([this] { AddEvent(Scenario::Event::Type::Instruct); });
		menu.AddItem("Instigate response")
			.SetDelegate([this] { AddEvent(Scenario::Event::Type::Instigate); });
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
		menu.AddItem("Variable");
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
		pEvent->SetButtonDelegate([this, index] { OnEventSettingsMenu(index); });
		pEvent->SetMoveDelegate([this, index](auto&& dir) { OnMoveEvent(index, dir, IsShiftDown()); });
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

	void StoryEditorScenesTab::OnEventSettingsMenu(size_t eventIndex)
	{
		EventItem* pItem = nullptr;
		size_t index {};
		if (auto itFind = std::ranges::find(_eventItems, eventIndex, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_eventItems))
		{
			pItem = &(*itFind);
			index = std::distance(_eventItems.begin(), itFind);
		}
		else
			return;

		auto& menu = CreateMenu();
		menu.AddItem("Move to top")
			.SetEnabled(index > 0uz)
			.SetDelegate([this, eventIndex] { OnMoveEvent(eventIndex, -1, true); });
		menu.AddItem("Move up")
			.SetEnabled(index > 0uz)
			.SetDelegate([this, eventIndex] { OnMoveEvent(eventIndex, -1, false); });
		menu.AddItem("Move down")
			.SetEnabled(index + 1uz < _eventItems.size())
			.SetDelegate([this, eventIndex] { OnMoveEvent(eventIndex, 1, false); });
		menu.AddItem("Move to bottom")
			.SetEnabled(index + 1uz < _eventItems.size())
			.SetDelegate([this, eventIndex] { OnMoveEvent(eventIndex, 1, true); });

		menu.AddSeparator();
		menu.AddItem("Remove", Resource::ICON_DELETE)
			.SetDelegate([this, eventIndex] { OnRemoveEvent(eventIndex); });

		menu.Show();
	}

	void StoryEditorScenesTab::OnMoveEvent(size_t EventIndex, int32_t dir, bool bMaxDistance)
	{
		if (auto itFind = std::ranges::find(_eventItems, EventIndex, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_eventItems))
		{
			size_t index = std::distance(std::ranges::begin(_eventItems), itFind);
			if (dir < 0 and bMaxDistance) // Move to top
			{
				if (index == 0uz)
					return;
				EventItem item = *itFind;
				_eventItems.erase(itFind);
				_eventItems.insert(_eventItems.cbegin(), item);
			}
			else if (dir < 0) // Move up
			{
				if (index == 0uz)
					return;
				std::swap(_eventItems[index - 1], _eventItems[index]);
			}
			if (dir > 0 and bMaxDistance) // Move to bottom
			{
				if (index + 1 >= _eventItems.size())
					return;

				EventItem item = *itFind;
				_eventItems.erase(itFind);
				_eventItems.insert(_eventItems.cbegin() + _eventItems.size(), item);
			}
			else if (dir > 0) // Move down
			{
				if (index + 1 >= _eventItems.size())
					return;
				std::swap(_eventItems[index], _eventItems[index + 1]);
			}

			_pEventSizer->RemoveAll();
			for (auto& item : _eventItems)
			{
				_pEventSizer->Add(item.pControl, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);

				if (_pTabGroup)
				{
					_pTabGroup->RemoveControl(item.pControl);
					_pTabGroup->AddControl(item.pControl);
				}
			}
			InvalidateLayout();
			SetDirty();
		}
	}

	void StoryEditorScenesTab::OnRemoveEvent(size_t index)
	{
		if (auto itFind = std::ranges::find(_eventItems, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_eventItems))
		{
			auto& item = *itFind;
			_pEventSizer->Remove(item.pControl);
			DestroyChild(item.pControl);
			_eventItems.erase(itFind);
			SetDirty();
		}
	}
}