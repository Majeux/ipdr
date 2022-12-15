#ifndef STRING_EXT
#define STRING_EXT

#include <algorithm>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <regex>
#include <z3++.h>

namespace str::ext
{
  using std::string;
  using std::string_view;
  using std::vector;

  inline bool size_lt(string_view a, string_view b)
  {
    return a.size() < b.size();
  };

  // trim from start (in place)
  inline void ltrim(string& s)
  {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                           [](unsigned char ch) { return !std::isspace(ch); }));
  }

  // trim from end (in place)
  inline void rtrim(string& s)
  {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
        s.end());
  }

  // trim from both ends (in place)
  inline void trim(string& s)
  {
    ltrim(s);
    rtrim(s);
  }

  // indent the start of each line in str with 2*n spaces
  inline std::string indent(std::string const& str, unsigned n = 1)
  {
    return std::regex_replace(str, std::regex("^"), std::string(2*n, ' '));
  }
  inline void indent_inplace(std::string& str, unsigned n = 1)
  {
    std::regex_replace(str, std::regex("^"), std::string(2*n, ' '));
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
} // namespace str::extend
#endif // !STRING_EXT
