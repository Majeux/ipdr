#ifndef PARSE_DOT
#define PARSE_DOT

#include "dag.h"
#include "graphvizgraph.h"

#include <cstring>
#include <string>
#include <vector>

namespace parse
{
  // parse a dot string and convert into a DAG
  inline dag::Graph parse_dot(const std::string& dot, const std::string& name)
  {
    graphviz::Graph graph(dot);
    dag::Graph dagraph(name, dot);

    graph.foreach_node(
        [&dagraph, &graph](Agnode_t* node)
        {
          auto [name, type] = graph.name_of(node);
          switch (type)
          {
            case graphviz::XMGnode::pi: dagraph.add_input(name); break;
            case graphviz::XMGnode::po: break;
            case graphviz::XMGnode::node: dagraph.add_node(name);
          }

          std::vector<std::string> c = graph.children_of(node);

          if (type == graphviz::XMGnode::po)
          {
            assert(c.size() == 1);
            dagraph.add_output(c[0]);
          }
          else
            dagraph.add_edges_to(c, name);
        });

    return dagraph;
  }
} // namespace parse

#endif // PARSE_DOT
