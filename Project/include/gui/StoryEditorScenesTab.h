#pragma once

#include "gui/EditorTab.h"
#include "gui/StoryEditorArgs.h"
#include "data/Scenario.h"

namespace fig::gui
{
	class StoryEventWidget;

	class StoryEditorScenesTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorScenesTab(ControlPtr pParent);

	protected:
		bool OnInit(StoryEditorArgs args) override;

	protected:
		SaveResult OnSave() noexcept override;
		void OnEventSettingsMenu(size_t eventIndex);
		void OnMoveEvent(size_t EventIndex, int32_t dir, bool bMaxDistance);
		void OnRemoveEvent(size_t index);


		void ShowEventsMenu();
		void ShowTriggersMenu();
		void AddEvent(fig::data::Scenario::Event::Type type);
		fig::observer_ptr<StoryEventWidget> AddEvent(const fig::data::Scenario::Event& event);
		fig::observer_ptr<StoryEventWidget> AppendEventWidget(const fig::data::Scenario::Event& event, size_t index);
		void OnChangedEventType(size_t eventIndex, fig::data::Scenario::Event::Type eventType);

	private:
		fig::observer_ptr<fig::data::Scenario> _pScenario {};
		fig::observer_ptr<fig::data::Scenario::Scene> _pScene {};
		fig::observer_ptr<TextBox> _pTags {};

		struct EventItem
		{
			size_t index;
			fig::observer_ptr<StoryEventWidget> pControl;
		};
		std::vector<EventItem> _eventItems;
		static size_t _nextEventIndex;
		SizerPtr _pEventSizer {};
	};
}