#include "dag.h"
#include "io.h"

#include <fmt/ranges.h>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace dag
{
  using std::endl;
  using std::string;
  using std::vector;

  Graph::Graph() {}
  Graph::Graph(string const& s) : name(s) {}
  Graph::Graph(string const& name, string const& dotstring)
      : name(name), image(std::make_unique<graphviz::Graph>(dotstring))
  {
    image->foreach_node(
        [this](Agnode_t* node)
        {
          auto [name, type] = image->name_of(node);
          switch (type)
          {
            case graphviz::XMGnode::pi: add_input(name); break;
            case graphviz::XMGnode::po: break;
            case graphviz::XMGnode::node: add_node(name);
          }

          vector<string> c = image->children_of(node);

          if (type == graphviz::XMGnode::po)
          {
            assert(c.size() == 1);
            add_output(c[0]);
          }
          else
            add_edges_to(c, name);
        });
  }

  void Graph::add_input(string iname) { input.insert(node(iname)); }

  void Graph::add_node(string nname) { nodes.insert(node(nname)); }

  void Graph::add_output(string oname)
  {
    nodes.insert(node(oname));
    output.insert(node(oname));
  }

  void Graph::add_edges_to(vector<string> from, string to)
  {
    if (from.empty())
      return;

    to = node(to);
    assert(nodes.find(to) != nodes.end());

    vector<string> to_children;
    to_children.reserve(from.size());

    for (string i : from)
    {
      string n = node(i);
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

  string Graph::summary() const
  {
    return fmt::format("Graph {{ In: {}, Out {}, Nodes {} }}", input.size(),
        output.size(), nodes.size());
  }

  std::string Graph::DAG_string() const
  {
    std::stringstream ss;
    ss << "DAG \{" << endl
       << fmt::format("\tinput {}", input) << endl
       << fmt::format("\toutput {}", output) << " }" << endl
       << fmt::format("\tnodes {}", nodes) << " }" << endl
       << fmt::format("\tedges {}", edges) << " }" << endl
       << "}" << endl;
    return ss.str();
  }

  void Graph::show_image(string const& destination)
  {
    if (!image)
      image = std::make_unique<graphviz::Graph>(dot());
    image->render(destination);
  }

  void Graph::show(string const& destination, bool to_cout)
  {
    show_image(destination);
    std::ofstream out = my::io::trunc_file(destination + ".txt");
    if (to_cout)
    {
      std::cout << summary() << endl << endl << DAG_string() << endl;
    }
    out << summary() << endl << endl << DAG_string() << endl;
  }

  string Graph::dot()
  {
    std::stringstream ss;
    ss << "digraph G {" << endl;

    for (Edge const& e : input_edges)
      ss << fmt::format("{} -> {};", e.from, e.to) << endl;
    for (Edge const& e : edges)
      ss << fmt::format("{} -> {};", e.from, e.to) << endl;

    for (string const& o : input)
      ss << fmt::format("{} [shape=plain];", o) << endl;
    for (string const& o : output)
      ss << fmt::format("{} [shape=doublecircle];", o) << endl;

    ss << "}" << endl;
    return ss.str();
  }

  bool Graph::is_output(std::string_view name) const
  {
    return output.find(name) != output.end();
  }

  vector<string> const& Graph::get_children(std::string_view key) const
  {
    auto result = children.find(key);

    if (result == children.end())
      return empty_vec;

    return result->second;
  }
} // namespace dag
