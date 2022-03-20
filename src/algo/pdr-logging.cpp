#include "pdr.h"

// LOGGING AND STAT COLLECTION SHORTHANDS
//
namespace pdr
{
  void PDR::log_start() const
  {
    logger("");
    logger.and_whisper("PDR start:");
    logger("");
  }

  void PDR::log_iteration()
  {
    logger.show("###############");
    logger.show(fmt::format("iterate frame {}", frames.frontier()));
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "");
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
    logger.tabbed("iterate frame {}", frames.frontier());
  }

  void PDR::log_cti(z3::expr_vector cti, unsigned level)
  {
    (void)cti; // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP2);
    logger.tabbed("cti at frame {}", level);
    logger.tabbed("[{}]", str::extend::join(cti));
  }

  void PDR::log_propagation(unsigned level, double time)
  {
    std::string msg = fmt::format("Propagation elapsed {}", time);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, msg);
    logger.show(msg);
    logger.stats.propagation_it.add_timed(level, time);
  }

  void PDR::log_top_obligation(size_t queue_size, unsigned top_level,
                               z3::expr_vector top)
  {
    (void)queue_size; // ignore unused warning when logging is off
    (void)top_level;  // ignore unused warning when logging is off
    (void)top;        // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP);
    logger.tabbed("obligations pending: {}", queue_size);
    logger.tabbed("top obligation");
    logger.indent++;
    logger.tabbed("{}, [{}]", top_level, str::extend::join(top));
    logger.indent--;
  }

  void PDR::log_pred(z3::expr_vector p)
  {
    (void)p; // ignore unused warning when logging is off
    logger.tabbed("predecessor:");
    logger.indent++;
    logger.tabbed("[{}]", str::extend::join(p));
    logger.indent--;
  }

  void PDR::log_state_push(unsigned frame, z3::expr_vector p)
  {
    (void)frame; // ignore unused warning when logging is off
    (void)p;     // ignore unused warning when logging is off
    logger.tabbed("pred is inductive until F_{}", frame - 1);
    logger.tabbed("push predecessor to level {}: [{}]", frame, str::extend::join(p));
  }

  void PDR::log_finish(z3::expr_vector s)
  {
    (void)s; // ignore unused warning when logging is off
    logger.tabbed("finishing state");
    logger.indent++;
    logger.tabbed("[{}]", str::extend::join(s));
    logger.indent--;
  }

  void PDR::log_obligation(std::string_view type, unsigned l, double time)
  {
    logger.stats.obligations_handled.add_timed(l, time);
    logger.and_show("Obligation {} elapsed {}", type, time);
  }
} // namespace pdr
