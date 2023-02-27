#ifndef TABULATE_EXT_H
#define TABULATE_EXT_H

#include <string>
#include <tabulate/table.hpp>
#include <variant>
#include <vector>

namespace tabulate
{
  using std::holds_alternative;
  using std::string;
  using std::string_view;

  // Row_t::value_type = std::variant<string, const char*, string_view, Table>
  inline string to_string(const Table::Row_t::value_type& var)
  {
    if (holds_alternative<std::string>(var))
    {
      return *get_if<std::string>(&var);
    }
    else if (holds_alternative<const char*>(var))
    {
      return *get_if<const char*>(&var);
    }
    else if (holds_alternative<string_view>(var))
    {
      return std::string{ *get_if<string_view>(&var) };
    }
    else
    {
      auto table = *get_if<Table>(&var);
      std::stringstream stream;
      table.print(stream);
      return stream.str();
    }
  }
} // namespace tabulate
#endif // TABULATE_EXT_H
