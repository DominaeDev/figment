export module GUI.Layout.LayoutElement;

import Common;
export import GUI.GraphicTypes;
export import GUI.Layout.IUpdateable;

extern "C++" class Sizer;
extern "C++" class LayoutElement;

export
{
	extern "C++" class LayoutElement : public IUpdateable
	{
	public:
		virtual ~LayoutElement();

		virtual void Update(float fDeltaTime);

		Rectf& GetRect() { return _rect; }
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

	extern "C++" class Sizer : public IUpdateable
	{
	public:
		enum Flag : int {
			None = 0,
			Expand = 1 << 0,
			Top = 1 << 1,
			Bottom = 1 << 2,
			Left = 1 << 3,
			Right = 1 << 4,

			AlignLeft = 1 << 10,
			AlignRight = 1 << 11,
			AlignTop = 1 << 12,
			AlignBottom = 1 << 13,
			AlignCenterHorizontal = 1 << 14,
			AlignCenterVertical = 1 << 15,

			All = Top | Bottom | Left | Right,
			Default = None,
		};

	public:
		void Layout();
		void SetOwner(LayoutElement* pOwner);

		void Add(LayoutElement* pControl, int proportion = 0, int flags = Flag::Default, int border = 0);
		void AddStretchSpacer();
		void Remove(LayoutElement* pControl);
		void Clear();

	protected:
		struct LayoutInfo
		{
			LayoutElement* pControl;
			int prop = 0;
			int flags = Flag::None;
			int border = 0;
		};

		LayoutElement* _pOwner = nullptr;
		std::vector<LayoutInfo> _items;

		unsigned int GetCount() const { return static_cast<unsigned int>(_items.size()); }

		virtual void OnLayout(Rectf rect) = 0;
		void Update(float fDeltaTime) override {}
	};
}