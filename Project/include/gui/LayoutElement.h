#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include "IUpdateable.h"

namespace fig::gui
{
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

		void AddChild(LayoutElement* pChild);
		bool RemoveChild(LayoutElement* pChild);
		bool RemoveChildren();
		bool DestroyChild(LayoutElement* pChild);
		bool DestroyChildren();
		void MoveChildToTop(LayoutElement* pChild);
		void MoveChildToBottom(LayoutElement* pChild);

		void SetSizer(Sizer* sizer);

		void EnableLayout(bool bEnable) noexcept { _bLayoutEnabled = bEnable; }
		bool IsLayoutEnabled() const noexcept { return _bLayoutEnabled; }
		void InvalidateLayout();
		void LayoutNow();

		inline LayoutElement* GetParent() { return _pParent; }
		inline constexpr LayoutElement* GetParent() const { return _pParent; }

		inline void Cull(bool bCulled) { _bCulled = bCulled; }
		inline bool IsCulled() const { return _bCulled; }

	protected:
		void SetParent(LayoutElement* pParent);
		void InvalidateParentLayout(bool bRefreshImmediately = false);
		Sizer* const GetSizer() const { return _pSizer.get(); }
		void Layout(const Rect* const pRect = nullptr);

		virtual void OnUpdate(float fElapsed) {};
		virtual void OnParent() {};
		virtual void OnSize() {};
		virtual void OnAfterLayout() {};
		virtual void OnAddedChild(LayoutElement* pChild) {}
		virtual void OnRemovedChild(LayoutElement* pChild) {}

	protected:
		std::vector<LayoutElement*> _children;
		std::unique_ptr<Sizer> _pSizer {};
		LayoutElement* _pParent = nullptr;
		bool _bCulled = false;

	private:
		void OnParentMoved();

		Rect _rect = {};
		Point _localPosition {};
		Point _minSize = {};
		Point _maxSize = {};
		bool _bLayoutEnabled = true;
		bool _bInvalidLayout = false;
	};
}