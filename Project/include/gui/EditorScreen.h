#pragma once

#include "gui/Screen.h"
#include "gui/Editor.h"
#include "gui/ScrollPanel.h"

namespace fig::gui
{
	class ScrollPanel;

	class EditorScreen : public Screen
	{
	public:
		EditorScreen(Frame* pParent);
		~EditorScreen();

		template <typename T, typename... Args>
			requires std::derived_from<T, Editor>
		fig::observer_ptr<T> SetEditor(Args&&... args)
		{
			_pEditor = std::make_unique<T>(this, std::forward<Args>(args)...);
			OnSetEditor();
			return fig::observer_ptr<T>((T*)_pEditor.get());
		}

		template <typename T, typename... Args>
			requires std::derived_from<T, Editor>
		fig::observer_ptr<T> GetEditor() const noexcept
		{
			return _pEditor.get();
		}

		fig::observer_ptr<Editor> GetEditor() const noexcept
		{
			return _pEditor.get();
		}

		void ReleaseEditor();

	protected:
		void SetTitle(fig::string_view text);
		void OnSetEditor();

		bool OnKeyboardEvent(KeyboardEvent& event) override;
		EventResult OnEvent(fig::event& event) override;

	private:
		std::unique_ptr<Editor> _pEditor;

		fig::observer_ptr<StaticText> _pTitle {};
		fig::observer_ptr<ScrollPanel> _pScrollPanel {};
		fig::observer_ptr<class TopBar> _pTopBar;
	};

	template <>
	constexpr ScreenType ScreenTypeOf<EditorScreen> = ScreenType::EditorPage;
}