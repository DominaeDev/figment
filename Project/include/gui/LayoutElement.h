#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include "IUpdateable.h"

namespace fig::gui
{
	class Sizer;

	class LayoutElement : public IUpdateable
	{
	protected:
		LayoutElement() = default;
	public:
		virtual ~LayoutElement();

		virtual void Update(float fElapsed);

		inline Rectf& GetRect() noexcept { return _rect; }
		inline const Rectf& GetRect() const noexcept { return _rect; }
		inline const Pointf& GetPosition() const noexcept { return _position; }
		inline const Pointf& GetSize() const noexcept { return _size; }

		inline Pointf GetAbsolutePosition() const noexcept;
		inline float GetX() const noexcept { return _position.x; }
		inline float GetY() const noexcept { return _position.y; }
		inline float GetWidth() const noexcept { return _size.x; }
		inline float GetHeight() const noexcept { return _size.y; }
		inline const Pointf& GetMinSize() const noexcept { return _minSize; }
		inline const Pointf& GetMaxSize() const noexcept { return _maxSize; }

		void SetRect(Rectf rect);
		void SetRect(float x, float y, float width, float height);
		void SetPosition(Pointf position);
		void SetPosition(float x, float y);
		void SetX(float x);
		void SetY(float y);
		void SetSize(Pointf size);
		void SetSize(float width, float height);
		void SetWidth(float width);
		void SetHeight(float height);
		void Center();
		void CenterHorizontally();
		void CenterVertically();

		void SetMinSize(Pointf size) { _minSize = size; }
		void SetMinSize(float width, float height) { _minSize = Pointf { width, height }; }
		void SetMaxSize(Pointf size) { _maxSize = size; }
		void SetMaxSize(float width, float height) { _maxSize = Pointf { width, height }; }

		void AddChild(LayoutElement* pChild);
		bool RemoveChild(LayoutElement* pChild);
		bool RemoveChildren(bool destroy = false);
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
		void Layout();
		Sizer* const GetSizer() const { return _pSizer; }

		virtual void OnSize();
		virtual void OnParent();
		virtual void OnUpdate(float fElapsed) {};
		virtual void OnAfterLayout() {};
		virtual void OnAddedChild(LayoutElement* pChild) {}
		virtual void OnRemovedChild(LayoutElement* pChild) {}

	protected:
		std::vector<LayoutElement*> _children;
		LayoutElement* _pParent = nullptr;
		Sizer* _pSizer = nullptr;
		bool _bCulled = false;

		Rectf _rect = {};
		Pointf _position = {};
		Pointf _size = {};
		Pointf _minSize = {};
		Pointf _maxSize = {};
		bool _bLayoutEnabled = true;
		bool _bInvalidLayout = false;
	};
}