#ifndef DAG
#define DAG

#include <set>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fmt/format.h>
#include <cassert>

#include "string-ext.h"

namespace dag {
	using std::set;
	using std::map;
	using std::vector;
	using std::string;
	using std::endl;
	using str::extensions::join;

	struct Edge
	{
		string from;
		string to;

		Edge(const string& f, const string& t) : from(f), to(t) { }

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

		friend std::stringstream& operator<<(std::stringstream& stream, Edge const& e) {
			stream << "(" << e.from << ", " << e.to << ")";
			return stream;
		}
	};

	class Graph
	{
		public:	
			set<string> input;
			set<string> nodes;
			set<string> output; //subet of nodes
			set<Edge> edges; //nodes X nodes
			set<Edge> input_edges; //nodes X nodes
			map<string, vector<string>> children; //nodes X nodes
			string prefix = "";

			string node(string name) { return prefix + name; }
			
			Graph() { }

			void add_input(string name) { input.insert(node(name)); }

			void add_node(string name) { nodes.insert(node(name)); }

			void add_output(string name)
			{
				nodes.insert(node(name));
				output.insert(node(name));
			}

			void add_edges_to(vector<string> from, string to)
			{
				to = node(to);
				assert(nodes.find(to) != nodes.end());

				vector<string> to_children;
				to_children.reserve(from.size());

				for (string i : from)
				{
					string n = node(i);
					if (input.find(n) != input.end())
					{
						assert(nodes.find(n) != nodes.end());
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
				stream << endl;
				stream << "\tinput { " << join(g.input) << " }" << endl;
				stream << "\toutput { " << join(g.output) << " }" << endl;
				stream << "\tnodes { " << join(g.nodes) << " }" << endl;
				stream << "\tinput { " << join(g.edges) << " }" << endl;
				stream << "}" << endl;
				return stream;
			}

			std::ostream& export_digraph(std::ostream& stream)
			{
				// std::vector<string> inner_nodes;
				// std::set_difference(
				// 		nodes.begin(), nodes.end(),
				// 		input.begin(), input.end(),
				// 		std::back_inserter(inner_nodes));

				// stream << inner_nodes.size() << endl
				// 	   << join(inner_nodes, "\n") << endl;

				stream << "digraph G {" << endl;
				
				for (const Edge& e : input_edges)
					stream << fmt::format("{} -> {};", e.from, e.to) << endl;
				for (const Edge& e : edges)
					stream << fmt::format("{} -> {};", e.from, e.to) << endl;

				for (const string& o : input)
					stream << fmt::format("{} [shape=plain];", o) << endl;
				for (const string& o : output)
					stream << fmt::format("{} [shape=doublecircle];", o) << endl;

				stream << "}" << endl;

				return stream;
			}

			bool is_output(const string& name) const { return output.find(name) != output.end(); }
	};
}

#endif
