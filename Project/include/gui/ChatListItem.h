#pragma once

#include "gui/Panel.h"
#include "util/SearchIndex.h"
#include "util/Timestamp.h"

namespace fig::data
{
	class ChatLog;
}

namespace fig::io
{
	struct ContentUserSettings;
}

namespace fig::gui
{
	using OnChatUpdatedDelegate = std::function<void(class ChatListItem&)>;

	class ChatListItem : public Panel
	{
		ChatListItem(ParentPtr pParent);
	
	public:
		ChatListItem(ParentPtr pParent, const fig::uuid& assetId, const fig::data::ChatLog& chatLog, const fig::string& timeString);
		void SetDelegate(OnChatUpdatedDelegate onUpdated);
		void ShowStar(bool bShow);

	protected:
		void OnUpdate(float fElapsed);

		void OnSize() override;
		EventResult OnEvent(Event& event);

		void ShowMenu() noexcept;
		void NotifyMetaUpdated();
	private:
		fig::uuid _assetId;
		fig::uuid _primaryCharacterId;
		fig::observer_ptr<Image> _pPortrait;
		fig::observer_ptr<StaticText> _pTitle;
		fig::observer_ptr<StaticText> _pMessage;
		fig::observer_ptr<StaticText> _pTimestamp;
		fig::observer_ptr<Image> _pStar;

		bool _bSelected {};
		bool _bHovered {};
		int32_t _menuId { -1 };
		OnChatUpdatedDelegate _fnOnUpdated {};
	};
}