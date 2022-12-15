#ifndef DAG_H
#define DAG_H

#include "graphvizgraph.h"
#include "string-ext.h"

#include <cassert>
#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <graphviz/gvc.h>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace dag
{
  struct Edge
  {
    std::string from;
    std::string to;

    Edge(std::string const& f, std::string const& t) : from(f), to(t) {}

    friend bool operator<(Edge const& lhs, Edge const& rhs)
    {
      if (lhs.from < rhs.from)
        return true;
      if (rhs.from < lhs.from)
        return false;

      if (lhs.to < rhs.to)
        return true;
      return false;
    }

    friend std::stringstream& operator<<(
        std::stringstream& stream, Edge const& e)
    {
      stream << "(" << e.from << ", " << e.to << ")";
      return stream;
    }
  };

  class Graph
  {
   public:
    std::string name;
    std::string prefix = "";

    std::set<std::string> input;
    std::set<std::string> nodes;
    std::set<std::string, std::less<>> output; // subet of nodes
    std::set<Edge> edges;                      // nodes X nodes

    std::string node(std::string name) { return prefix + name; }

    Graph();
    Graph(std::string const& s);
    Graph(std::string const& s, std::string const& dot);

    void add_input(std::string iname);
    void add_node(std::string nname);
    void add_output(std::string oname);
    void add_edges_to(std::vector<std::string> from, std::string to);

    std::string summary() const;
    std::string DAG_string() const;

    // write a dot image to the destination (path/filename without extension)
    void show_image(std::string const& destination);
    // write a dot image and text description
    void show(std::string const& destination, bool to_cout);

    std::string dot();

    bool is_output(std::string_view name) const;

    const std::vector<std::string>& get_children(std::string_view key) const;

   private:
    std::map<std::string, std::vector<std::string>, std::less<>>
        children;               // nodes X nodes
    std::set<Edge> input_edges; // nodes X nodes
    std::vector<std::string> empty_vec;
    std::unique_ptr<graphviz::Graph> image;
  };
} // namespace dag

// formatter for edges
template <> struct fmt::formatter<dag::Edge> : fmt::formatter<std::string_view>
{
  auto format(dag::Edge e, fmt::format_context& ctx) const
  {
    std::string e_str = "(" + e.from + ", " + e.to + ")"; 
    return fmt::formatter<std::string_view>::format(e_str, ctx);
  }
};
#endif // DAG_H
