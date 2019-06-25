#ifndef EH_SIM_MAGIC_HPP
#define EH_SIM_MAGIC_HPP

#include <thumbulator/cpu.hpp>

#include "scheme/eh_scheme.hpp"
#include "scheme/data_sheet.hpp"
#include "capacitor.hpp"
#include "stats.hpp"

namespace ehsim {

/**
 * A magical checkpointing scheme.
 *
 * Does not waste any energy, because it is magic.
 */
class magical_scheme : public eh_scheme {
public:
  magical_scheme() : battery(MEMENTOS_CAPACITANCE, MEMENTOS_MAX_CAPACITOR_VOLTAGE, MEMENTOS_MAX_CURRENT)
  {
  }

  capacitor &get_battery() override
  {
    return battery;
  }

  uint32_t clock_frequency() const override
  {
    return MEMENTOS_CPU_FREQUENCY;
  }

  void execute_instruction(stats_bundle *stats) override
  {
  }

  void execute_instruction(uint32_t cycles,stats_bundle *stats) override
  {
  }
  bool is_active(stats_bundle *stats) override
  {
    return true;
  }

  bool will_backup(stats_bundle *stats) const override
  {
    return false;
  }

  uint64_t backup(stats_bundle *stats) override
  {
    // do not touch arch/app state
    return 0;
  }

  uint64_t restore(stats_bundle *stats) override
  {
    // do not touch arch/app state
    return 0;
  }

    void execute_startup_routine()  override
    {

    }

private:
  capacitor battery;
};
}

#endif //EH_SIM_MAGIC_HPP
