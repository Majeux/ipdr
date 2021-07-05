#ifndef PARSE_BENCH
#define PARSE_BENCH

#include <string>
#include <vector>
#include <fmt/format.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "model/dag.h"
#include "auxiliary/string-extensions.h"

using std::vector;
using std::string;
using fmt::format;

struct LineResult
{
    bool success;
    vector<string> nodes;
    string new_node;

    LineResult(bool s, vector<string> n = {}, string nn = "") 
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

vector<string> parse_any_operator(string operands)
{
    size_t begin_bracket = operands.find_first_of('(');
    if (begin_bracket == string::npos)
        throw format("\'{0}\' expected at line {1}", '(', lineNo);

    size_t operands_begin =  begin_bracket + 1;
    size_t end_bracket = operands.find_first_of(')');
    if (end_bracket == string::npos)
        throw format("\'{0}\' expected at line {1}", ')', lineNo);

    assert(operands_begin < end_bracket);

    size_t n_chars = end_bracket - operands_begin;
    return split(operands.substr(operands_begin, n_chars), ',');
}

vector<string> parse_operator(const string& line, string op)
{
    if (line.rfind(op, 0) == string::npos) //other operation than current state, advance to next
        throw ParsingException::NEXT_STATE;

    return parse_any_operator(line.substr(op.size()));
}

LineResult parse_line(const string& line, BenchState state)
{
    vector<string> nodes;
    string new_node = "";
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
            if (sep == string::npos)
                return  LineResult(false);

            new_node = line.substr(0, sep);
            trim(new_node);
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

void add_to_graph(Graph& G, const LineResult& result, BenchState state)
{
    assert(result.success);

    switch (state)
    {
    case IN:
        if (result.nodes.size() != 1)
            throw format("INPUT at line {0} must have 1 argument", lineNo);

        G.add_input(result.nodes[0]);
        break;
    case OUT:
        if (result.nodes.size() != 1)
            throw format("OUTPUT at line {0} must have 1 argument", lineNo);

        G.add_output(result.nodes[0]);
        break;
    case GATE:
        if (result.nodes.size() == 0)
            throw format("No argument for gate at line {0}", lineNo);

        G.add_node(result.new_node);
        G.add_edges_to(result.nodes, result.new_node);
        break;
    default:
        throw format("argument \"state\" is out of range");
    }
}


Graph parse_file(string filename)
{
    assert(filename.substr(filename.find_last_of('.')) == ".bench");
    std::cout << "file: " <<  filename << std::endl;
    lineNo = 0;
    Graph G;
    BenchState state = IN;

    std::ifstream file(filename);

    string line;
    while (std::getline(file, line)) 
    {
        trim(line);

        // ignore empty lines and comments
        if (line == "" || line[0] == '#')
            continue;

        LineResult result = new LineResult(false);

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
#endif