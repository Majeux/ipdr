#ifndef Z3_PEBBLING_EXP_H
#define Z3_PEBBLING_EXP_H

#include "experiments.h"
#include "z3-pebbling-model.h"

namespace pdr::test::experiments
{
  namespace expsuper = ::pdr::experiments;

  class Z3PebblingExperiment final : public expsuper::Experiment
  {
   public:
    Z3PebblingExperiment(my::cli::ArgumentList const& a, Logger& l);

   private:
    my::cli::model_t::Pebbling ts_descr;

    void reset_tables() override;
    std::shared_ptr<expsuper::Run> do_reps(const bool is_control) override;
  };

} // namespace pdr::test::experiments

#endif // Z3_PEBBLING_EXP_H
