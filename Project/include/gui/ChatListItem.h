#pragma once

#include "gui/Panel.h"

namespace fig::data
{
	class ChatLog;
}

namespace fig::gui
{
	class ChatListItem : public Panel
	{
	public:
		enum class TimeBucket
		{
			LessThan5Minutes = 0,
			LessThan1Day,
			LessThan2Days,
			LessThan1Week,
			LessThan1Month,
			Older,
		};
		static std::array<fig::string_view, 6> TimeBucketLabels;

		ChatListItem(ParentPtr pParent);
		ChatListItem(ParentPtr pParent, const fig::data::ChatLog& asset, fig::timestamp lastUsed, TimeBucket bucket);
		
	protected:
		void OnUpdate(float fElapsed);

		void OnSize() override;
		EventResult OnEvent(Event& event);

		void ShowMenu() noexcept;

	private:
		fig::uuid _assetId;
		fig::observer_ptr<Image> _pPortrait;
		fig::observer_ptr<StaticText> _pTitle;
		fig::observer_ptr<StaticText> _pMessage;
		fig::observer_ptr<StaticText> _pTimestamp;
		bool _bSelected {};
		bool _bHovered {};
		int32_t _menuId { -1 };
	};
}