#include <pch.h>
#include "gui/RoundedBorder.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	RoundedBorder::RoundedBorder(ControlPtr pParent, float radius, float thickness, fig::color color) : Control(pParent),
		_thickness(thickness),
		_radius(radius),
		_color(color)
	{
		_pTexture = AppResources::GetTexture(Resource::BORDER);
	}

	void RoundedBorder::OnRender(fig::renderer_ptr pRenderer)
	{
		auto rect = GetDrawRect();
		if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
			RefreshGeometry(rect);

		SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), _indices.data(), toI(_indices.size()));
	}

	void RoundedBorder::SetColor(fig::color color)
	{
		_color = color;
	}

	constexpr int32_t corner_triangles = 8;

	static void AddPoint(float x, float y, float u, float v, std::vector<fig::vertex>& vertices, fig::colorf color)
	{
		vertices.push_back(fig::vertex { fig::pointf { x, y }, color,  fig::pointf { u, v } });
	}

	static void AddQuad(int p0, int p1, int p2, int p3, std::vector<int>& indices)
	{
		indices.push_back(p0);
		indices.push_back(p1);
		indices.push_back(p2);
		indices.push_back(p0);
		indices.push_back(p2);
		indices.push_back(p3);
	}

	static void AddTriangle(int p0, int p1, int p2, std::vector<int>& indices)
	{
		indices.push_back(p0);
		indices.push_back(p1);
		indices.push_back(p2);
	}

	static fig::pointf Vec_Rotate(fig::pointf vec, float theta)
	{
		float sinTheta = SDL_sinf(-theta);
		float cosTheta = SDL_cosf(-theta);

		return fig::pointf {
			vec.x * cosTheta - vec.y * sinTheta,
			vec.x * sinTheta + vec.y * cosTheta,
		};
	}

	static fig::pointf Vec_Normalize(fig::pointf v)
	{
		float l = SDL_sqrtf(v.x * v.x + v.y * v.y);
		return fig::pointf { v.x / l, v.y / l };
	}

	static fig::pointf Vec_Add(fig::pointf a, fig::pointf b)
	{
		return fig::pointf { a.x + b.x, a.y + b.y };
	}

	static fig::pointf Vec_Multiply(fig::pointf v, float factor)
	{
		return fig::pointf { v.x * factor, v.y * factor };
	}

	// Takes an origin and a vector, then rotates that vector 90d, tracing a line with thickness
	static void AddCorner(float x0, float y0, float x1, float y1, float thickness, std::vector<fig::vertex>& vertices, std::vector<int>& indices, fig::colorf color)
	{
		float theta = SDL_PI_F / (2.0f * corner_triangles);
		fig::pointf v0 { x0, y0 };
		fig::pointf v1 { x1 - x0, y1 - y0 };
		fig::pointf v2 = Vec_Add(Vec_Multiply(Vec_Normalize(v1), -thickness), v1);

		int index = (int)vertices.size();

		for (int i = 0; i < corner_triangles; ++i)
		{
			fig::pointf p0 = Vec_Add(Vec_Rotate(v2, (float)(i + 1) * theta), v0);
			fig::pointf p1 = Vec_Add(Vec_Rotate(v2, (float)(i + 0) * theta), v0);
			fig::pointf p2 = Vec_Add(Vec_Rotate(v1, (float)(i + 0) * theta), v0);
			fig::pointf p3 = Vec_Add(Vec_Rotate(v1, (float)(i + 1) * theta), v0);

			AddPoint(p0.x, p0.y, 0.0f, 0.0f, vertices, color);
			AddPoint(p1.x, p1.y, 1.0f, 0.0f, vertices, color);
			AddPoint(p2.x, p2.y, 1.0f, 1.0f, vertices, color);
			AddPoint(p3.x, p3.y, 0.0f, 1.0f, vertices, color);

			AddQuad(index, index + 1, index + 2, index + 3, indices);
			index += 4;
		}
	}

	void RoundedBorder::RefreshGeometry(fig::rectf rect)
	{
		float radius = SDL_min(_radius, SDL_min(rect.w, rect.h) / 2.0f);

		if (radius <= 0.0f)
			return;

		fig::colorf color = {
			_color.r / 255.0f,
			_color.g / 255.0f,
			_color.b / 255.0f,
			1.0f,
		};

		_lastRect = rect;

		_vertices.clear();
		_indices.clear();
		_vertices.reserve(16 + corner_triangles * 16);
		_indices.reserve(3 * (16 + corner_triangles * 4));

		float outerLeft = rect.x;
		float outerRight = rect.x + rect.w;
		float outerTop = rect.y;
		float outerBottom = rect.y + rect.h;

		float innerLeft = rect.x + radius;
		float innerRight = rect.x + rect.w - radius - 0.1f;
		float innerTop = rect.y + radius;
		float innerBottom = rect.y + rect.h - radius - 0.1f;

		// Top
		AddPoint(innerLeft, outerTop + _thickness, 0.0f, 0.0f, _vertices, color);
		AddPoint(innerRight, outerTop + _thickness, 1.0f, 0.0f, _vertices, color);
		AddPoint(innerRight, outerTop, 1.0f, 1.0f, _vertices, color);
		AddPoint(innerLeft, outerTop, 0.0f, 1.0f, _vertices, color);
		AddQuad(0, 1, 2, 3, _indices);

		// Left
		AddPoint(outerLeft, innerBottom, 0.0f, 1.0f, _vertices, color);
		AddPoint(outerLeft + _thickness, innerBottom, 0.0f, 0.0f, _vertices, color);
		AddPoint(outerLeft + _thickness, innerTop, 1.0f, 0.0f, _vertices, color);
		AddPoint(outerLeft, innerTop, 1.0f, 1.0f, _vertices, color);
		AddQuad(4, 5, 6, 7, _indices);

		// Bottom
		AddPoint(innerLeft, outerBottom, 1.0f, 1.0f, _vertices, color);
		AddPoint(innerRight, outerBottom, 0.0f, 1.0f, _vertices, color);
		AddPoint(innerRight, outerBottom - _thickness, 0.0f, 0.0f, _vertices, color);
		AddPoint(innerLeft, outerBottom - _thickness, 1.0f, 0.0f, _vertices, color);
		AddQuad(8, 9, 10, 11, _indices);

		// Right
		AddPoint(outerRight - _thickness, innerBottom, 1.0f, 0.0f, _vertices, color);
		AddPoint(outerRight, innerBottom, 1.0f, 1.0f, _vertices, color);
		AddPoint(outerRight, innerTop, 0.0f, 1.0f, _vertices, color);
		AddPoint(outerRight - _thickness, innerTop, 0.0f, 0.0f, _vertices, color);
		AddQuad(12, 13, 14, 15, _indices);

		// Top-left corner
		AddCorner(innerLeft, innerTop, innerLeft, outerTop, _thickness, _vertices, _indices, color);
		// Bottom-left corner
		AddCorner(innerLeft, innerBottom, outerLeft, innerBottom, _thickness, _vertices, _indices, color);
		// Bottom-right corner
		AddCorner(innerRight, innerBottom, innerRight, outerBottom, _thickness, _vertices, _indices, color);
		// Top-right corner
		AddCorner(innerRight, innerTop, outerRight, innerTop, _thickness, _vertices, _indices, color);
	}

}