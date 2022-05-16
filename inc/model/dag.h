#ifndef DAG_H
#define DAG_H

#include "graphvizgraph.h"
#include "string-ext.h"

#include <cassert>
#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <graphviz/gvc.h>
#include <map>
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

    Edge(const std::string& f, const std::string& t) : from(f), to(t) {}

    friend bool operator<(const Edge& lhs, const Edge& rhs)
    {
      if (lhs.from < rhs.from)
        return true;
      if (rhs.from < lhs.from)
        return false;

      if (lhs.to < rhs.to)
        return true;
      return false;
    }

    friend std::stringstream& operator<<(std::stringstream& stream,
                                         Edge const& e)
    {
      stream << "(" << e.from << ", " << e.to << ")";
      return stream;
    }
  };

  class Graph
  {
   private:
    std::map<std::string, std::vector<std::string>, std::less<>> children; // nodes X nodes
    std::set<Edge> input_edges;                               // nodes X nodes
    std::vector<std::string> empty_vec;
    std::unique_ptr<graphviz::Graph> image;

   public:
    std::string name;
    std::string prefix = "";

    std::set<std::string, std::less<>> input;
    std::set<std::string, std::less<>> nodes;
    std::set<std::string, std::less<>> output; // subet of nodes
    std::set<Edge> edges;         // nodes X nodes

    std::string node(std::string name) { return prefix + name; }

    Graph();
    Graph(const std::string& s);
    Graph(const std::string& s, const std::string& dot);

    void add_input(std::string iname);

    void add_node(std::string nname);

    void add_output(std::string oname);

    void add_edges_to(std::vector<std::string> from, std::string to);

    std::string summary() const;

    friend std::ostream& operator<<(std::ostream& stream, Graph const& g);

    void show_image(const std::string& destination);

    std::string dot();

    bool is_output(std::string_view name) const;

    const std::vector<std::string>& get_children(std::string_view key) const;
  };
} // namespace dag

#endif // DAG_H
