#include <fstream>
#include <z3++.h>
#include <filesystem>
#include <memory>
#include <string>

#include "pdr-model.h"
#include "parse_bench.h"
#include "parse_tfc.h"
#include "dag.h"
#include "pdr.h"

using namespace std;
using namespace z3;

void test() 
{
	config settings;
	settings.set("unsat_core", true);
	settings.set("model", true);
	context ctx;

	solver test_solver = solver(ctx);
	test_solver.set("sat.cardinality.solver", true);

	expr_vector vars = expr_vector(ctx);
	vars.push_back(ctx.bool_const("a"));
	vars.push_back(ctx.bool_const("b"));

	test_solver.add(vars[0] & vars[1]);

	expr e = !vars[0];

	check_result result = test_solver.check(1, &e);

	if (result == sat)
		cout << "SAT" << endl;
	else if (result == unsat)
		cout << "UNSAT" << endl;
	else
		cout << "UNKNOWN" << endl;
}

int main()
{
	// std::string model_name = "c432";
	// filesystem::path file = filesystem::current_path() / "benchmark" / "iscas85" / "bench" / (model_name + ".bench");
	// dag::Graph G = parse::parse_file(file.string());

	std::string model_name = "ham3tc";
	filesystem::path file = filesystem::current_path() / "benchmark" / "rls" / (model_name + ".tfc");

	parse::TFCParser parser;
	dag::Graph G = parser.parse_file(file.string());
	int max_pebbles = 10;

	cout << "Graph" << endl << G;

	return 0;
	config settings;
	settings.set("unsat_core", true);
	settings.set("model", true);

	PDRModel model(settings);
	model.load_model(model_name, G, max_pebbles);

	pdr::PDR algorithm(model);
	algorithm.run();
	algorithm.show_results();
	cout << endl << algorithm.stats << endl;

	return 0;
}
