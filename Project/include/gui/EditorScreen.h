#pragma once

#include "gui/Screen.h"
#include "gui/IEditor.h"

namespace fig::gui
{
	class ScrollPanel;

	class EditorScreen : public Screen
	{
	public:
		EditorScreen(Frame* pParent);

		template <typename T, typename... Args>
			requires std::derived_from<T, IEditor>
		fig::observer_ptr<T> SetEditor(Args&&... args)
		{
			_pEditor = std::make_unique<T>(std::forward<Args>(args)...);
			OnSetEditor();
			return fig::observer_ptr<T>((T*)_pEditor.get());
		}

		void ReleaseEditor();

	protected:
		void SetTitle(fig::string_view text);
		bool OnKeyboardEvent(KeyboardEvent& event);
		void OnSetEditor();

	private:
		fig::observer_ptr<StaticText> _pTitle {};
		fig::observer_ptr<ScrollPanel> _pScrollPanel {};
		std::unique_ptr<IEditor> _pEditor;
		EditorFields _fields {};
	};

	template <>
	constexpr ScreenType ScreenTypeOf<EditorScreen> = ScreenType::Editor;
}