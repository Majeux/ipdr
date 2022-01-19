#include "dag.h"

namespace dag
{
  Graph::Graph() {}
  Graph::Graph(const std::string& s) : name(s) {}
  Graph::Graph(const std::string& name, const std::string& dotstring)
      : image(std::make_unique<graphviz::Graph>(dotstring)), name(name)
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

          std::vector<std::string> c = image->children_of(node);

          if (type == graphviz::XMGnode::po)
          {
            assert(c.size() == 1);
            add_output(c[0]);
          }
          else
            add_edges_to(c, name);
        });
  }

  void Graph::add_input(std::string iname) { input.insert(node(iname)); }

  void Graph::add_node(std::string nname) { nodes.insert(node(nname)); }

  void Graph::add_output(std::string oname)
  {
    nodes.insert(node(oname));
    output.insert(node(oname));
  }

  void Graph::add_edges_to(std::vector<std::string> from, std::string to)
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

  std::string Graph::summary() const
  {
    return fmt::format("Graph {{ In: {}, Out {}, Nodes {} }}", input.size(),
                       output.size(), nodes.size());
  }

  std::ostream& operator<<(std::ostream& stream, Graph const& g)
  {
    stream << "DAG \{" << std::endl
           << "\tinput { " << str::extend::join(g.input) << " }" << std::endl
           << "\toutput { " << str::extend::join(g.output) << " }" << std::endl
           << "\tnodes { " << str::extend::join(g.nodes) << " }" << std::endl
           << "\tedges { " << str::extend::join(g.edges) << " }" << std::endl
           << "}" << std::endl;
    return stream;
  }

  void Graph::show_image(const std::string& destination)
  {
    if (!image)
      image = std::make_unique<graphviz::Graph>(dot());
    image->render(destination);
  }

  std::string Graph::dot()
  {
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
    return ss.str();
  }

  bool Graph::is_output(const std::string& name) const
  {
    return output.find(name) != output.end();
  }

  const std::vector<std::string>&
      Graph::get_children(const std::string& key) const
  {
    auto result = children.find(key);

    if (result == children.end())
      return empty_vec;

    return result->second;
  }
} // namespace dag
