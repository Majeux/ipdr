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
	std::filesystem::path bench_folder = std::filesystem::current_path() / "benchmark" / "rls";
	std::filesystem::path results_folder = std::filesystem::current_path() / "results";
	std::filesystem::path stats_folder = std::filesystem::current_path() / "stats";
	std::filesystem::create_directory(results_folder);
	std::filesystem::create_directory(stats_folder);

	std::filesystem::path model_path = 
		std::filesystem::current_path() / "benchmark" / "rls" / (model_name + ".tfc");

	string stats_file = model_name + ".stats";
	string stats_path = (stats_folder / stats_file).string();
	std::cout << "Statistics to: " << stats_path << std::endl;
	std::fstream stats(stats_path, std::fstream::out | std::fstream::trunc);
	string results_file = model_name + "-strategy.result";
	string results_path = (results_folder / results_file).string();
	std::cout << "Result to: " << results_path << std::endl;
	std::fstream results(results_path, std::fstream::out | std::fstream::trunc);	
	assert(stats.is_open()); assert(results.is_open());

	parse::TFCParser parser;
	dag::Graph G = parser.parse_file(model_path.string(), model_name);
	int max_pebbles = 10;

	std::cout << "Graph" << std::endl << G;
	G.export_digraph(bench_folder);

	z3::config settings;
	settings.set("unsat_core", true);
	settings.set("model", true);

	PDRModel model(settings);
	model.load_model(model_name, G, max_pebbles);

	pdr::PDR algorithm(model);
	algorithm.run();

	algorithm.show_results(results);
	stats << algorithm.stats << std::endl;

	results.close();
	stats.close();

	return 0;
}
