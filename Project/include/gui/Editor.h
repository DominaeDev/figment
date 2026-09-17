#pragma once

#include "EditorTab.h"

namespace fig::gui
{
	using EditorTabPtr = fig::observer_ptr<EditorTabBase>;

	class Editor : public Control
	{
		Editor() = delete;
	public:
		Editor(ControlPtr pParent);
		
		virtual void PopulateTopBar(ControlPtr pTopBar) {};
		virtual fig::string GetTitle() const = 0;

		virtual std::vector<EditorTabDescriptor> GetTabDescriptors() const = 0;
		std::vector<EditorTabPtr> GetTabs() const noexcept { return _tabs; }

		void SelectTab(size_t index);
		void Shutdown();
		
	protected:
		std::vector<EditorTabPtr> _tabs;

		template <typename T, typename... Args>
			requires std::derived_from<T, EditorTabBase>
		fig::observer_ptr<T> CreateTab(Args&&... args)
		{
			auto pTab = CreateControl<T>(std::forward<Args>(args)...);
			_tabs.push_back(pTab);
			EnableTab(pTab, false);
			_pTabSizer->Add(pTab, 0, SizerFlag::Expand);
			return pTab;
		}
		
		virtual void OnShutdown() noexcept = 0;
	private:
		void EnableTab(EditorTabBase* pTab, bool bEnabled);

	private:
		SizerPtr _pTabSizer;
	};
}