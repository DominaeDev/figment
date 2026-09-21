#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class IMeshControl : public Control
	{
	public:
		void SetTexture(fig::texture_ptr pTexture);
		void OnRender(fig::renderer_ptr pRenderer) override;
		void InvalidateMesh() noexcept;
	
	protected:
		IMeshControl(ControlPtr pParent);
		virtual void RefreshGeometry(const fig::rectf& rect) = 0;
		EventResult OnEvent(fig::event& event) override;

		void ClearMesh(size_t nVertices = 0uz, size_t nIndices = 0uz);
		void AddPoint(float x, float y, fig::colorf color = 0xFFFFFF_rgb);
		void AddPoint(float x, float y, float u, float v, fig::colorf color = 0xFFFFFF_rgb);
		void AddPoint(fig::pointf pos, fig::colorf color = 0xFFFFFF_rgb);
		void AddPoint(fig::pointf pos, fig::pointf uv, fig::colorf color = 0xFFFFFF_rgb);
		void AddQuad() noexcept;
		void AddQuad(int32_t p0, int32_t p1, int32_t p2, int32_t p3) noexcept;
		void AddTriangle() noexcept;
		void AddTriangle(int32_t p0, int32_t p1, int32_t p2) noexcept;
		static fig::pointf Rotate(fig::pointf vec, float theta) noexcept;
		static fig::pointf Normalize(fig::pointf v) noexcept;
		static fig::pointf Add(fig::pointf a, fig::pointf b) noexcept;
		static fig::pointf Multiply(fig::pointf v, float factor) noexcept;

		fig::texture_ptr _pTexture;
		std::vector<fig::vertex> _vertices {};
		std::vector<int32_t> _indices {};
		fig::rectf _lastRect {};
	};
}