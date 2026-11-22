#include <pch.h>
#include "gui/UserPersonaListItem.h"
#include "gui/CustomRenderers.h"
#include "gui/AppResources.h"
#include "gui/StaticText.h"
#include "gui/ImageWithMask.h"
#include "gui/NineGridImage.h"
#include "gui/Menu.h"
#include "gui/Frame.h"
#include "io/ContentManager.h"
#include "io/AssetManager.h"
#include "data/Character.h"
#include "text/TextEvaluator.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	UserPersonaListItem::UserPersonaListItem(ControlPtr pParent, const fig::uuid& userPersonaId) : Panel(pParent),
		_assetId { userPersonaId }
	{
		SetSize(Width, Height);

		// Background
		SetForegroundColor(Color::PanelForeground);
		SetBackgroundColor(Color::PanelBackground);
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);

		// Title
		_pTitle = CreateControl<StaticText>("", FontFace::CardHeader, 21.0, false);
		_pTitle->SetPosition(112, 8);
		_pTitle->SetSize(Width - _pTitle->GetX() - 8, 80);
		_pTitle->SetForegroundColor(Color::White);
		_pTitle->SetBackgroundColor(Color::Transparent);
		_pTitle->EnableDropShadow(true);
		_pTitle->EnableEllipsis(true);

		// Message
		_pMessage = CreateControl<StaticText>("", FontFace::Italic, 14.0, false);
		_pMessage->SetPosition(112, 36);
		_pMessage->SetForegroundColor(Color::PanelForeground);
		_pMessage->SetMaxSize(-1, 15);
		_pMessage->SetTextWrapWidth(Width - _pMessage->GetX() - 8);
		_pMessage->SetMaxLines(6uz);
		_pMessage->EnableWordWrap(true);

		// Portrait
		_pPortrait = CreateControl<ImageWithMask>(nullptr, nullptr);
		_pPortrait->SetSize(96, 96);
		_pPortrait->SetPosition(8, 8);
		_pPortrait->SetVisible(false);

		RefreshFull();
		RefreshState();
	}

	void UserPersonaListItem::RefreshFull()
	{
		if (auto try_user = Global::GetUserContent().Get<UserPersona>(_assetId))
		{
			auto& userPersona = *try_user;
			auto ctx = userPersona.GetContext();
			ctx.SetMacroProvider(Global::GetMacroProvider());
			ctx.SetPrimarySelector({});
			ctx.SetAlias("current", ContextSelector {});
			ctx.SetMacro("user", "{current:name}");
			_pTitle->SetText(userPersona.GetName());

			fig::string info = R"({gender?Gender: {gender}}\n{age?Age: {age}}\n\n{brief})";
			_pMessage->SetText(eval_text(info, ctx));

			if (auto try_asset = Global::GetUserContent().GetAssets().FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::SmallPortrait), _assetId))
			{
				if (auto portrait = Global::GetUserContent().Get<fig::sdl::Surface>((*try_asset).id))
				{
					_pPortrait->SetImage(*portrait, AppResources::GetTexture(Resource::MASK_SMALL_PORTRAIT_48PX));
					_pPortrait->SetVisible(true);
				}
			}

			SetIndex(userPersona.GetSearchIndex());
		}

		RefreshMeta();
		RefreshState();
		
		// Reset hover
		ResetHover();
	}

	EventResult UserPersonaListItem::OnEvent(fig::event& event)
	{
		switch (event.type)
		{
			case SDL_EVENT_MOUSE_BUTTON_UP:
				if (event.button.button == SDL_BUTTON_RIGHT
					and is_inside(GetRect(), toI(event.button.x), toI(event.button.y))
					and not _bHidden)
				{
					ShowMenu();
					return EventResult::Handled;
				}
				break;
		}

		if (IsUserEvent(event, UserEvent::MenuOpened))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = true;
				return EventResult::Continue;
			}
		}
		else if (IsUserEvent(event, UserEvent::MenuClosed))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = false;
				return EventResult::Continue;
			}
		}
		return EventResult::Pass;
	}
	
	void UserPersonaListItem::OnUpdate(float fElapsed)
	{
		bool bHovered = (_bSelected or is_inside(GetRect(), GetMousePos())
			and not GetOwnerFrame()->IsMenuShowing())
			and !_bHidden;

		if (_bHovered != bHovered)
		{
			_bHovered = bHovered;
			SetBackgroundColor(_bHovered ? Color::PanelBackgroundHover : Color::PanelBackground);
			GetBackgroundRenderer()->SetColor(_bHidden ? Color::CardShadow : GetBackgroundColor());
			_pMessage->SetBackgroundColor(GetBackgroundColor());
		}
	}

	void UserPersonaListItem::ShowMenu() noexcept
	{
		auto& menu = CreateMenu();

		bool bLLM = Global::IsLLMInitialized();

		menu.AddCheckItem("Set as default identity", _userSettings.flags.IsSet(AssetUserSettings::Flag::Default))
			.SetDelegate([this, bChecked = _userSettings.flags.IsSet(AssetUserSettings::Flag::Default)] {
				Global::GetUserContent().MarkDefault(_assetId, !bChecked);
				NotifyUpdated();
			});
		menu.AddSeparator();
		menu.AddItem("Edit identity\u2026", Icon::EDIT)
			.SetDelegate([this] {
				PushEvent(UserEvent::EditUserPersona, &_assetId);
			});
		menu.AddItem("Duplicate");
		menu.AddItem("Export\u2026");
		if (!_userSettings.HasFlag(AssetUserSettings::Flag::Hidden))
		{
			menu.AddItem("Hide")
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_assetId, true);
					_userSettings.flags.Set(AssetUserSettings::Flag::Hidden);
					NotifyHidden();
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([this] { 
					Global::GetUserContent().MarkHidden(_assetId, false);
					_userSettings.flags.Unset(AssetUserSettings::Flag::Hidden);
					NotifyHidden();
				});
		}
		menu.AddSeparator();
		menu.AddItem("Delete\u2026", Icon::DELETE)
			.SetDelegate([this] { NotifyDelete(); });
		
		_menuId = menu.Show();
	}

	void UserPersonaListItem::NotifyUpdated()
	{
		if (_fnDelegate)
			_fnDelegate(*this, UserPersonaListItemEvent::Refresh);
	}

	void UserPersonaListItem::NotifyHidden()
	{
		if (_fnDelegate)
			_fnDelegate(*this, UserPersonaListItemEvent::Hidden);
	}

	void UserPersonaListItem::NotifyDelete()
	{
		ShowDialogBox("Are you sure you want to delete this identity?\nThis action cannot be undone.", DialogBoxType::ConfirmCancel,
			[this](bool bOk) {
				if (_fnDelegate and bOk)
					_fnDelegate(*this, UserPersonaListItemEvent::Delete);
			});
	}

	void UserPersonaListItem::ShowDefault(bool bShow)
	{
		if (not _pDefaultIndicator)
		{
			_pDefaultIndicator = CreateControl<NineGridImage>(AppResources::GetTexture(Resource::CARD_TAG_BG), fig::corners { 16, 16, 13, 13 });
			_pDefaultIndicator->SetForegroundColor(Color::TagNew);

			auto pLabel = _pDefaultIndicator->CreateControl<StaticText>(fig::string { fig::strings::UI::Default }, FontFace::Default, 14.0, true);
			pLabel->SetPosition(6, 3);
			pLabel->SetForegroundColor(Color::White);
			pLabel->SetBackgroundColor(Color::Transparent);

			auto [w, h] = pLabel->MeasureText();
			_pDefaultIndicator->SetSize(w + 12, 26);
			_pDefaultIndicator->SetPosition(Width - _pDefaultIndicator->GetWidth() - 6, 6);
		}

		_pDefaultIndicator->SetVisible(bShow and !_bHidden);
		_pTitle->SetWidth((bShow ? _pDefaultIndicator->GetX() : Width) - _pTitle->GetX() - 8);
		_pTitle->InvalidateText();
	}
	
	void UserPersonaListItem::RefreshMeta()
	{
		auto& userContent = Global::GetUserContent();

		// Meta data
		if (auto try_meta = userContent.GetMetaData(_assetId))
		{
			auto& meta = *try_meta;
			_metaData = meta;
		}
		else // Error
		{
			auto now = fig::now();
			_metaData = ContentMetaData {
				.createdAt = now,
				.updatedAt = now,
				.lastUsedAt = now,
			};
		}

		_userSettings = Global::GetUserContent().GetUserSettings(_assetId);
		ShowDefault(_userSettings.HasFlag(AssetUserSettings::Flag::Default));
	}

	bool UserPersonaListItem::MatchesFlags(FilterFlags filter) const noexcept
	{
		if (filter.IsSet(FilterFlag::Hidden))
			return _userSettings.HasFlag(AssetUserSettings::Flag::Hidden);
		if (_userSettings.HasFlag(AssetUserSettings::Flag::Hidden))
			return false;

		if (filter.IsAnySet({ FilterFlag::GenderMale, FilterFlag::GenderFemale, FilterFlag::GenderOther })
			and not filter.IsSet({ FilterFlag::GenderMale, FilterFlag::GenderFemale, FilterFlag::GenderOther }))
		{
			auto [name, gender, _] = _metaData.gender.Get();
			switch (gender)
			{
			case ConventionalGender::Male:
				if (not filter.IsSet(FilterFlag::GenderMale))
					return false;
				break;
			case ConventionalGender::Female:
				if (not filter.IsSet(FilterFlag::GenderFemale))
					return false;
				break;
			case ConventionalGender::Nonbinary:
			case ConventionalGender::Hermaphrodite:
				if (not filter.IsSet(FilterFlag::GenderOther))
					return false;
				break;
			default:
				return false;
			}
		}

		if (filter.IsSet(FilterFlag::New) and not _metaData.IsNew())
			return false;

		return true;
	}

	bool UserPersonaListItem::MatchesSearch(const SearchQuery& query) const noexcept
	{
		return query.empty() or _searchIndex->Match(query);
	}

	void UserPersonaListItem::SetIndex(const SearchIndex& index) noexcept
	{
		_searchIndex = std::make_unique<SearchIndex>(index);
	}

	void UserPersonaListItem::SetHidden(bool bHidden) noexcept
	{
		_bHidden = bHidden;
		RefreshState();
	}

	void UserPersonaListItem::RefreshState()
	{
		if (_bHidden)
		{
			GetBackgroundRenderer()->SetColor(Color::CardShadow);
			SetBorderRenderer(nullptr);
		}
		else
		{
			GetBackgroundRenderer()->SetColor(Color::PanelBackground);
			auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
			pBorder->SetColor(Color::Border);
		}

		_pTitle->SetVisible(not _bHidden);
		_pPortrait->SetVisible(not _bHidden);
		_pMessage->SetVisible(not _bHidden);

		ShowDefault(_userSettings.HasFlag(AssetUserSettings::Flag::Default) and not _bHidden);
	}

	void UserPersonaListItem::ResetHover()
	{
		_bHovered = false;
		SetBackgroundColor(Color::PanelBackground);
		GetBackgroundRenderer()->SetColor(_bHidden ? Color::CardShadow : GetBackgroundColor());
		_pMessage->SetBackgroundColor(GetBackgroundColor());
	}
}