#ifndef EH_SIM_TASK_BASED_HPP
#define EH_SIM_TASK_BASED_HPP

#include "scheme/eh_scheme.hpp"
#include "scheme/data_sheet.hpp"
#include "scheme/eh_model.hpp"
#include "capacitor.hpp"

namespace ehsim {

/**
 * Tester scheme for dual purpose predictor
 *
 */
class task_based : public eh_scheme {
public:
  task_based() : battery(NVP_CAPACITANCE, MEMENTOS_MAX_CAPACITOR_VOLTAGE, MEMENTOS_MAX_CURRENT)
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

  double min_energy_to_power_on(stats_bundle *stats) override
  {
    auto required_energy = NVP_INSTRUCTION_ENERGY + NVP_BEC_BACKUP_ENERGY;

    if(stats->cpu.instruction_count != 0) {
      // we only need to restore if an instruction has been executed
      required_energy += NVP_BEC_RESTORE_ENERGY;
    }
    return required_energy;
  }

  void execute_instruction(stats_bundle *stats) override
  {
    //TODO: currently every inst consumes the same energy regardless of cycles (ticks)
    battery.consume_energy(NVP_INSTRUCTION_ENERGY);

    stats->models.back().energy_for_instructions += NVP_INSTRUCTION_ENERGY;
  }

  bool is_active(stats_bundle *stats) override
  {
    auto required_energy = NVP_INSTRUCTION_ENERGY + NVP_BEC_BACKUP_ENERGY + NVP_BEC_RESTORE_ENERGY;
    if(battery.energy_stored() >  battery.maximum_energy_stored() - required_energy)
    {
      active = true;
    }
    else if (battery.energy_stored() < required_energy)
    {
      active = false;
    }
    return active;
  }

  bool will_backup(stats_bundle *stats) const override
  {
    return stats->cpu.backup_recently_requested;
  }

  uint64_t backup(stats_bundle *stats) override
  {

      // can't backup if the power is off
      //
    assert(active);


    // do not touch arch/app state, assume it is all non-volatile <--should we continue assuming this?

    backup_CPU_state = thumbulator::cpu;

    std::cout <<" BACKUP! \n";
    //to remain idempotent on an arbitrary checkpoint, need to save non-volatile as well
    std::copy(std::begin(thumbulator::RAM),std::end(thumbulator::RAM),std::begin(backup_RAM));
    std::copy(std::begin(thumbulator::FLASH_MEMORY),std::end(thumbulator::FLASH_MEMORY),std::begin(backup_FLASH));
    auto &active_stats = stats->models.back();
    active_stats.num_backups++;
    active_stats.time_between_backups += stats->cpu.cycle_count - last_backup_cycle;
    last_backup_cycle = stats->cpu.cycle_count;

    //this stuff needs to be adjusted
    active_stats.energy_for_backups += NVP_BEC_BACKUP_ENERGY;
    battery.consume_energy(NVP_BEC_BACKUP_ENERGY);

    return NVP_BEC_BACKUP_TIME;
  }

  uint64_t restore(stats_bundle *stats) override
  {
    last_backup_cycle = stats->cpu.cycle_count;

    // do not touch arch/app state, assume it is all non-volatile

    stats->models.back().energy_for_restore = NVP_BEC_RESTORE_ENERGY;
    battery.consume_energy(NVP_BEC_RESTORE_ENERGY);

    return NVP_BEC_RESTORE_TIME;
  }

  double estimate_progress(eh_model_parameters const &eh) const override
  {
    return estimate_eh_progress(eh, dead_cycles::best_case, NVP_BEC_OMEGA_R, NVP_BEC_SIGMA_R, NVP_BEC_A_R,
                                NVP_BEC_OMEGA_B, NVP_BEC_SIGMA_B, NVP_BEC_A_B);
  }

private:
  capacitor battery;
  thumbulator::cpu_state backup_CPU_state;
  uint64_t last_backup_cycle = 0u;
  bool active = false;
  uint32_t backup_RAM[RAM_SIZE_BYTES >>2 ];
  uint32_t backup_FLASH[FLASH_SIZE_BYTES >>2]; //we also need to backup nonvolatile memory as well, see DINO paper
};
}

#endif //EH_SIM_TASK_BASED_HPP
