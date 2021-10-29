#ifndef PARSE_BENCH
#define PARSE_BENCH

#include "dag.h"
#include "string-ext.h"

#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <fmt/format.h>

namespace parse 
{
	struct LineResult
	{
		bool success;
		std::vector<std::string> nodes;
		std::string new_node;

		LineResult(bool s, std::vector<std::string> n = {}, std::string nn = "") 
			: success(s), nodes(n), new_node(nn) { }
	};

	enum BenchState
	{
		IN, OUT, GATE,
		_END
	};

	static BenchState next(BenchState s)
	{
		assert((int)s >= 0);

		int next = std::min((int)s + 1, (int)_END);

		return (BenchState) next;
	}

	static int lineNo;

	enum class ParsingException {
		ERROR, NEXT_STATE
	};

	inline std::vector<std::string> parse_any_operator(std::string operands)
	{
		size_t begin_bracket = operands.find_first_of('(');
		if (begin_bracket == std::string::npos)
			throw fmt::format("\'{0}\' expected at line {1}", '(', lineNo);

		size_t operands_begin =  begin_bracket + 1;
		size_t end_bracket = operands.find_first_of(')');
		if (end_bracket == std::string::npos)
			throw fmt::format("\'{0}\' expected at line {1}", ')', lineNo);

		assert(operands_begin < end_bracket);

		size_t n_chars = end_bracket - operands_begin;
		return str::extend::split(operands.substr(operands_begin, n_chars), ',');
	}

	inline std::vector<std::string> parse_operator(const std::string& line, std::string op)
	{
		if (line.rfind(op, 0) == std::string::npos) //other operation than current state, advance to next
			throw ParsingException::NEXT_STATE;

		return parse_any_operator(line.substr(op.size()));
	}

	inline LineResult parse_line(const std::string& line, BenchState state)
	{
		std::vector<std::string> nodes;
		std::string new_node = "";
		size_t sep;

		try
		{
			switch (state)
			{
			case IN:
				nodes = parse_operator(line, "INPUT");
				break;
			case OUT:
				nodes = parse_operator(line, "OUTPUT");
				break;
			case GATE:
				sep = line.find_first_of('=');
				if (sep == std::string::npos)
					return  LineResult(false);

				new_node = line.substr(0, sep);
				str::extend::trim(new_node);
				nodes = parse_any_operator(line);
				break;
			default:
				return LineResult(false);
			}
		}
		catch (ParsingException e) {
			if (e == ParsingException::NEXT_STATE)
				return LineResult(false);
			if (e == ParsingException::ERROR)
				throw e;
		}

		return LineResult(true, nodes, new_node);
	}

	inline void add_to_graph(dag::Graph& G, const LineResult& result, BenchState state)
	{
		assert(result.success);

		switch (state)
		{
		case IN:
			if (result.nodes.size() != 1)
				throw fmt::format("INPUT at line {0} must have 1 argument", lineNo);

			G.add_input(result.nodes[0]);
			break;
		case OUT:
			if (result.nodes.size() != 1)
				throw fmt::format("OUTPUT at line {0} must have 1 argument", lineNo);

			G.add_output(result.nodes[0]);
			break;
		case GATE:
			if (result.nodes.size() == 0)
				throw fmt::format("No argument for gate at line {0}", lineNo);

			G.add_node(result.new_node);
			G.add_edges_to(result.nodes, result.new_node);
			break;
		default:
			throw fmt::format("argument \"state\" is out of range");
		}
	}


	inline dag::Graph parse_file(const std::string& filename, const std::string& graph_name)
	{
		assert(filename.substr(filename.find_last_of('.')) == ".bench");
		std::cout << "file: " <<  filename << std::endl;
		lineNo = 0;
		dag::Graph G(graph_name);
		G.prefix = "n_";
		BenchState state = IN;

		std::ifstream file(filename);

		std::string line;
		while (std::getline(file, line)) 
		{
			str::extend::trim(line);

			// ignore empty lines and comments
			if (line == "" || line[0] == '#')
				continue;

			LineResult result(false);

			while (state != _END)
			{
				result = parse_line(line, state);

				if (result.success)
					break;
				else
					state = next(state); // try parsing next state
			}
			if (state == _END)
				return G;

			add_to_graph(G, result, state);

			lineNo++;
		}

		file.close();
		return G;
	}
}
#endif
