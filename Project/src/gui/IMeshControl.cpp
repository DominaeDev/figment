#include <pch.h>
#include "gui/IMeshControl.h"

namespace fig::gui
{
	IMeshControl::IMeshControl(ControlPtr pParent) : Control(pParent)
	{
	}

	void IMeshControl::SetTexture(fig::texture_ptr pTexture)
	{
		_pTexture = pTexture;
	}

	void IMeshControl::OnRender(fig::renderer_ptr pRenderer)
	{
		auto rect = GetDrawRect();
		if (!SDL_RectsEqualFloat(&_lastRect, &rect))
		{
			_lastRect = rect;
			RefreshGeometry(rect);
		}

		if (not (_vertices.empty() or _indices.empty()))
		{
			if (_pTexture)
			{
				auto fgColor = GetForegroundColor();

				if (fgColor.IsDefined())
					SDL_SetTextureColorMod(_pTexture, fgColor.r(), fgColor.g(), fgColor.b());
				else
					SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);

				if (fgColor.IsDefined() && fgColor.a() != 0)
					SDL_SetTextureAlphaMod(_pTexture, fgColor.a());
				else
					SDL_SetTextureAlphaMod(_pTexture, 0xFF);
				
				SDL_SetTextureBlendMode( _pTexture, SDL_BLENDMODE_BLEND);
			}

			SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), _indices.data(), toI(_indices.size()));
		}
	}

	void IMeshControl::AddPoint(float x, float y, fig::colorf color)
	{
		_vertices.push_back(fig::vertex { fig::pointf { x, y }, color });
	}

	void IMeshControl::AddPoint(float x, float y, float u, float v, fig::colorf color)
	{
		_vertices.push_back(fig::vertex { fig::pointf { x, y }, color, fig::pointf { u, v } });
	}

	void IMeshControl::AddPoint(fig::pointf pos, fig::colorf color)
	{
		_vertices.push_back(fig::vertex { pos, color });
	}

	void IMeshControl::AddPoint(fig::pointf pos, fig::pointf uv, fig::colorf color)
	{
		_vertices.push_back(fig::vertex { pos, color,  uv });
	}

	void IMeshControl::AddQuad() noexcept
	{
		if (_vertices.size() < 4uz)
			return; // Error

		int32_t index = toI(_vertices.size()) - 4;
		AddQuad(index, index + 1, index + 2, index + 3);
	}

	void IMeshControl::AddQuad(int32_t p0, int32_t p1, int32_t p2, int32_t p3) noexcept
	{
		_indices.push_back(p0);
		_indices.push_back(p1);
		_indices.push_back(p2);
		_indices.push_back(p0);
		_indices.push_back(p2);
		_indices.push_back(p3);
	}

	void IMeshControl::AddTriangle() noexcept
	{
		if (_vertices.size() < 3uz)
			return; // Error
		int32_t index = toI(_vertices.size()) - 3;
		AddTriangle(index, index + 1, index + 2);
	}

	void IMeshControl::AddTriangle(int32_t p0, int32_t p1, int32_t p2) noexcept
	{
		_indices.push_back(p0);
		_indices.push_back(p1);
		_indices.push_back(p2);
	}

	fig::pointf IMeshControl::Rotate(fig::pointf vec, float theta) noexcept
	{
		float sinTheta = SDL_sinf(-theta);
		float cosTheta = SDL_cosf(-theta);

		return fig::pointf {
			vec.x * cosTheta - vec.y * sinTheta,
			vec.x * sinTheta + vec.y * cosTheta,
		};
	}

	fig::pointf IMeshControl::Normalize(fig::pointf v) noexcept
	{
		float l = SDL_sqrtf(v.x * v.x + v.y * v.y);
		return fig::pointf { v.x / l, v.y / l };
	}

	fig::pointf IMeshControl::Add(fig::pointf a, fig::pointf b) noexcept
	{
		return fig::pointf { a.x + b.x, a.y + b.y };
	}

	fig::pointf IMeshControl::Multiply(fig::pointf v, float factor) noexcept
	{
		return fig::pointf { v.x * factor, v.y * factor };
	}

	void IMeshControl::ClearMesh(size_t nVertices, size_t nIndices)
	{
		_vertices.clear();
		_indices.clear();
		if (nVertices > 0uz)
			_vertices.reserve(nVertices);
		if (nIndices > 0uz)
			_indices.reserve(nVertices);
	}

	void IMeshControl::InvalidateMesh() noexcept
	{
		_lastRect = {};
	}

	EventResult IMeshControl::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::ColorThemeChanged))
		{
			InvalidateMesh();
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}
}