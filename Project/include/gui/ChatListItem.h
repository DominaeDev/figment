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
	struct AssetUserSettings;
}

namespace fig::gui
{
	enum class ChatListItemEvent
	{
		Refresh,
		Delete,
		DeleteArchive,
	};

	using ChatItemEventDelegate = std::function<void(class ChatListItem&, ChatListItemEvent event)>;

	class ChatListItem : public Panel
	{
		ChatListItem(ControlPtr pParent);
	
	public:
		ChatListItem(ControlPtr pParent, const fig::uuid& chatInstanceId, const fig::data::ChatInstance& chatInstance, const fig::data::ChatLog& chatLog, const fig::string& timeString);
		void ShowStar(bool bShow);
		void SetDelegate(ChatItemEventDelegate fnDelegate) { _fnDelegate = fnDelegate; }

	protected:
		void OnUpdate(float fElapsed);

		void OnSize() override;
		EventResult OnEvent(fig::event& event);

		void ShowMenu() noexcept;
		void NotifyUpdated();
		void NotifyDelete();
		void NotifyDeleteArchive();
	
	private:
		fig::uuid _chatInstanceId;
		fig::uuid _primaryCharacterId;
		fig::observer_ptr<Image> _pPortrait;
		fig::observer_ptr<StaticText> _pTitle;
		fig::observer_ptr<StaticText> _pMessage;
		fig::observer_ptr<StaticText> _pTimestamp;
		fig::observer_ptr<Image> _pStar;

		bool _bSelected {};
		bool _bHovered {};
		int32_t _menuId { -1 };
		ChatItemEventDelegate _fnDelegate {};
		bool _bHasError {};
	};
}