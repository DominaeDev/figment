#include "Utility.h"

#include <algorithm> 
#include <cctype>
#include <locale>

inline string& trim(string& s)
{
	return ltrim(rtrim(s));
}

// trim from start (in place)
inline string& ltrim(string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	return s;
}

// trim from end (in place)
inline string& rtrim(string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
	return s;
}

inline bool isEmpty(const string& s)
{
	return s.size() == 0;
}