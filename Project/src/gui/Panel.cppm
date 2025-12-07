export module Panel;

import Control;

export
{
	class Panel : public Control
	{
	public:
		Panel(Control* pParent) : Control(pParent)
		{}

	protected:
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override
		{
			DrawBackground(pRenderer);
		}
	};
}