#ifndef CONTROL_H__
#define CONTROL_H__

#include "Types.h"
#include "LayoutElement.h"

class CustomRenderer;

class Control : public LayoutElement
{
public:
	Control(Control* pParent);
	virtual ~Control();

	virtual void Render(Renderer* pRenderer);
	virtual void Update(float fDeltaTime) override;

	Color GetForegroundColor() const;
	Color GetBackgroundColor() const;
	bool GetClipping() const { return _bClipping; }
	virtual void SetForegroundColor(Color color) { _foregroundColor = color; }
	virtual void SetBackgroundColor(Color color) { _backgroundColor = color; }
	void SetBorderColor(Color color) { _borderColor = color; }
	void EnableClipping(bool bEnable) { _bClipping = bEnable; }
	void EnableCulling(bool bEnable) { _bCulling = bEnable; }

	bool GetVisible() { return _bVisible; }
	void SetVisible(bool bVisible) { _bVisible = bVisible; }

	bool ProcessEvent(SDL_Event* event);

	void SetBackgroundRenderer(CustomRenderer* pCustom);
	void SetBorderRenderer(CustomRenderer* pCustom);

protected:
	virtual void OnRender(Renderer* pRenderer);
	virtual void OnParent();
	virtual bool OnEvent(SDL_Event* event) { return false; }

	void DrawBackground(Renderer* pRenderer);
	void DrawBorder(Renderer* pRenderer);

protected:
	Color _foregroundColor {};
	Color _backgroundColor {};
	Color _borderColor {};
	bool _bClipping = false;
	bool _bCulling = false;
	bool _bVisible = true;
	
	// Theming
	CustomRenderer* _pBGRenderer = nullptr;
	CustomRenderer* _pBorderRenderer = nullptr;
};
#endif