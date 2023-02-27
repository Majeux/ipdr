#include "logger.h"
#include "pdr.h"

// LOGGING AND STAT COLLECTION SHORTHANDS
//
namespace pdr
{
  using z3ext::join_ev;

  void PDR::log_start() const
  {
    MYLOG_INFO(logger, "");
    MYLOG_INFO(logger, "PDR start ({}):", ts.constraint_str());
    MYLOG_INFO(logger, "");
  }

  void PDR::log_iteration()
  {
    MYLOG_INFO(logger, "");
    MYLOG_INFO(logger, SEP3);
    MYLOG_INFO(logger, "iterate frame {}", frames.frontier());
    MYLOG_INFO(logger, SEP3);
  }

  void PDR::log_cti(const z3::expr_vector& cti, unsigned level)
  {
    (void)cti; // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, SEP2);
    logger.stats.ctis.add(level);
    MYLOG_DEBUG(logger, "cti at frame {}", level);
    MYLOG_DEBUG(logger, "[{}]", join_ev(cti));
  }

  void PDR::log_propagation(unsigned level, double time)
  {
    MYLOG_INFO(logger, "Propagation elapsed {}", time);
    logger.stats.propagation_it.add_timed(level, time);
  }

  void PDR::log_top_obligation(size_t queue_size, unsigned top_level,
                               const z3::expr_vector& top)
  {
    (void)queue_size; // ignore unused warning when logging is off
    (void)top_level;  // ignore unused warning when logging is off
    (void)top;        // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, SEP1);
    MYLOG_DEBUG(logger, "obligations pending: {}", queue_size);
    MYLOG_DEBUG(logger, "top obligation");
    logger.indent++;
    MYLOG_DEBUG(logger, "{}, [{}]", top_level, join_ev(top));
    logger.indent--;
  }

  void PDR::log_pred(const z3::expr_vector& p)
  {
    (void)p; // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, "predecessor:");
    logger.indent++;
    MYLOG_DEBUG(logger, "[{}]", join_ev(p));
    logger.indent--;
  }

  void PDR::log_state_push(unsigned frame)
  {
    (void)frame; // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, "predecessor is inductive until F_{}", frame - 1);
    MYLOG_DEBUG(logger, "push predecessor to level {}", frame);
  }

  void PDR::log_finish(const z3::expr_vector& s)
  {
    (void)s; // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, "finishing state");
    // logger.indent++;
    // logger.tabbed("[{}]", str::extend::join(s));
    // logger.indent--;
  }

  void PDR::log_obligation_done(std::string_view type, unsigned l, double time)
  {
    logger.stats.obligations_handled.add_timed(l, time);
    MYLOG_DEBUG_SHOW(logger, "Obligation {} elapsed {}", type, time);
  }
} // namespace pdr
