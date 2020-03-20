#ifndef EH_SIM_ON_DEMAND_ALL_BACKUP_HPP
#define EH_SIM_ON_DEMAND_ALL_BACKUP_HPP

#include "scheme/eh_scheme.hpp"
#include "scheme/data_sheet.hpp"
#include "capacitor.hpp"
#include "stats.hpp"

namespace ehsim {

/**
 * Based on Architecture Exploration for Ambient Energy Harvesting Nonvolatile Processors.
 *
 * See the data relating to the ODAB scheme.
 */
class on_demand_all_backup : public eh_scheme {
public:
  on_demand_all_backup() : battery(NVP_CAPACITANCE, MEMENTOS_MAX_CAPACITOR_VOLTAGE, MEMENTOS_MAX_CURRENT)
  {
  }

  capacitor &get_battery() override
  {
    return battery;
  }

  uint32_t clock_frequency() const override
  {
    return NVP_CPU_FREQUENCY;
  }

  void execute_instruction(stats_bundle *stats) override
  {
    battery.consume_energy(NVP_INSTRUCTION_ENERGY);

    stats->models.back().energy_for_instructions += NVP_INSTRUCTION_ENERGY;
  }

  void execute_instruction(uint32_t cycles,stats_bundle *stats) override
  {
  }

  bool is_active(stats_bundle *stats) override
  {
    if(battery.energy_stored() >= battery.maximum_energy_stored()) {
      assert(!active);
      active = true;
    }

    return active;
  }

  bool will_backup(stats_bundle *stats) const override
  {
    // can't backup if the power is off
    assert(active);

    auto const energy_warning = NVP_ODAB_BACKUP_ENERGY + NVP_INSTRUCTION_ENERGY;

    return battery.energy_stored() <= energy_warning;
  }

  uint64_t backup(stats_bundle *stats) override
  {
    // can't backup if the power is off
    assert(active);

    // we only backup when moving to power-off mode
    active = false;

    auto &active_stats = stats->models.back();
    active_stats.num_backups++;

    active_stats.time_between_backups += stats->cpu.cycle_count - last_backup_cycle;
    last_backup_cycle = stats->cpu.cycle_count;

    active_stats.energy_for_backups += NVP_BEC_BACKUP_ENERGY;
    battery.consume_energy(NVP_ODAB_BACKUP_ENERGY);

    active_stats.time_for_backups += NVP_BEC_BACKUP_TIME;
    return NVP_ODAB_BACKUP_TIME;
  }

  uint64_t restore(stats_bundle *stats) override
  {
    // is_active should have set this to true before a restore can happen
    assert(active);

    last_backup_cycle = stats->cpu.cycle_count;

    // allocate space for a new active period model
    stats->models.emplace_back();

    stats->models.back().energy_for_restore = NVP_ODAB_RESTORE_ENERGY;
    battery.consume_energy(NVP_ODAB_RESTORE_ENERGY);

    return NVP_ODAB_RESTORE_TIME;
  }

    void execute_startup_routine()  override
    {

    }

  std::string &get_scheme_name() override
  {
    return scheme_name;
  }

private:
  capacitor battery;

  uint64_t last_backup_cycle = 0u;

  bool active = false;
  std::string scheme_name = "on_demand";
};
}
#endif //EH_SIM_ON_DEMAND_ALL_BACKUP_HPP
