#pragma once

#include "gui/ValueBinding.h"

namespace fig::gui
{
	class IEditorField
	{
	public:
		IEditorField(fig::string_view label) noexcept : 
			_label { label }
		{}

		virtual ~IEditorField()
		{}

		fig::string_view GetLabel() const noexcept
		{ 
			return _label;
		}

		virtual ControlPtr CreateControl(ControlPtr pParent) = 0;

	protected:
		fig::string _label;
	};

	class TextBox;

	template <typename T>
	class EditorTextField : public IEditorField
	{
	public:
		EditorTextField(T* pMember, fig::string_view label, fig::coord maxWidth = 0) noexcept : IEditorField(label),
			_binding { pMember },
			_maxWidth { maxWidth }
		{}

		ControlPtr CreateControl(ControlPtr pParent) override;

	protected:
		void OnChange(const fig::string& text);

		ValueBinding<T> _binding;
		fig::observer_ptr<TextBox> _pTextBox;
		fig::coord _maxWidth {};
	};

	struct EditorHeader
	{
		EditorHeader(fig::string_view text) : label { text }
		{}

		fig::string label;
	};

	using EditorFieldEntry = std::variant<std::shared_ptr<IEditorField>, std::shared_ptr<EditorHeader>>;
	using EditorFields = std::vector<EditorFieldEntry>;

}