#pragma once

#include "EditorFields.h"
#include "ComboBox.h"
#include "DropList.h"

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
		virtual EditorFields GetFields() noexcept { return {}; };

		virtual void Initialize() noexcept = 0;
		virtual void ShutDown() noexcept = 0;
		virtual void PopulateTopBar(ControlPtr pTopBar) {};

	protected:
		void CreateEditorField(ControlPtr pParent, const EditorField& field);
		void CreateEditorField(ControlPtr pParent, fig::observer_ptr<Sizer> pSizer, const EditorField& field);

		fig::observer_ptr<StaticText> CreateHeader(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<StaticText> CreateHint(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<StaticText> CreateLabel(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);

		template <typename T>
		fig::observer_ptr<class TextBox> CreateTextBox(ControlPtr pParent, SizerPtr pSizer, ValueBinding<T> binding)
		{
			return CreateTextBox(pParent, pSizer, binding, 1);
		};

		template <typename T>
		fig::observer_ptr<class TextBox> CreateTextBox(ControlPtr pParent, SizerPtr pSizer, ValueBinding<T> binding, int32_t rows) = delete;
		template <>
		fig::observer_ptr<class TextBox> CreateTextBox<fig::string>(ControlPtr pParent, SizerPtr pSizer, ValueBinding<fig::string> binding, int32_t rows);

		template <is_string_value_bindable T, is_string_range U>
		fig::observer_ptr<class ComboBox> CreateComboBox(ControlPtr pParent, SizerPtr pSizer, const U& items, ValueBinding<T> binding)
		{
			auto pControl = pParent->CreateControl<ComboBox>();
			pControl->AddItems(items);
			pControl->SetText(binding.AsString());
			pControl->SetTextChangedCallback([binding](const fig::string& text) mutable { binding.Set(text); });
			pSizer->Add(pControl, 0, SizerFlag::Expand, 0);
			return pControl;
		}

		template <is_int_value_bindable T, is_string_range U>
		fig::observer_ptr<DropList> CreateDropList(ControlPtr pParent, SizerPtr pSizer, const U& items, ValueBinding<T> binding)
		{
			auto pControl = pParent->CreateControl<DropList>();
			pControl->AddItems(items);
			pControl->Select(binding.AsInt());
			pControl->SetDelegate([binding](int32_t index) mutable { 
				binding.Set(index >= 0 ? index : 0); 
			});
			pSizer->Add(pControl, 0, SizerFlag::Expand, 0);
			return pControl;
		}
	};
}