#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <map>

struct TTF_TextEngine;

typedef std::string string;
typedef SDL_FPoint Point;
typedef SDL_FRect Rect;

class Control;
class LayoutElement;
class Sizer;

struct llama_chat_message;

enum class Role
{
	System,
	Narrator,
	User,
	Bot,
};

struct Message 
{
	Role role;
    string content;
    string name;
};
