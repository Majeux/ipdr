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



int main()
{
	// std::string model_name = "c432";
	// filesystem::path file = filesystem::current_path() / "benchmark" / "iscas85" / "bench" / (model_name + ".bench");
	// dag::Graph G = parse::parse_file(file.string());

	std::string model_name = "ham7tc";
	std::filesystem::path folder = std::filesystem::current_path() / "benchmark" / "rls";
	std::filesystem::path file = std::filesystem::current_path() / "benchmark" / "rls" / (model_name + ".tfc");

	parse::TFCParser parser;
	dag::Graph G = parser.parse_file(file.string(), model_name);
	int max_pebbles = 10;

	std::cout << "Graph" << std::endl << G;
	G.export_digraph(folder);

	return 0;
	z3::config settings;
	settings.set("unsat_core", true);
	settings.set("model", true);

	PDRModel model(settings);
	model.load_model(model_name, G, max_pebbles);

	pdr::PDR algorithm(model);
	algorithm.run();
	algorithm.show_results();
	std::cout << std::endl << algorithm.stats << std::endl;

	return 0;
}
