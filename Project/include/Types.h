#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <map>
#include <uuid_v4.h>

struct TTF_TextEngine;

typedef std::string string;
typedef SDL_FPoint Point;
typedef SDL_FRect Rect;
typedef UUIDv4::UUID uuid;

class Control;
class LayoutElement;
class Sizer;
struct llama_chat_message;

enum class Role
{
	System,
	Narrator,
	Director,
	User,
	Bot,
};

