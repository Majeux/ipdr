#ifndef GRAPHVIZ_GRAPH_H
#define GRAPHVIZ_GRAPH_H

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <graphviz/gvcext.h>

#include <string>
#include <vector>

namespace graphviz
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
    return name.compare(0, prefix.size(), prefix) == 0;
  }

  // check if name has the prefix of a constant
  inline bool is_const(const std::string& name)
  {
    std::string prefix = "const_";
    return name.compare(0, prefix.size(), prefix) == 0;
  }

  class Graph
  {
   private:
    GVC_t* context;
    Agraph_t* graph;

   public:
    Graph(const std::string& dot)
        : context(gvContext()), graph(agmemread(dot.c_str()))
    {
      gvLayout(context, graph, "dot");
    }

    ~Graph()
    {
      gvFreeLayout(context, graph);
      agclose(graph);
      gvFreeContext(context);
    }

    void render(const std::string& dest_file) const
    {
      gvRenderFilename(context, graph, "svg", dest_file.c_str());
    }

    std::vector<std::string> children_of(Agnode_t* n)
    {
      // determine edges to node
      std::vector<std::string> children;
      auto store_child = [&children, this](Agedge_t* edge)
      {
        Agnode_t* source  = agtail(edge);
        auto [name, type] = name_of(source);
        assert(type == XMGnode::pi || type == XMGnode::node);
        children.emplace_back(name);
      };

      foreach_edge_to(n, store_child);

      return children;
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
          return { name, XMGnode::po };

      if (is_input(label))
        return { label, XMGnode::pi };

      if (is_const(label))
        return { label, XMGnode::node };

      return { "n_" + name, XMGnode::node };
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
} // namespace graphviz

#endif // GRAPHVIZ_GRAPH_H
