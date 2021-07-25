#ifndef PARSE_TFC
#define PARSE_TFC

#include "dag.h"
#include "string-ext.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <map>
#include <fstream>

#define OUT(s) std::cout << s << std::endl

namespace parse 
{
	using std::string;
	using std::vector;
	using str::extensions::split;
	using vars_iterator = std::map<string, unsigned>::iterator;
	using const_vars_iterator = std::map<string, unsigned>::const_iterator;

	enum class TFCState
	{
		VARS, INPUTS, OUTPUTS, CONSTANTS, BEGIN, BODY,
		_END
	};

	class TFCParser 
	{
		private:
			std::ifstream file;
			std::map<string, unsigned> vars;
			std::set<string> ins;
			std::set<string> outs;
		
		public:
			static string node(const string& name, unsigned i) { return name + "_" + std::to_string(i); }
			static string node(const_vars_iterator n) { return n->first + "_" + std::to_string(n->second); }

			dag::Graph parse_file(string filename)
			{
				assert(filename.substr(filename.find_last_of('.')) == ".tfc");
				std::cout << "file: " <<  filename << std::endl;
				dag::Graph G;
				vars.clear();

				file.open(filename);
				assert(file.is_open());

				TFCState state = TFCState::VARS;
				string line;
				while (state != TFCState::_END && std::getline(file, line)) 
				{
					str::extensions::trim(line);
					if (line.size() == 0 || line[0] == '#')
						continue;

					bool retry;
					do {
						retry = false;
						switch(state)
						{
							case TFCState::VARS:
								assert(prefixed(line, ".v "));
								state = TFCState::INPUTS;
								break;
							case TFCState::INPUTS:
								parse_inputs(G, line);
								state = TFCState::OUTPUTS;
								break;
							case TFCState::OUTPUTS:
								parse_outputs(line);
								state = TFCState::CONSTANTS;
								break;
							case TFCState::CONSTANTS:
								retry = !prefixed(line, ".c ");
								state = TFCState::BEGIN;
								break;
							case TFCState::BEGIN:
								assert(prefixed(line, "BEGIN"));
								state = TFCState::BODY;
								break;
							case TFCState::BODY:
								if (line == "END")
									state = TFCState::_END;
								else
									parse_line(G, line);
								break;
							case TFCState::_END:
								assert(false);
						}
					} while (retry);
				}
				file.close();
				for (const string& o : outs)
				{
					auto it = vars.find(o);
					assert(it != vars.end());
					//name of latest version static single assignment form
					string output = node(it);
					G.add_output(output);
				}

				return G;
			}

			//assumes no starting or trailing whitespace
			void parse_line(dag::Graph& G, string line)
			{
				vector<string> op_operands = split(line, ' ');
				assert(op_operands.size() == 2);
				vector<string> operands = split(op_operands.back(), ',');
				
				auto [old_t, new_t] = target(G, operands.back());
				operands.pop_back();
				std::transform(operands.begin(), operands.end(), operands.begin(), 
						[this](const string& s) { return operand(s); });				

				if (old_t != "")
					operands.push_back(old_t);

				G.add_edges_to(operands, new_t);
			}

			string new_var(dag::Graph& G, const string& name)
			{
				string new_node = node(name, 0);
				vars.emplace(name, 0);
				G.add_node(new_node);
				return new_node;
			}

			string upgrade_var(dag::Graph& G, vars_iterator var)
			{
				var->second++;
				G.add_node(node(var));
				return node(var);
			}

			string operand(const string& name)
			{
				auto it = vars.find(name);
				assert(it != vars.end());
				return node(it);
			}

			std::pair<string, string> target(dag::Graph& G, const string& name)
			{
				auto it = vars.find(name);
				if (it == vars.end())
				{
					string new_t = new_var(G, name);
					return std::make_pair("", new_t);
				}
				string old_t = node(it);
				string new_t = upgrade_var(G, it);

				return std::make_pair(old_t, new_t);
			}

			bool prefixed(string line, string pre)
			{
				if (pre != "")
					return line.rfind(pre, 0) == 0;
				return true;
			}

			void parse_inputs(dag::Graph& G, string line)
			{
				assert(line.rfind(".i ", 0) == 0);
				line = line.substr(3);
				vector<string> names = str::extensions::split(line, ',');
				for (const string& n : names)
				{
					string var_init = new_var(G, n);
					ins.insert(n);
					G.add_input(var_init);
				}
			}

			void parse_outputs(string line)
			{
				assert(line.rfind(".o ", 0) == 0);

				line = line.substr(3);
				vector<string> names = str::extensions::split(line, ',');
				for (const string& n : names)
				{
					outs.insert(n);
				}
			}
	};
}
#endif //PARSE_TFC
