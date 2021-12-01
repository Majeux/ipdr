#ifndef PARSE_DOT
#define PARSE_DOT

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <graphviz/gvcext.h>

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "dag.h"

namespace parse
{
  enum XMGnode
  {
    pi,
    po,
    node
  };

  // check if name has the prefix of an input
  inline bool is_input(const std::string& name)
  {
    std::string prefix = "in_";
    return name.compare(0, prefix.size(), prefix) == 0;
  }

  // check if name has the prefix of an input
  inline bool is_output(const std::string& name)
  {
    std::string prefix = "po";
    std::cout << name << "  " << (name.compare(0, prefix.size(), prefix) == 0) << std::endl;
    return name.compare(0, prefix.size(), prefix) == 0;
  }

  // check if name has the prefix of a constant
  inline bool is_const(const std::string& name)
  {
    std::string prefix = "const_";
    return name.compare(0, prefix.size(), prefix) == 0;
  }

  // retrieves the value of the "label" attribute
  // or returns the node index if it does not exist
  inline std::pair<std::string, XMGnode> name_of(Agnode_t* n)
  {
    char* l = agget(n, (char*)"label");
    std::string label(l);
    std::string name(agnameof(n));

    if (label == "\\N") 
      if (is_output(name))
        return { "out_" + name.substr(2), XMGnode::po };

    if (is_input(label))
      return { label, XMGnode::pi };

    if (is_const(label))
      return { label, XMGnode::node };

    return { "n_" + name, XMGnode::node };
  }

  class GraphvizGraph
  {
   private:
    GVC_t* context;
    Agraph_t* graph;

   public:
    GraphvizGraph(const std::string& dot)
        : context(gvContext()), graph(agmemread(dot.c_str()))
    {
      gvLayout(context, graph, "dot");
    }

    ~GraphvizGraph()
    {
      gvFreeLayout(context, graph);
      agclose(graph);
      gvFreeContext(context);
    }

    void render(const std::string& dest_file) const
    {
      gvRenderFilename(context, graph, "svg", dest_file.c_str());
    }

    template <typename Fn> void foreach_node(Fn&& function)
    {
      for (Agnode_t* n = agfstnode(graph); n; n = agnxtnode(graph, n))
        function(n);
    }

    template <typename Fn> void foreach_edge_to(Agnode_t* p, Fn&& function)
    {
      for (Agedge_t* e = agfstin(graph, p); e; e = agnxtin(graph, e))
        function(e);
    }
  };

  // parse a dot string and convert into a DAG
  inline dag::Graph parse_dot(const std::string& dot, bool show)
  {
    GraphvizGraph graph(dot);
    dag::Graph dagraph("dot-test");
    if (show)
      graph.render("text.svg");

    graph.foreach_node(
        [&dagraph, &graph](Agnode_t* node)
        {
          auto [name, type] = name_of(node);
          // TODO remove poX from the graph?
          switch (type)
          {
            case XMGnode::pi: dagraph.add_input(name); break;
            case XMGnode::po: dagraph.add_output(name); break;
            case XMGnode::node: dagraph.add_node(name);
          }

          std::vector<std::string> v;
          auto store_child = [&v](Agedge_t* edge)
          {
            Agnode_t* source  = agtail(edge);
            auto [name, type] = name_of(source);
            assert(type == XMGnode::pi || type == XMGnode::node);
            v.emplace_back(name);
          };
          graph.foreach_edge_to(node, store_child);
          dagraph.add_edges_to(v, name);
        });

    return dagraph;
  }
} // namespace parse

#endif // PARSE_DOT
