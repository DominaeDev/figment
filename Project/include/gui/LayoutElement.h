#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"
#include "IUpdateable.h"

namespace fig::gui
{
	using ControlPtr = fig::observer_ptr<class LayoutElement>;
	using ParentPtr = fig::observer_ptr<class LayoutElement>;

	class LayoutElement : public IUpdateable
	{
		friend class Sizer;
	protected:
		LayoutElement() = default;
	public:
		virtual ~LayoutElement();

		void Update(float fElapsed);

		inline const fig::rect& GetRect() const noexcept { return _rect; }
		inline constexpr fig::rectf GetDrawRect() const noexcept { return fig::rectf { toF(_rect.x), toF(_rect.y), toF(_rect.w), toF(_rect.h) }; }

		inline fig::coord GetX() const noexcept { return _localPosition.x; }
		inline fig::coord GetY() const noexcept { return _localPosition.y; }
		inline fig::point GetPosition() const noexcept { return fig::point { _localPosition.x, _localPosition.y }; }
		inline fig::coord GetAbsoluteX() const noexcept { return _rect.x; }
		inline fig::coord GetAbsoluteY() const noexcept { return _rect.y; }
		inline fig::point GetAbsolutePosition() const noexcept { return fig::point { _rect.x, _rect.y }; }

		inline fig::point GetSize() const noexcept { return fig::point { _rect.w, _rect.h }; }
		inline fig::coord GetWidth() const noexcept { return _rect.w; }
		inline fig::coord GetHeight() const noexcept { return _rect.h; }
		inline const fig::point& GetMinSize() const noexcept { return _minSize; }
		inline const fig::point& GetMaxSize() const noexcept { return _maxSize; }

		void SetRect(fig::rect rect);
		void SetRect(fig::coord x, fig::coord y, fig::coord width, fig::coord height);
		void SetAbsolutePosition(fig::coord x, fig::coord y);
		void SetAbsolutePosition(fig::point position);
		void SetPosition(fig::point position);
		void SetPosition(fig::coord x, fig::coord y);
		void SetX(fig::coord x);
		void SetY(fig::coord y);
		void SetSize(fig::point size);
		void SetSize(fig::coord width, fig::coord height);
		void SetWidth(fig::coord width);
		void SetHeight(fig::coord height);
		void Center();
		void CenterHorizontally();
		void CenterVertically();
		void FillParent();

		void SetMinSize(fig::point size) { _minSize = size; }
		void SetMinSize(fig::coord width, fig::coord height) { _minSize = fig::point { width, height }; }
		void SetMaxSize(fig::point size) { _maxSize = size; }
		void SetMaxSize(fig::coord width, fig::coord height) { _maxSize = fig::point { width, height }; }

		void AddChild(ControlPtr pChild);

		template <typename T, typename... Args> 
			requires std::derived_from<T, LayoutElement>
		fig::observer_ptr<T> CreateControl(Args&&... args)
		{
			T* child = new T(this, std::forward<Args>(args)...);
			AddChild(child);
			return child;
		}

		bool RemoveChild(ControlPtr pChild);
		bool RemoveChildren();
		bool DestroyChild(ControlPtr pChild);
		bool DestroyChildren();
		void MoveChildToTop(ControlPtr pChild);
		void MoveChildToBottom(ControlPtr pChild);

		template <typename T, typename... Args>
			requires std::derived_from<T, Sizer>
		fig::observer_ptr<T> SetSizer(Args&&... args)
		{
			_pSizer = std::make_unique<T>(std::forward<Args>(args)...);
			if ((bool)_pSizer)
				InvalidateLayout();
			return fig::observer_ptr<T>((T*)_pSizer.get());
		}

		void EnableLayout(bool bEnable) noexcept { _bLayoutEnabled = bEnable; }
		bool IsLayoutEnabled() const noexcept { return _bLayoutEnabled; }
		void InvalidateLayout();
		void LayoutNow();

		fig::observer_ptr<LayoutElement> GetParent() { return _pParent; }
		fig::observer_ptr<const LayoutElement> GetParent() const { return fig::observer_ptr<const LayoutElement>(_pParent); }

		inline void Cull(bool bCulled) { _bCulled = bCulled; }
		inline bool IsCulled() const { return _bCulled; }

	protected:
		void SetParent(ParentPtr pParent);
		void InvalidateParentLayout(bool bRefreshImmediately = false);
		fig::observer_ptr<Sizer> const GetSizer() const { return _pSizer.get(); }
		void Layout();

		virtual void OnUpdate(float fElapsed) {};
		virtual void OnParent() {};
		virtual void OnSize() {};
		virtual void OnAfterLayout() {};
		virtual void OnAddedChild(ControlPtr pChild) {}
		virtual void OnRemovedChild(ControlPtr pChild) {}

	protected:
		std::vector<LayoutElement*> _children;
		std::unique_ptr<Sizer> _pSizer {};
		fig::observer_ptr<LayoutElement> _pParent;
		bool _bCulled = false;
		bool _bLocalFromOrigin = false; // for render targets

	private:
		inline fig::coord GetOriginX() const noexcept { return _bLocalFromOrigin ? 0 : _rect.x; }
		inline fig::coord GetOriginY() const noexcept { return _bLocalFromOrigin ? 0 : _rect.y; }
		inline fig::rect GetSizerRect() const noexcept { return _bLocalFromOrigin ? fig::rect { 0, 0, _rect.w, _rect.h } : _rect; }

		void OnParentMoved();

		fig::rect _rect = {};
		fig::point _localPosition {};
		fig::point _minSize = {};
		fig::point _maxSize = {};
		bool _bLayoutEnabled = true;
		bool _bInvalidLayout = false;
	};

}