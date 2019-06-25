#ifndef EH_SIM_SCHEME_HPP
#define EH_SIM_SCHEME_HPP

namespace ehsim {

class capacitor;
struct stats_bundle;
struct eh_model_parameters;

/**
 * An abstract checkpointing scheme.
 */
class eh_scheme {
public:
  virtual capacitor &get_battery() = 0;

  virtual uint32_t clock_frequency() const = 0;

  virtual double min_energy_to_power_on(stats_bundle *stats) = 0;

  virtual void execute_instruction(stats_bundle *stats) = 0;

  virtual bool is_active(stats_bundle *stats) = 0;

  virtual bool will_backup(stats_bundle *stats) const = 0;

  virtual void execute_instruction(uint32_t cycles,stats_bundle *stats) = 0;
  //startup routine that may be needed for some schemes
  virtual void execute_startup_routine() = 0;

  virtual uint64_t backup(stats_bundle *stats) = 0;

  virtual uint64_t restore(stats_bundle *stats) = 0;

  virtual double estimate_progress(eh_model_parameters const &) const = 0;
};
}

#endif //EH_SIM_SCHEME_HPP
