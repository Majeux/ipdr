#include "pdr-model.h"
#include "parse_bench.h"
#include "parse_tfc.h"
#include "dag.h"
#include "pdr.h"

#include <cstring>
#include <fmt/core.h>
#include <fstream>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <z3++.h>
#include <fmt/format.h>

//ensure proper folders exist and create file names for In and Output
std::pair<string, string> setup_in_out(const string& model_name, unsigned n_pebbles, bool dynamic) 
{
	std::filesystem::path results_folder = std::filesystem::current_path() / "results";
	std::filesystem::path stats_folder = std::filesystem::current_path() / "stats";

	std::filesystem::create_directory(results_folder);
	std::filesystem::create_directory(results_folder / model_name);
	std::filesystem::create_directory(stats_folder);
	std::filesystem::create_directory(stats_folder / model_name);

	string stats_file = fmt::format("{}-{}pebbles{}.stats", model_name, n_pebbles, dynamic ? "-dyn" : "");
	string strategy_file = fmt::format("{}-{}pebbles{}.strategy", model_name, n_pebbles, dynamic ? "-dyn" : "");

	return std::make_pair(
			(stats_folder / model_name / stats_file).string(),
			(results_folder / model_name / strategy_file).string()
		);
}

int main(int argc, char *argv[])
{
	// std::string model_name = "c432";
	// filesystem::path file = filesystem::current_path() / "benchmark" / "iscas85" / "bench" / (model_name + ".bench");
	// dag::Graph G = parse::parse_file(file.string());

	std::string model_name; int max_pebbles;
	if (argc >= 3) 
	{
		model_name = argv[1];
		max_pebbles = std::stoul(argv[2]);
	}
	else 
	{
		model_name = "ham7tc";
		max_pebbles = 9;
	}

	bool dynamic = false;
	if (argc >= 4)
	{
		if (std::strcmp("-d", argv[3]) == 0)
			dynamic = true;
		else 
			assert(false); // invalid argument
	}

	std::cout << fmt::format("Finding {}-pebble strategy for {}", max_pebbles, model_name) << std::endl;

	std::filesystem::path bench_folder = std::filesystem::current_path() / "benchmark" / "rls";
	std::filesystem::path model_file = 
		std::filesystem::current_path() / "benchmark" / "rls" / (model_name + ".tfc");
	const auto [stats_file, strategy_file] = setup_in_out(model_name, max_pebbles, dynamic);

	std::cout << "Statistics to: " << stats_file << std::endl;
	std::fstream stats(stats_file, std::fstream::out | std::fstream::trunc);

	std::cout << "Result to: " << strategy_file << std::endl;
	std::fstream results(strategy_file, std::fstream::out | std::fstream::trunc);	
	
	assert(stats.is_open()); assert(results.is_open());

	//read input model
	parse::TFCParser parser;
	dag::Graph G = parser.parse_file(model_file, model_name);

	std::cout << "Graph" << std::endl << G;
	G.export_digraph(bench_folder);

	//init z3
	z3::config settings;
	settings.set("unsat_core", true);
	settings.set("model", true);

	//create model from DAG graph and set up algorithm
	PDRModel model(settings);
	model.load_model(model_name, G, max_pebbles);
	pdr::PDR algorithm(model);
	algorithm.stats.model.emplace("nodes", G.nodes.size());
	algorithm.stats.model.emplace("edges", G.edges.size());
	algorithm.stats.model.emplace("outputs", G.output.size());

	//run pdr and write output
	bool strategy = !algorithm.run(dynamic);
	algorithm.show_results(results);
	stats << algorithm.stats << std::endl;


	if (dynamic && strategy) //decrement and find better strategy
	{
		max_pebbles--;
		std::cout << "retrying with" << max_pebbles << std::endl;
		algorithm.decrement(1);
		algorithm.run(true);
		algorithm.show_results(results);
		stats << algorithm.stats << std::endl;
	}

	results.close();
	stats.close();

	return 0;
}
