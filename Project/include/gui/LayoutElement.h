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

		void Update(float fElapsed);
		void Layout();

		inline const Rectf& GetRect() const noexcept { return _rect; }

		inline float GetX() const noexcept { return _localPosition.x; }
		inline float GetY() const noexcept { return _localPosition.y; }
		inline Pointf GetPosition() const noexcept { return Pointf { _localPosition.x, _localPosition.y }; }
		inline float GetAbsoluteX() const noexcept { return _rect.x; }
		inline float GetAbsoluteY() const noexcept { return _rect.y; }
		inline Pointf GetAbsolutePosition() const noexcept { return Pointf { _rect.x, _rect.y }; }

		inline Pointf GetSize() const noexcept { return Pointf { _rect.w, _rect.h }; }
		inline float GetWidth() const noexcept { return _rect.w; }
		inline float GetHeight() const noexcept { return _rect.h; }
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
		void FillParent();

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
		Sizer* const GetSizer() const { return _pSizer.get(); }

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

		Rectf _rect = {};
		Pointf _localPosition {};
		Pointf _minSize = {};
		Pointf _maxSize = {};
		bool _bLayoutEnabled = true;
		bool _bInvalidLayout = false;
	};
}