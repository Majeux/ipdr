#include "pdr.h"

// LOGGING AND STAT COLLECTION SHORTHANDS
//
namespace pdr
{
  void PDR::log_start() const
  {
    logger.and_whisper("");
    logger.and_whisper("PDR start ({}):", model.constraint_str());
    logger("");
  }

  void PDR::log_iteration()
  {
    logger.show("###############");
    logger.show(fmt::format("iterate frame {}", frames.frontier()));
    logger("");
    logger(SEP3);
    logger.tabbed("iterate frame {}", frames.frontier());
  }

  void PDR::log_cti(const z3::expr_vector& cti, unsigned level)
  {
    (void)cti; // ignore unused warning when logging is off
    logger(SEP2);
    logger.stats.ctis.add(level);
    logger.tabbed("cti at frame {}", level);
    logger.tabbed("[{}]", str::ext::join(cti));
  }

  void PDR::log_propagation(unsigned level, double time)
  {
    std::string msg = fmt::format("Propagation elapsed {}", time);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, msg);
    logger.show(msg);
    logger.stats.propagation_it.add_timed(level, time);
  }

  void PDR::log_top_obligation(size_t queue_size, unsigned top_level,
                               const z3::expr_vector& top)
  {
    (void)queue_size; // ignore unused warning when logging is off
    (void)top_level;  // ignore unused warning when logging is off
    (void)top;        // ignore unused warning when logging is off
    logger.tabbed(SEP);
    logger.tabbed("obligations pending: {}", queue_size);
    logger.tabbed("top obligation");
    logger.indent++;
    logger.tabbed("{}, [{}]", top_level, str::ext::join(top));
    logger.indent--;
  }

  void PDR::log_pred(const z3::expr_vector& p)
  {
    (void)p; // ignore unused warning when logging is off
    logger.tabbed("predecessor:");
    logger.indent++;
    logger.tabbed("[{}]", str::ext::join(p));
    logger.indent--;
  }

  void PDR::log_state_push(unsigned frame)
  {
    (void)frame; // ignore unused warning when logging is off
    logger.tabbed("predecessor is inductive until F_{}", frame - 1);
    logger.tabbed("push predecessor to level {}", frame);
  }

  void PDR::log_finish(const z3::expr_vector& s)
  {
    (void)s; // ignore unused warning when logging is off
    logger.tabbed("finishing state");
    // logger.indent++;
    // logger.tabbed("[{}]", str::extend::join(s));
    // logger.indent--;
  }

  void PDR::log_obligation_done(std::string_view type, unsigned l, double time)
  {
    logger.stats.obligations_handled.add_timed(l, time);
    logger.and_show("Obligation {} elapsed {}", type, time);
  }
} // namespace pdr
