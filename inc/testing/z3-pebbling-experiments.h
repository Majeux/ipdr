#ifndef Z3_PEBBLING_EXP_H
#define Z3_PEBBLING_EXP_H

#include "experiments.h"
#include "z3-pebbling-model.h"

namespace pdr::test::experiments
{
  namespace expsuper = ::pdr::experiments;

  class Z3PebblingExperiment final : public expsuper::Experiment
  {
    Z3PebblingExperiment(
        my::cli::ArgumentList const& a, Z3PebblingModel& m, Logger& l);
    
    private:
    Z3PebblingModel& ts;
    my::cli::model_t::Pebbling ts_descr;

    void reset_tables() override;
    std::shared_ptr<expsuper::Run> do_reps(const bool is_control) override;
  };
  
}


#endif // Z3_PEBBLING_EXP_H
