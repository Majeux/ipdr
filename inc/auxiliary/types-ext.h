#ifndef TYPES_EXT_H
#define TYPES_EXT_H

#include <functional>
#include <optional>
#include <variant>

namespace my
{

  template <class T, class... Types>
  std::optional<std::reference_wrapper<T>> get_ref(std::variant<Types...> const& v)
  {
    T const* const rv = std::get_if<T>(v);
    if (rv == nullptr)
      return {};
    return std::cref(*rv);
  }

  // gets a copy of an alternative T if the variant holds it, or an empty
  // optional otherwise
  template <class T, class... Types>
  std::optional<std::reference_wrapper<const T>> get_cref(std::variant<Types...> const& v)
  {
    T const* const rv = std::get_if<T>(v);
    if (rv == nullptr)
      return {};
    return std::cref(*rv);
  }
} // namespace my

#endif // TYPES_EXT_H
