#ifndef STRING_EXT
#define STRING_EXT

#include <string>
#include <vector>
#include <sstream>

using std::vector;
using std::string;

namespace str::extensions  
{
	// trim from start (in place)
	void ltrim(string& s);

	// trim from end (in place)
	void rtrim(string& s);

	// trim from both ends (in place)
	void trim(string& s);

	template < typename Container >
	string join(const Container& c, const string delimiter = ", ")
	{   
		//only join containers that can stream into stringstream
		if (c.size() == 0)
			return "";

		auto i = c.begin();
		std::stringstream ss; ss << *(i++);
		for (; i != c.end(); i++)
		{
			ss << delimiter;
			ss << *i;
		}

		return ss.str();
	}


	vector<string> split(const string s, const char delimiter);
}
#endif // !STRING_EXT
