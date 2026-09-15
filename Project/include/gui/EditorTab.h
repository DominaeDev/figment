#pragma once

#include "gui/ComboBox.h"
#include "gui/DropList.h"
#include "gui/ValueBinding.h"

namespace fig::gui
{
	struct EditorTabDescriptor
	{
		size_t tabIndex;
		fig::string label;
		Resource iconLarge;
		Resource iconSmall;
	};

	class HorizontalLine;

	class EditorTabBase : public Control
	{
	public:
		EditorTabBase(ControlPtr pParent) : Control(pParent)
		{
		}

		using SaveResult = std::expected<void, std::runtime_error>;
		virtual SaveResult OnSave() noexcept { return {}; };

	protected:
		fig::observer_ptr<StaticText> CreateHeader(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<StaticText> CreateHint(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<StaticText> CreateLabel(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<HorizontalLine> CreateHorizontalLine(ControlPtr pParent, SizerPtr pSizer);

		template <typename T>
		fig::observer_ptr<class TextBox> CreateTextBox(ControlPtr pParent, SizerPtr pSizer, ValueBinding<T> binding)
		{
			return CreateTextBox(pParent, pSizer, binding, 1);
		};

		fig::observer_ptr<class TextBox> CreateTextBox(ControlPtr pParent, SizerPtr pSizer);

		template <typename T>
		fig::observer_ptr<class TextBox> CreateTextBox(ControlPtr pParent, SizerPtr pSizer, ValueBinding<T> binding, int32_t rows) = delete;
		template <>
		fig::observer_ptr<class TextBox> CreateTextBox<fig::string>(ControlPtr pParent, SizerPtr pSizer, ValueBinding<fig::string> binding, int32_t rows);

		template <is_string_range U>
		fig::observer_ptr<class ComboBox> CreateComboBox(ControlPtr pParent, SizerPtr pSizer, const U& items)
		{
			auto pControl = pParent->CreateControl<ComboBox>();
			pControl->AddItems(items);
			pSizer->Add(pControl, 0, SizerFlag::Expand, 0);
			return pControl;
		}

		template <is_string_value_bindable T, is_string_range U>
		fig::observer_ptr<class ComboBox> CreateComboBox(ControlPtr pParent, SizerPtr pSizer, const U& items, ValueBinding<T> binding)
		{
			auto pControl = pParent->CreateControl<ComboBox>();
			pControl->AddItems(items);
			pControl->SetText(binding.AsString());
			pControl->SetTextChangedDelegate([binding](fig::string_view text) mutable { binding.Set(fig::string { text }); });
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

	template <typename TArgs>
	class EditorTab : public EditorTabBase
	{
	public:
		EditorTab(ControlPtr pParent) : EditorTabBase(pParent)
		{}

		virtual bool Initialize(TArgs args) = 0;
	};
}