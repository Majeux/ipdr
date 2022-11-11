#ifndef TYPES_EXT_H
#define TYPES_EXT_H

#include <functional>
#include <optional>
#include <variant>

namespace my
{
  namespace variant
  {
    // gets a reference to an alternative T if the variant holds it, or an empty
    // optional otherwise
    // reference wrapper is const, so it cannot be reassigned
    template <class T, class... Types>
    std::optional<std::reference_wrapper<T> const> get_ref(
        std::variant<Types...>& v)
    {
      T const* rv = std::get_if<T>(&v);
      if (rv == nullptr)
        return {};
      return std::ref(*rv);
    }

    // gets a const reference to an alternative T if the variant holds it, or an
    // empty optional otherwise reference is const, so it cannot be reassigned
    template <class T, class... Types>
    std::optional<std::reference_wrapper<T const> const> get_cref(
        std::variant<Types...> const& v)
    {
      T const* const rv = std::get_if<T>(&v);
      if (rv == nullptr)
        return {};
      return std::cref(*rv);
    }
  } // namespace variant
} // namespace my

#endif // TYPES_EXT_H
