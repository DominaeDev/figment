#pragma once

#include "gui/Overlay.h"
#include <future>
#include <thread>

namespace fig::gui
{
	using MenuDelegate = std::function<void()>;

	class ModalOverlay : public Overlay
	{
	public:
		ModalOverlay(FramePtr pFrame);

		int32_t Show();
		void Close();

	protected:
		void BlurBackground();

		void OnUpdate(float fElapsed) override;
		void OnSize() override;

	private:
		int32_t _overlayId {};
		fig::sdl::Texture _pBGTexture;
		fig::observer_ptr<Image> _pBGImage;
		fig::observer_ptr<Panel> _pBGTint;
		std::future<fig::sdl::Surface> _future;

		bool _bWaitOnBlur {};
		float _fBlurFade {};
		std::jthread _blurWorker;
		std::atomic<fig::surface_ptr> _pBlurResult {};
		
		void Blur(fig::sdl::Surface&& surface, float sigma);
	};
}
