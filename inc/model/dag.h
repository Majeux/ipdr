#ifndef DAG
#define DAG

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
   public:
    std::string name;
    std::set<std::string> input;
    std::set<std::string> nodes;
    std::set<std::string> output;                             // subet of nodes
    std::set<Edge> edges;                                     // nodes X nodes
    std::set<Edge> input_edges;                               // nodes X nodes
    std::map<std::string, std::vector<std::string>> children; // nodes X nodes
    std::string prefix = "";

    std::string node(std::string name) { return prefix + name; }

    Graph(const std::string& s) : name(s) {}

    void add_input(std::string name) { input.insert(node(name)); }

    void add_node(std::string name) { nodes.insert(node(name)); }

    void add_output(std::string name)
    {
      nodes.insert(node(name));
      output.insert(node(name));
    }

    void add_edges_to(std::vector<std::string> from, std::string to)
    {
      if (from.empty())
        return;

      to = node(to);
      assert(nodes.find(to) != nodes.end());

      std::vector<std::string> to_children;
      to_children.reserve(from.size());

      for (std::string i : from)
      {
        std::string n = node(i);
        if (input.find(n) != input.end())
        {
          input_edges.emplace(n, to);
          continue;
        }

        assert(nodes.find(n) != nodes.end());
        edges.emplace(n, to);
        to_children.push_back(n);
      }

      children.emplace(to, std::move(to_children));
    }

    friend std::ostream& operator<<(std::ostream& stream, Graph const& g)
    {
      stream << "DAG {";
      stream << std::endl;
      stream << "\tinput { " << str::extend::join(g.input) << " }" << std::endl;
      stream << "\toutput { " << str::extend::join(g.output) << " }"
             << std::endl;
      stream << "\tnodes { " << str::extend::join(g.nodes) << " }" << std::endl;
      stream << "\tinput { " << str::extend::join(g.edges) << " }" << std::endl;
      stream << "}" << std::endl;
      return stream;
    }

    void export_digraph(const ghc::filesystem::path& folder)
    {
      // make dot representation
      std::stringstream ss;
      ss << "digraph G {" << std::endl;

      for (const Edge& e : input_edges)
        ss << fmt::format("{} -> {};", e.from, e.to) << std::endl;
      for (const Edge& e : edges)
        ss << fmt::format("{} -> {};", e.from, e.to) << std::endl;

      for (const std::string& o : input)
        ss << fmt::format("{} [shape=plain];", o) << std::endl;
      for (const std::string& o : output)
        ss << fmt::format("{} [shape=doublecircle];", o) << std::endl;

      ss << "}" << std::endl;

      // export to svg
      GVC_t* gvc             = gvContext();
      Agraph_t* g            = agmemread(ss.str().c_str());
      std::string image_file = (folder / "graphs" / (name + ".svg")).string();

      gvLayout(gvc, g, "dot");
      gvRenderFilename(gvc, g, "svg", image_file.c_str());

      gvFreeLayout(gvc, g);
      agclose(g);
      gvFreeContext(gvc);
    }

    bool is_output(const std::string& name) const
    {
      return output.find(name) != output.end();
    }
  };
} // namespace dag

#endif
