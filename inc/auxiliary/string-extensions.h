#ifndef STRING_EXT
#define STRING_EXT

#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <fmt/format.h>

using std::vector;
using std::string;
using fmt::format;

// trim from start (in place)
static inline void ltrim(string& s) {
    s.erase(
        s.begin(),
        std::find_if(
            s.begin(), s.end(),
            [](unsigned char ch) {return !std::isspace(ch); }
        )
    );
}

// trim from end (in place)
static inline void rtrim(string& s) {
    s.erase(
        std::find_if(s
            .rbegin(), s.rend(),
            [](unsigned char ch) {return !std::isspace(ch); }
        ).base(),
        s.end()
    );
}

// trim from both ends (in place)
static inline void trim(string& s) {
    ltrim(s);
    rtrim(s);
}

template < typename Container >
static string join(const Container& c, const string delimiter = ", ")
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

static vector<string> split(const string s, const char delimiter)
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

#endif // !STRING_EXT