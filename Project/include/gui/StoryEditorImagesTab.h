#pragma once

#include "gui/EditorTab.h"
#include "gui/StoryEditorArgs.h"

namespace fig::gui
{
	class GridSizer;
	class StoryPortraitWidget;
	class StorySmallPortraitWidget;
	class StoryBackgroundWidget;
	class NonOwningImageWithMask;

	class StoryEditorImagesTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorImagesTab(ControlPtr pParent);

		bool Initialize(StoryEditorArgs args) override;
		SaveResult OnSave() noexcept override;

	protected:
		void InitBackgrounds(SizerPtr pSizer);
		void ProcessLoadQueue();

		void OnUpdate(float fElapsed) override;

		void OnClickedBackground(ControlPtr pControl);
		void RemoveBackground(size_t index);
		void MoveBackgroundUp(size_t index);
		void MoveBackgroundDown(size_t index);

		void OpenFile();
		void OnOpenFile(const fig::path& filename);

		struct FileDialogUserData
		{
			StoryEditorImagesTab* pThis;
		} _fileDlgUserData {};
		static void SDLCALL OnFileDialogResult(void* userdata, const char* const* fileList, int filter);

	private:
		fig::uuid _scenarioId;
		std::vector<fig::uuid> _removedAssets;

		struct ImageWidget
		{
			fig::uuid assetId;
			fig::observer_ptr<Control> pControl;
			fig::sdl::Surface image;
			bool isCover = false;
			fig::io::DataFormat format = fig::io::DataFormat::Undefined;
			fig::bytes data {};
		};

		// Backgrounds
		fig::observer_ptr<GridSizer> _pBackgroundGridSizer;
		fig::observer_ptr<Control> _pAddBackgroundButton;
		std::queue<fig::path> _backgroundLoadQueue;
		std::vector<ImageWidget> _backgroundWidgets;
	};
}