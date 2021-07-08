#include <algorithm>

#include "string-ext.h"

using std::vector;
using std::string;

namespace str::extensions
{
	// trim from start (in place)
	void ltrim(string& s) 
	{
		s.erase(
			s.begin(),
			std::find_if(
				s.begin(), s.end(),
				[](unsigned char ch) {return !std::isspace(ch); }
			)
		);
	}

	// trim from end (in place)
	void rtrim(string& s) 
	{
		s.erase(
			std::find_if(s
				.rbegin(), s.rend(),
				[](unsigned char ch) {return !std::isspace(ch); }
			).base(),
			s.end()
		);
	}

	// trim from both ends (in place)
	void trim(string& s) 
	{
		ltrim(s);
		rtrim(s);
	}

	vector<string> split(const string s, const char delimiter)
	{
		std::vector<string> list;
		std::stringstream stream(s);
		std::string segment;

		while (std::getline(stream, segment, delimiter))
		{
			trim(segment);
			list.push_back(segment);
		}

		return list;
	}
}
