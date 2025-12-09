export module GUI.GraphicTypes;

export import <SDL3/SDL.h>;
export import <SDL3_image/SDL_image.h>;

export import GUI.Colors;

export
{
	// SDL types
	using Pointf = SDL_FPoint;
	using Point = SDL_Point;
	using Rectf = SDL_FRect;
	using Rect = SDL_Rect;
	using Renderer = SDL_Renderer;
	using Texture = SDL_Texture;
	using Vertex = SDL_Vertex;
	using Surface = SDL_Surface;
	using Window = SDL_Window;
	using WindowID = SDL_WindowID;
	using Event = SDL_Event;

	namespace gui_util
	{
		inline constexpr Rect expand_rect(const Rect& rect, int pixels)
		{
			return Rect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
		}

		inline constexpr Rectf expand_rect(const Rectf& rect, float pixels)
		{
			return Rectf { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
		}

		inline constexpr Rect to_rect(Rectf rect)
		{
			return Rect {
				(int32_t)rect.x,
				(int32_t)rect.y,
				(int32_t)rect.w,
				(int32_t)rect.h
			};
		}

		inline constexpr Rectf to_rectf(Rect rect)
		{
			return Rectf {
				(float)rect.x,
				(float)rect.y,
				(float)rect.w,
				(float)rect.h
			};
		}
	}
}