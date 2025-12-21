#include <pch.h>
#include "gui/RoundedBackgroundRenderer.h"

using namespace fig::gui;

RoundedBackgroundRenderer::RoundedBackgroundRenderer(float radius, Color color) :
	_radius(radius),
	_color(color)
{
}

void RoundedBackgroundRenderer::Render(Renderer* pRenderer, Rectf rect)
{
	if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
		RefreshGeometry(rect);

	SDL_RenderGeometry(pRenderer, nullptr, _vertices.data(), toI(_vertices.size()), _indices.data(), toI(_indices.size()));
}

void RoundedBackgroundRenderer::SetColor(Color color)
{
	_color = color;
}

#define CORNER_TRIANGLES 2

static void AddPoint(float x, float y, std::vector<Vertex>& vertices, Colorf color)
{
	vertices.push_back(Vertex { Pointf { x, y }, color });
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

static Pointf Rotate(Pointf vec, float theta)
{
	float sinTheta = SDL_sinf(-theta);
	float cosTheta = SDL_cosf(-theta);

	return Pointf {
		vec.x * cosTheta - vec.y * sinTheta,
		vec.x * sinTheta + vec.y * cosTheta,
	};
}

static void AddCorner(float x0, float y0, float x1, float y1, std::vector<Vertex>& vertices, std::vector<int>& indices, Colorf color)
{
	float theta = SDL_PI_F / (2.0f * CORNER_TRIANGLES);
	Pointf v0 { x1 - x0, y1 - y0 };
	int index = (int)vertices.size();

	for (int i = 0; i < CORNER_TRIANGLES; ++i)
	{
		Pointf p0 { x0, y0 };
		Pointf p1 = Rotate(v0, (float)(i + 0) * theta);
		Pointf p2 = Rotate(v0, (float)(i + 1) * theta);
		AddPoint(p0.x, p0.y, vertices, color);
		AddPoint(p0.x + p1.x, p0.y + p1.y, vertices, color);
		AddPoint(p0.x + p2.x, p0.y + p2.y, vertices, color);
		AddTriangle(index, index + 1, index + 2, indices);
		index += 3;
	}
}

void RoundedBackgroundRenderer::RefreshGeometry(Rectf rect)
{
	float radius = SDL_min(_radius, SDL_min(rect.w, rect.h) / 2.0f);

	if (radius <= 0.0f)
		return;

	Colorf color = {
		_color.r / 255.0f,
		_color.g / 255.0f,
		_color.b / 255.0f,
		1.0f,
	};

	_lastRect = rect;

	_vertices.clear();
	_indices.clear();
	_vertices.reserve(12 + (CORNER_TRIANGLES - 2) * 4);
	_indices.reserve(3 * (20 + CORNER_TRIANGLES * 4));
	
	float outerLeft = rect.x;
	float outerRight = rect.x + rect.w;
	float outerTop = rect.y;
	float outerBottom = rect.y + rect.h;

	float innerLeft = rect.x + radius;
	float innerRight = rect.x + rect.w - radius;
	float innerTop = rect.y + radius;
	float innerBottom = rect.y + rect.h - radius;

	// Center
	AddPoint(innerLeft, innerBottom, _vertices, color);
	AddPoint(innerRight, innerBottom, _vertices, color);
	AddPoint(innerRight, innerTop, _vertices, color);
	AddPoint(innerLeft, innerTop, _vertices, color);
	AddQuad(0, 1, 2, 3, _indices);

	// Top
	AddPoint(innerLeft, innerTop, _vertices, color);
	AddPoint(innerRight, innerTop, _vertices, color);
	AddPoint(innerRight, outerTop, _vertices, color);
	AddPoint(innerLeft, outerTop, _vertices, color);
	AddQuad(4, 5, 6, 7, _indices);

	// Left
	AddPoint(outerLeft, innerBottom, _vertices, color);
	AddPoint(innerLeft, innerBottom, _vertices, color);
	AddPoint(innerLeft, innerTop, _vertices, color);
	AddPoint(outerLeft, innerTop, _vertices, color);
	AddQuad(8, 9, 10, 11, _indices);

	// Bottom
	AddPoint(innerLeft, outerBottom, _vertices, color);
	AddPoint(innerRight, outerBottom, _vertices, color);
	AddPoint(innerRight, innerBottom, _vertices, color);
	AddPoint(innerLeft, innerBottom, _vertices, color);
	AddQuad(12, 13, 14, 15, _indices);

	// Right
	AddPoint(innerRight, innerBottom, _vertices, color);
	AddPoint(outerRight, innerBottom, _vertices, color);
	AddPoint(outerRight, innerTop, _vertices, color);
	AddPoint(innerRight, innerTop, _vertices, color);
	AddQuad(16, 17, 18, 19, _indices);

	// Top-left corner
	AddCorner(innerLeft, innerTop, innerLeft, outerTop, _vertices, _indices, color);
	// Bottom-left corner
	AddCorner(innerLeft, innerBottom, outerLeft, innerBottom, _vertices, _indices, color);
	// Bottom-right corner
	AddCorner(innerRight, innerBottom, innerRight, outerBottom, _vertices, _indices, color);
	// Top-right corner
	AddCorner(innerRight, innerTop, outerRight, innerTop, _vertices, _indices, color);
}