#pragma once

#include "EditorPage.h"

namespace fig::gui
{
	using EditorPagePtr = fig::observer_ptr<EditorPage>;

	class Editor : public Control
	{
		Editor() = delete;
	public:
		Editor(ControlPtr pParent);
		
		virtual void Shutdown() = 0;
		virtual void PopulateTopBar(ControlPtr pTopBar) {};
		virtual fig::string GetTitle() const noexcept = 0;

		std::vector<EditorPagePtr> GetPages() const noexcept { return _pages; }

		void SelectPage(size_t index);
	
	protected:
		template <typename T, typename... Args>
			requires std::derived_from<T, EditorPage>
		fig::observer_ptr<T> CreatePage(Args&&... args)
		{
			auto pPage = CreateControl<T>(std::forward<Args>(args)...);
			_pages.push_back(pPage);
			EnablePage(pPage, false);
			_pPageSizer->Add(pPage, 0, SizerFlag::Expand);
			return pPage;
		}
		
		void EnablePage(EditorPage* pPage, bool bEnabled);

	private:
		std::vector<EditorPagePtr> _pages;
		SizerPtr _pPageSizer;
	};
}