#include "logger.h"
#include "pdr.h"

// LOGGING AND STAT COLLECTION SHORTHANDS
//
namespace pdr
{
  void PDR::log_start() const
  {
    MYLOG_INFO(log, "");
    MYLOG_INFO(log, "PDR start ({}):", model.constraint_str());
    MYLOG_INFO(log, "");
  }

  void PDR::log_iteration()
  {
    MYLOG_INFO(log, "");
    MYLOG_INFO(log, SEP3);
    MYLOG_INFO(log, "iterate frame {}", frames.frontier());
    MYLOG_INFO(log, SEP3);
  }

  void PDR::log_cti(const z3::expr_vector& cti, unsigned level)
  {
    (void)cti; // ignore unused warning when logging is off
    MYLOG_DEBUG(log, SEP2);
    log.stats.ctis.add(level);
    MYLOG_DEBUG(log, "cti at frame {}", level);
    MYLOG_DEBUG(log, "[{}]", str::ext::join(cti));
  }

  void PDR::log_propagation(unsigned level, double time)
  {
    MYLOG_INFO(log, "Propagation elapsed {}", time);
    log.stats.propagation_it.add_timed(level, time);
  }

  void PDR::log_top_obligation(size_t queue_size, unsigned top_level,
                               const z3::expr_vector& top)
  {
    (void)queue_size; // ignore unused warning when logging is off
    (void)top_level;  // ignore unused warning when logging is off
    (void)top;        // ignore unused warning when logging is off
    MYLOG_DEBUG(log, SEP1);
    MYLOG_DEBUG(log, "obligations pending: {}", queue_size);
    MYLOG_DEBUG(log, "top obligation");
    log.indent++;
    MYLOG_DEBUG(log, "{}, [{}]", top_level, str::ext::join(top));
    log.indent--;
  }

  void PDR::log_pred(const z3::expr_vector& p)
  {
    (void)p; // ignore unused warning when logging is off
    MYLOG_DEBUG(log, "predecessor:");
    log.indent++;
    MYLOG_DEBUG(log, "[{}]", str::ext::join(p));
    log.indent--;
  }

  void PDR::log_state_push(unsigned frame)
  {
    (void)frame; // ignore unused warning when logging is off
    MYLOG_DEBUG(log, "predecessor is inductive until F_{}", frame - 1);
    MYLOG_DEBUG(log, "push predecessor to level {}", frame);
  }

  void PDR::log_finish(const z3::expr_vector& s)
  {
    (void)s; // ignore unused warning when logging is off
    MYLOG_DEBUG(log, "finishing state");
    // logger.indent++;
    // logger.tabbed("[{}]", str::extend::join(s));
    // logger.indent--;
  }

  void PDR::log_obligation_done(std::string_view type, unsigned l, double time)
  {
    log.stats.obligations_handled.add_timed(l, time);
    MYLOG_DEBUG_SHOW(log, "Obligation {} elapsed {}", type, time);
  }
} // namespace pdr
