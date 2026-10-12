#include <pch.h>
#include "gui/Frame.h"
#include "gui/Menu.h"
#include "gui/ModalOverlay.h"
#include "gui/GUITypes.h"
#include "gui/Window.h"
#include "gui/Events.h"
#include "gui/ColorTheme.h"

namespace fig::gui
{
	Frame::Frame(WindowPtr pHostWindow) : Control(nullptr, pHostWindow, this)
	{
		int w, h;
		SDL_GetWindowSizeInPixels(pHostWindow->GetSDLWindow().get(), &w, &h);
		SetSize(w, h);
	}

	Frame::~Frame()
	{
		PopAllOverlays();
		PopAllModals();
		Flush();
	}

	void Frame::Update(float fElapsed)
	{
		if (_modals.empty())
			Control::Update(fElapsed);

		for (int32_t i = toI(_modals.size()) - 1; i >= 0; --i)
		{
			auto& modal = _modals.at(toUZ(i));
			modal.ptr->Update(fElapsed);
		}

		for (int32_t i = toI(_overlays.size()) - 1; i >= 0; --i)
		{
			auto& overlay = _overlays.at(toUZ(i));
			overlay.ptr->Update(fElapsed);
		}

		if (AppColors::IsTransitioning())
			PushEvent(UserEvent::ColorThemeChanged);

		Flush();
	}

	void Frame::Render(fig::renderer_ptr pRenderer)
	{
		if constexpr (Debugging)
			SDL_SetRenderDrawColor(pRenderer, 0xFF, 0x00, 0xFF, 0xFF);
		else
			SDL_SetRenderDrawColor(pRenderer, 0x00, 0x00, 0x00, 0xFF);

		SDL_RenderClear(pRenderer);

		Control::Render(pRenderer);

		// Draw modals
		for (auto& modal : _modals)
			modal.ptr->Render(pRenderer);

		// Draw overlays
		for (auto& overlay : _overlays)
			overlay.ptr->Render(pRenderer);

		// Grab screen buffer
		if (not _snapshotPromises.empty())
		{
			for (auto& promise : _snapshotPromises)
			{
				auto surface = fig::sdl::Surface::from_ptr(SDL_RenderReadPixels(pRenderer, nullptr));
				promise.set_value(std::move(surface));
			}
			_snapshotPromises.clear();
		}

		SDL_RenderPresent(pRenderer);
	}

	Menu& Frame::CreateMenu() noexcept
	{
		return *new Menu(this);
	}

	int32_t Frame::PushOverlay(OverlayPtr pOverlay)
	{
		int32_t overlayId = ++_nextOverlayId;

		_overlays.push_back(OverlayInstance {
			.id = overlayId,
			.ptr = pOverlay,
		});

		// Clamp to edge
		auto& frameRect = GetRect();
		auto& overlayRect = pOverlay->GetRect();
		fig::point pos { overlayRect.x, overlayRect.y };

		if (overlayRect.x + overlayRect.w > frameRect.w)
		{
			if (_overlays.size() == 1)
				pos.x = frameRect.w - overlayRect.w;
			else
				pos.x = _overlays.at(_overlays.size() - 2).ptr->GetRect().x - overlayRect.w;
		}
		if (overlayRect.y < 0)
			pos.y = 0;
		if (overlayRect.y + overlayRect.h > frameRect.h)
			pos.y = frameRect.h - overlayRect.h;
		if (overlayRect.y < 0)
			pos.y = 0;

		if (pos.x != overlayRect.x or pos.y != overlayRect.y)
			pOverlay->SetAbsolutePosition(pos);

		OnOverlayOpen(overlayId);
		return overlayId;
	}

	void Frame::PopOverlay(OverlayPtr pOverlay)
	{
		std::vector<int32_t> removedIds;

		auto itFind = std::find_if(_overlays.begin(), _overlays.end(), [pOverlay](auto&& m) { return m.ptr == pOverlay; });
		while (itFind != _overlays.end())
		{
			removedIds.push_back((*itFind).id);
			_removalQueue.push_back((*itFind).ptr);
			itFind = _overlays.erase(itFind);
		}

		for (auto it = removedIds.crbegin(); it != removedIds.crend(); it++)
			OnOverlayClose(*it);
	}

	void Frame::PopAllOverlays()
	{
		std::vector<int32_t> removedIds;
		for (auto overlay : _overlays)
		{
			removedIds.push_back(overlay.id);
			_removalQueue.push_back(overlay.ptr);
		}
		_overlays.clear();

		for (auto it = removedIds.crbegin(); it != removedIds.crend(); it++)
			OnOverlayClose(*it);
	}

	int32_t Frame::PushModal(ModalPtr pOverlay)
	{
		int32_t overlayId = ++_nextOverlayId;

		_modals.push_back(ModalInstance {
			.id = overlayId,
			.ptr = pOverlay,
		});

		PopAllOverlays();
		OnOverlayOpen(overlayId);
		return overlayId;
	}

	void Frame::PopModal(ModalPtr pModal)
	{
		std::vector<int32_t> removedIds;

		auto itFind = std::ranges::find_if(_modals.begin(), _modals.end(), [pModal](auto&& m) { return m.ptr == pModal; });
		while (itFind != _modals.end())
		{
			removedIds.push_back((*itFind).id);
			_removalQueue.push_back((*itFind).ptr);
			itFind = _modals.erase(itFind);
		}

		for (auto it = removedIds.crbegin(); it != removedIds.crend(); it++)
			OnOverlayClose(*it);
	}

	void Frame::PopAllModals()
	{
		std::vector<int32_t> removedIds;
		for (auto modal : _modals)
		{
			removedIds.push_back(modal.id);
			_removalQueue.push_back(modal.ptr);
		}
		_modals.clear();

		for (auto it = removedIds.crbegin(); it != removedIds.crend(); it++)
			OnOverlayClose(*it);
	}

	EventResult Frame::ProcessEvent(fig::event& event)
	{
		for (int32_t i = toI(_overlays.size()) - 1; i >= 0; --i)
		{
			auto pOverlay = _overlays[toUZ(i)].ptr;
			if (pOverlay->ProcessEvent(event) == EventResult::Handled)
			{
				if (pOverlay->_bDestroyMe)
					PopOverlay(pOverlay);
				return EventResult::Handled;
			}
		}

		if (not _modals.empty())
		{
			for (int32_t i = toI(_modals.size()) - 1; i >= 0; --i)
			{
				auto pModal = _modals[toUZ(i)].ptr;
				if (pModal->ProcessEvent(event) == EventResult::Handled)
				{
					if (pModal->_bDestroyMe)
						PopAllOverlays();
					return EventResult::Handled;
				}
			}
			
			// Block all user input
			switch (event.type)
			{
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_MOTION:
			case SDL_EVENT_MOUSE_WHEEL:
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
			case SDL_EVENT_TEXT_EDITING:
			case SDL_EVENT_TEXT_INPUT:
			case SDL_EVENT_TEXT_EDITING_CANDIDATES:
				return EventResult::Handled; 
			}
		}

		if (event.type == SDL_EVENT_SYSTEM_THEME_CHANGED)
		{
			if (Global::IsSignedIn())
			{
				auto theme = Global::GetUserSettings().GetColorTheme();
				if (theme == ColorTheme::SystemDefault)
				{
					AppColors::SetTheme(ColorTheme::SystemDefault);
					PushEvent(UserEvent::ColorThemeChanged);
				}
			}
			else
			{
				AppColors::SetTheme(ColorTheme::SystemDefault);
				PushEvent(UserEvent::ColorThemeChanged);
			}
			return EventResult::Handled;
		}

		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && HandleMouseDown(event.button))
			return EventResult::Handled;
		
		if (IsUserEvent(event, UserEvent::PushCursor))
		{
			fig::cursor cursor = event.user.code;
			std::erase(_cursors, cursor);
			_cursors.push_back(cursor);
			RefreshCursor();
		}

		if (IsUserEvent(event, UserEvent::PopCursor))
		{
			fig::cursor cursor = event.user.code;
			std::erase(_cursors, cursor);
			RefreshCursor();
		}

		// Pass to self
		return OnEvent(event);
		// return Control::ProcessEvent(event);
	}

	bool Frame::HandleMouseDown(SDL_MouseButtonEvent& event)
	{
		auto fnIsInsideAnyOverlay = [this](const SDL_MouseButtonEvent& event) -> bool {
			int32_t mx = toI(event.x);
			int32_t my = toI(event.y);
			for (auto& overlay : _overlays)
			{
				auto rect = overlay.ptr->GetRect();
				if (is_inside(rect, mx, my))
					return true;
			}
			return false;
		};

		if (event.button == SDL_BUTTON_LEFT && !_overlays.empty())
		{
			if (!fnIsInsideAnyOverlay(event))
				PopAllOverlays();
		}
		else if (event.button == SDL_BUTTON_RIGHT && !_overlays.empty())
		{
			if (!fnIsInsideAnyOverlay(event))
				PopAllOverlays();
		}

		return false;
	}

	void Frame::OnOverlayOpen(int32_t overlayId)
	{
		PushEvent(UserEvent::MenuOpened, overlayId);
	}

	void Frame::OnOverlayClose(int32_t overlayId)
	{
		PushEvent(UserEvent::MenuClosed, overlayId);
	}

	void Frame::ResetCursor()
	{
		_cursors.clear();
		RefreshCursor();
	}

	void Frame::RefreshCursor()
	{
		if (_cursors.empty())
		{
			Global::SetCursor(Cursor::Default);
			return;
		}

		Global::SetCursor(_cursors.back());
	}

	void Frame::OnSize()
	{
		PopAllOverlays();

		for (auto modal : _modals)
			modal.ptr->SetSize(GetSize());
	}

	std::future<fig::sdl::Surface> Frame::GetSnapshot()
	{
		_snapshotPromises.emplace_back(std::promise<fig::sdl::Surface> {});
		auto future = _snapshotPromises.back().get_future();
		return future;
	}

	void Frame::Flush()
	{
		for (auto& pOverlay : _removalQueue)
			delete pOverlay;
		_removalQueue.clear();
	}
}