#include "pdr.h"

// LOGGING AND STAT COLLECTION SHORTHANDS
//
namespace pdr
{
  void PDR::log_start() const
  {
    SPDLOG_LOGGER_INFO(logger.spd_logger, "");
    SPDLOG_LOGGER_INFO(logger.spd_logger, "NEW RUN\n");
    logger.out() << std::endl;
    logger.show("PDR start:");
  }

  void PDR::log_iteration()
  {
    logger("###############");
    logger(fmt::format("iterate frame {}", k));
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "");
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| frame {}", logger.tab(), k);
  }

  void PDR::log_cti(const z3::expr_vector& cti)
  {
    (void)cti; // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP2);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| cti at frame {}", logger.tab(),
                        k);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(),
                        str::extend::join(cti));
  }

  void PDR::log_propagation(unsigned level, double time)
  {
    std::string msg = fmt::format("Propagation elapsed {}", time);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, msg);
    logger(msg);
    logger.stats.propagation_it.add_timed(level, time);
  }

  void PDR::log_top_obligation(size_t queue_size, unsigned top_level,
                               const z3::expr_vector& top)
  {
    (void)queue_size; // ignore unused warning when logging is off
    (void)top_level;  // ignore unused warning when logging is off
    (void)top;        // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| obligations pending: {}",
                        logger.tab(), queue_size);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| top obligation", logger.tab());
    logger.indent++;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| {}, [{}]", logger.tab(),
                        top_level, str::extend::join(top));
    logger.indent--;
  }

  void PDR::log_pred(const z3::expr_vector& p)
  {
    (void)p; // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| predecessor:", logger.tab());
    logger.indent++;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(),
                        str::extend::join(p));
    logger.indent--;
  }

  void PDR::log_state_push(unsigned frame, const z3::expr_vector& p)
  {
    (void)frame; // ignore unused warning when logging is off
    (void)p;     // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| pred is inductive until F_{}",
                        frame - 1, logger.tab());
    SPDLOG_LOGGER_TRACE(logger.spd_logger,
                        "{}| push predecessor to level {}: [{}]", logger.tab(),
                        frame, str::extend::join(p));
  }

  void PDR::log_finish(const z3::expr_vector& s)
  {
    (void)s; // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| finishing state", logger.tab());
    logger.indent++;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(),
                        str::extend::join(s));
    logger.indent--;
  }

  void PDR::log_obligation(std::string_view type, unsigned l, double time)
  {
    logger.stats.obligations_handled.add_timed(l, time);
    std::string msg = fmt::format("Obligation {} elapsed {}", type, time);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, msg);
    logger(msg);
  }
} // namespace pdr
