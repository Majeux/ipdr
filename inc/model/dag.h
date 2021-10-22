#ifndef DAG
#define DAG

#include "string-ext.h"

#include <set>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>
#include <ghc/filesystem.hpp>
#include <graphviz/gvc.h>
#include <fmt/format.h>

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
			string name;
			set<string> input;
			set<string> nodes;
			set<string> output; //subet of nodes
			set<Edge> edges; //nodes X nodes
			set<Edge> input_edges; //nodes X nodes
			map<string, vector<string>> children; //nodes X nodes
			string prefix = "";

			string node(string name) { return prefix + name; }
			
			Graph(const string& s) : name(s) { }

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

			void export_digraph(const ghc::filesystem::path& folder)
			{
				//make dot representation
				std::stringstream ss;
				ss << "digraph G {" << endl;
				
				for (const Edge& e : input_edges)
					ss << fmt::format("{} -> {};", e.from, e.to) << endl;
				for (const Edge& e : edges)
					ss << fmt::format("{} -> {};", e.from, e.to) << endl;

				for (const string& o : input)
					ss << fmt::format("{} [shape=plain];", o) << endl;
				for (const string& o : output)
					ss << fmt::format("{} [shape=doublecircle];", o) << endl;

				ss << "}" << endl;

				//export to svg
				GVC_t *gvc = gvContext();
				Agraph_t *g = agmemread(ss.str().c_str());
				string image_file = (folder / "graphs" / (name + ".svg")).string();

				gvLayout(gvc, g, "dot");
				gvRenderFilename (gvc, g, "svg", image_file.c_str());

				gvFreeLayout(gvc, g);
				agclose(g);
				gvFreeContext(gvc);
			}

			bool is_output(const string& name) const { return output.find(name) != output.end(); }
	};
}

#endif
