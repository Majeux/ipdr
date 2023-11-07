#include "logger.h"
#include "vpdr.h"
#include "result.h"

#include <chrono>
#include <ctime>
#include <fmt/format.h>

// LOGGING AND STAT COLLECTION SHORTHANDS
//
namespace pdr
{
  using fmt::format;
  using z3ext::join_ev;

  std::string time_now()
  {
    using std::chrono::system_clock;
    auto t = system_clock::to_time_t(system_clock::now());
    return std::ctime(&t);
  }

  void vPDR::log_start() const
  {
    std::cerr << format("Start PDR at    {}", time_now()) << std::endl;
    MYLOG_INFO(logger, "");
    MYLOG_INFO(logger, "PDR start ({}):", ts.constraint_str());
    MYLOG_INFO(logger, "");
  }

  void vPDR::log_iteration(size_t frame)
  {
    MYLOG_INFO(logger, "");
    MYLOG_INFO(logger, SEP3);
    MYLOG_INFO(logger, "iterate frame {}", frame);
    MYLOG_INFO(logger, SEP3);
  }

  void vPDR::log_cti(const std::vector<z3::expr>& cti, unsigned level)
  {
    (void)cti; // ignore unused warning when logging is off
    (void)level;
    MYLOG_DEBUG(logger, SEP2);
    IF_STATS(logger.stats.ctis.add(level);)
    MYLOG_DEBUG(logger, "cti at frame {}", level);
    MYLOG_DEBUG(logger, "[{}]", join_ev(cti));
  }

  void vPDR::log_propagation(unsigned level, double time)
  {
    (void)level;
    (void)time;
    MYLOG_INFO(logger, "Propagation elapsed {}", time);
    IF_STATS(logger.stats.propagation_it.add(level, time);)
  }

  void vPDR::log_top_obligation(
      size_t queue_size, unsigned top_level, const std::vector<z3::expr>& top)
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

  void vPDR::log_pred(const std::vector<z3::expr>& p)
  {
    (void)p; // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, "predecessor:");
    logger.indent++;
    MYLOG_DEBUG(logger, "[{}]", join_ev(p));
    logger.indent--;
  }

  void vPDR::log_state_push(unsigned frame)
  {
    (void)frame; // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, "predecessor is inductive until F_{}", frame - 1);
    MYLOG_DEBUG(logger, "push predecessor to level {}", frame);
  }

  void vPDR::log_finish_state(const std::vector<z3::expr>& s)
  {
    (void)s; // ignore unused warning when logging is off
    MYLOG_DEBUG(logger, "finishing state");
    // logger.indent++;
    // logger.tabbed("[{}]", str::extend::join(s));
    // logger.indent--;
  }

  void vPDR::log_obligation_done(std::string_view type, unsigned l, double time)
  {
    (void)type;
    (void)l;
    (void)time;
    IF_STATS(logger.stats.obligations_handled.add(l, time);)
    MYLOG_DEBUG_SHOW(logger, "Obligation {} elapsed {}", type, time);
  }

  void vPDR::log_pdr_finish(PdrResult const& r, double final_time)
  {
    std::cerr << format("PDR finished at {}", time_now()) << "--------"
              << std::endl;
    MYLOG_INFO(logger, format("Total elapsed time {}", final_time));
    if (r)
    {
      MYLOG_INFO(logger, "Invariant found");
    }
    else
    {
      MYLOG_INFO(logger, "Terminated with trace");
    }
  }
} // namespace pdr
