#include "RoundedBackgroundRenderer.h"
#include "Utility.h"

RoundedBackgroundRenderer::RoundedBackgroundRenderer(float radius, float contract, SDL_Color color) :
	_radius(radius),
	_contract(contract),
	_color(color)
{
}

void RoundedBackgroundRenderer::Draw(SDL_Renderer* pRenderer, SDL_FRect rect)
{
	if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
		RefreshGeometry(rect);

	SDL_RenderGeometry(pRenderer, nullptr, _vertices.data(), toI(_vertices.size()), _indices.data(), toI(_indices.size()));
}

void RoundedBackgroundRenderer::SetColor(SDL_Color color)
{
	_color = color;
}

#define CORNER_TRIANGLES 2

static void AddPoint(float x, float y, std::vector<SDL_Vertex>& vertices, SDL_FColor color)
{
	vertices.push_back(SDL_Vertex { SDL_FPoint { x, y }, color });
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

static SDL_FPoint Rotate(SDL_FPoint vec, float theta)
{
	float sinTheta = SDL_sinf(-theta);
	float cosTheta = SDL_cosf(-theta);

	return SDL_FPoint {
		vec.x * cosTheta - vec.y * sinTheta,
		vec.x * sinTheta + vec.y * cosTheta,
	};
}

static void AddCorner(float x0, float y0, float x1, float y1, std::vector<SDL_Vertex>& vertices, std::vector<int>& indices, SDL_FColor color)
{
	float theta = SDL_PI_F / (2.0f * CORNER_TRIANGLES);
	SDL_FPoint v0 { x1 - x0, y1 - y0 };
	int index = (int)vertices.size();

	for (int i = 0; i < CORNER_TRIANGLES; ++i)
	{
		SDL_FPoint p0 { x0, y0 };
		SDL_FPoint p1 = Rotate(v0, (float)(i + 0) * theta);
		SDL_FPoint p2 = Rotate(v0, (float)(i + 1) * theta);
		AddPoint(p0.x, p0.y, vertices, color);
		AddPoint(p0.x + p1.x, p0.y + p1.y, vertices, color);
		AddPoint(p0.x + p2.x, p0.y + p2.y, vertices, color);
		AddTriangle(index, index + 1, index + 2, indices);
		index += 3;
	}
}

void RoundedBackgroundRenderer::RefreshGeometry(SDL_FRect rect)
{
	float radius = SDL_min(_radius + _contract, SDL_min(rect.w, rect.h) / 2.0f);

	if (radius <= 0.0f)
		return;

	SDL_FColor color = {
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
	
	float outerLeft = rect.x + _contract;
	float outerRight = rect.x + rect.w - _contract;
	float outerTop = rect.y + _contract;
	float outerBottom = rect.y + rect.h - _contract;

	float innerLeft = rect.x + radius + _contract;
	float innerRight = rect.x + rect.w - radius - _contract;
	float innerTop = rect.y + radius + _contract;
	float innerBottom = rect.y + rect.h - radius - _contract;

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