#ifndef STRING_EXT
#define STRING_EXT

#include <z3++.h>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

namespace str::extensions  
{
	using std::vector;
	using std::string;

	// trim from start (in place)
	inline void ltrim(string& s) 
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
	inline void rtrim(string& s) 
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
	inline void trim(string& s) 
	{
		ltrim(s);
		rtrim(s);
	}
	template < typename Container >
	string join(const Container& c, const string delimiter = ", ")
	{   
		//only join containers that can stream into stringstream
		if (c.size() == 0)
			return "";

		bool first = true;
		std::stringstream ss; 
		for(auto i : c)
		{
			if (!first)
				ss << delimiter;
			first = false;
			ss << i;
		}
		return ss.str();
	}

	inline vector<string> split(const string s, const char delimiter)
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
#endif // !STRING_EXT
