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

		inline const Rect& GetRect() const noexcept { return _rect; }
		inline constexpr Rectf GetDrawRect() const noexcept { return Rectf { toF(_rect.x), toF(_rect.y), toF(_rect.w), toF(_rect.h) }; }

		inline Coord GetX() const noexcept { return _localPosition.x; }
		inline Coord GetY() const noexcept { return _localPosition.y; }
		inline Point GetPosition() const noexcept { return Point { _localPosition.x, _localPosition.y }; }
		inline Coord GetAbsoluteX() const noexcept { return _rect.x; }
		inline Coord GetAbsoluteY() const noexcept { return _rect.y; }
		inline Point GetAbsolutePosition() const noexcept { return Point { _rect.x, _rect.y }; }

		inline Point GetSize() const noexcept { return Point { _rect.w, _rect.h }; }
		inline Coord GetWidth() const noexcept { return _rect.w; }
		inline Coord GetHeight() const noexcept { return _rect.h; }
		inline const Point& GetMinSize() const noexcept { return _minSize; }
		inline const Point& GetMaxSize() const noexcept { return _maxSize; }

		void SetRect(Rect rect);
		void SetRect(Coord x, Coord y, Coord width, Coord height);
		void SetAbsolutePosition(Coord x, Coord y);
		void SetAbsolutePosition(Point position);
		void SetPosition(Point position);
		void SetPosition(Coord x, Coord y);
		void SetX(Coord x);
		void SetY(Coord y);
		void SetSize(Point size);
		void SetSize(Coord width, Coord height);
		void SetWidth(Coord width);
		void SetHeight(Coord height);
		void Center();
		void CenterHorizontally();
		void CenterVertically();
		void FillParent();

		void SetMinSize(Point size) { _minSize = size; }
		void SetMinSize(Coord width, Coord height) { _minSize = Point { width, height }; }
		void SetMaxSize(Point size) { _maxSize = size; }
		void SetMaxSize(Coord width, Coord height) { _maxSize = Point { width, height }; }

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
		inline Coord GetOriginX() const noexcept { return _bLocalFromOrigin ? 0 : _rect.x; }
		inline Coord GetOriginY() const noexcept { return _bLocalFromOrigin ? 0 : _rect.y; }
		inline Rect GetSizerRect() const noexcept { return _bLocalFromOrigin ? Rect { 0, 0, _rect.w, _rect.h } : _rect; }

		void OnParentMoved();

		Rect _rect = {};
		Point _localPosition {};
		Point _minSize = {};
		Point _maxSize = {};
		bool _bLayoutEnabled = true;
		bool _bInvalidLayout = false;
	};

}