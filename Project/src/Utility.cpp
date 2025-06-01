#include "Utility.h"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <fstream>

SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels)
{
	return SDL_FRect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
}

string LoadTextFile(const string& filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	char* buffer = new char[size + 1];
	memset(buffer, 0, size + 1);
	file.read(buffer, size);

	string s(buffer);
	delete[] buffer;
	return s;
}