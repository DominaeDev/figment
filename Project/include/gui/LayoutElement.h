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

		virtual void Update(float fDeltaTime);

		Rectf& GetRect() { return _rect; }
		const Rectf& GetRect() const { return _rect; }
		Pointf GetPosition() const { return _position; }
		Pointf GetSize() const { return _size; }

		Pointf GetAbsolutePosition() const;
		float GetX() const { return _position.x; }
		float GetY() const { return _position.y; }
		float GetWidth() const { return _size.x; }
		float GetHeight() const { return _size.y; }
		const Pointf& GetMinSize() const { return _minSize; }
		const Pointf& GetMaxSize() const { return _maxSize; }

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

		void SetMinSize(Pointf size) { _minSize = size; }
		void SetMinSize(float width, float height) { _minSize = Pointf { width, height }; }
		void SetMaxSize(Pointf size) { _maxSize = size; }
		void SetMaxSize(float width, float height) { _maxSize = Pointf { width, height }; }

		void AddChild(LayoutElement* pChild);
		bool RemoveChild(LayoutElement* pChild);
		void MoveChildToTop(LayoutElement* pChild);
		void MoveChildToBottom(LayoutElement* pChild);

		void SetSizer(Sizer* sizer);
		void InvalidateLayout();

	protected:
		void SetParent(LayoutElement* pParent);
		void InvalidateParentLayout(bool bRefreshImmediately = false);
		void Layout();
		Sizer* const GetSizer() const { return _pSizer; }

		virtual void OnSize();
		virtual void OnParent();
		virtual void OnUpdate(float fDeltaTime) {};
		virtual void OnAfterLayout() {};
		virtual void OnAddedChild(LayoutElement* pChild) {}
		virtual void OnRemovedChild(LayoutElement* pChild) {}

	protected:
		std::vector<LayoutElement*> _children;
		LayoutElement* _pParent = nullptr;

		Rectf _rect = {};
		Pointf _position = {};
		Pointf _size = {};
		Pointf _minSize = {};
		Pointf _maxSize = {};
		bool _bInvalidLayout = false;

	private:
		Sizer* _pSizer = nullptr;
	};
}