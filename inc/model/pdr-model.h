#ifndef PDR_MODEL
#define PDR_MODEL

#include <memory>
#include <string>
#include <vector>
#include <z3++.h>

#include "dag.h"
#include "exp-cache.h"

class PDRModel
{
 public:
  std::string name;
  z3::config conf;
  z3::context ctx;
  ExpressionCache literals;
  ExpressionCache property;
  ExpressionCache n_property;

  PDRModel(const std::string& model_name, const dag::Graph& G,
                          int pebbles);
  void load_model(const std::string& model_name, const dag::Graph& G,
                  int max_pebbles);
  const z3::expr_vector& get_transition() const;
  const z3::expr_vector& get_initial() const;
  const z3::expr_vector& get_cardinality() const;
  int get_max_pebbles() const;
  // sets the new constraint, returns false if final state cannot be pebbled
  bool set_max_pebbles(int x);
  int get_f_pebbles() const;
  void show(std::ostream& out) const;

 private:
  int max_pebbles;
  int final_pebbles;

  z3::expr_vector initial;
  z3::expr_vector transition; // vector of clauses (cnf)
  // cardinality constraint for current and next state
  z3::expr_vector cardinality;

  z3::config& set_config(z3::config& settings);
  void load_pebble_transition(const dag::Graph& G);
  void load_pebble_transition_raw1(const dag::Graph& G);
  void load_pebble_transition_raw2(const dag::Graph& G);
  void load_property(const dag::Graph& G);
};

#endif // !PDR_MODEL
