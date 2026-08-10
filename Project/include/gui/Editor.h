#pragma once

#include "EditorFields.h"

namespace fig::gui
{
	class Editor : public Control
	{
	public:
		Editor(ControlPtr pParent) : Control(pParent)
		{}
		virtual ~Editor()
		{};

		virtual fig::string GetTitle() const noexcept = 0;
		virtual EditorFields GetFields() noexcept = 0;

		virtual void Initialize() noexcept = 0;
		virtual bool SaveChanges() noexcept = 0;

	protected:
		void CreateEditorField(ControlPtr pParent, const EditorField& field);
		void CreateEditorField(ControlPtr pParent, fig::observer_ptr<Sizer> pSizer, const EditorField& field);

		fig::observer_ptr<StaticText> CreateHeader(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<StaticText> CreateHint(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<StaticText> CreateLabel(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);

		template <typename T>
		fig::observer_ptr<class SimpleTextBox> CreateTextBox(ControlPtr pParent, SizerPtr pSizer, ValueBinding<T> binding) = delete;
		template <>
		fig::observer_ptr<class SimpleTextBox> CreateTextBox<fig::string>(ControlPtr pParent, SizerPtr pSizer, ValueBinding<fig::string> binding);
	};
}