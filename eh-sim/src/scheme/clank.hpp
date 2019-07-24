#ifndef EH_SIM_CLANK_HPP
#define EH_SIM_CLANK_HPP

#include "scheme/eh_scheme.hpp"
#include "scheme/data_sheet.hpp"
#include "capacitor.hpp"
#include "stats.hpp"

#include <thumbulator/memory.hpp>

#include <set>
#include <unordered_map>
#include <thumbulator/cpu.hpp>

namespace ehsim {

/**
 * Based on Clank: Architectural Support for Intermittent Computation.
 *
 * Only implements the read- and write-first buffers.
 */
class clank : public eh_scheme {
public:
  /**
   * Construct a default clank configuration.
   */
  clank() : clank(8, 8, 8000)
  {
  }

  clank(size_t rf_entries, size_t wf_entries, int watchdog_period)
      : battery(NVP_CAPACITANCE, MEMENTOS_MAX_CAPACITOR_VOLTAGE, MEMENTOS_MAX_CURRENT)
      , WATCHDOG_PERIOD(watchdog_period)
      , READFIRST_ENTRIES(rf_entries)
      , WRITEFIRST_ENTRIES(wf_entries)
      , MAX_BACKUP_ENERGY(CLANK_BACKUP_ARCH_ENERGY)
      , progress_watchdog(WATCHDOG_PERIOD)
  {
    assert(READFIRST_ENTRIES >= 1);
    assert(WRITEFIRST_ENTRIES >= 0);

    thumbulator::ram_load_hook = [this](
        uint32_t address, uint32_t data) -> uint32_t { return this->process_read(address, data); };

    thumbulator::ram_store_hook = [this](uint32_t address, uint32_t last_value,
        uint32_t value) -> uint32_t { return this->process_store(address, last_value, value); };
  }

  capacitor &get_battery() override
  {
    return battery;
  }

  uint32_t clock_frequency() const override
  {
    return CORTEX_M0PLUS_FREQUENCY;
  }

  double min_energy_to_power_on(stats_bundle *stats) override
  {
    return battery.maximum_energy_stored();
  }

  void execute_instruction(stats_bundle *stats) override
  {
    auto const elapsed_cycles = stats->cpu.cycle_count - last_tick;
    last_tick = stats->cpu.cycle_count;

    progress_watchdog -= elapsed_cycles;

    // clank's instruction energy is in Energy-per-Cycle
    auto const instruction_energy = CLANK_INSTRUCTION_ENERGY * elapsed_cycles;
    battery.consume_energy(instruction_energy);
    stats->models.back().energy_for_instructions += instruction_energy;
  }


  void execute_instruction(uint32_t cycles,stats_bundle *stats)
  {
  }
  bool is_active(stats_bundle *stats) override
  {
    if(battery.energy_stored() >= battery.maximum_energy_stored()) {
      power_on();
    } else if(battery.energy_stored() < MAX_BACKUP_ENERGY) {
      power_off();
    }

    return active;
  }

  bool will_backup(stats_bundle *stats) const override
  {
    if(battery.energy_stored() < MAX_BACKUP_ENERGY) {
      return false;
    }

    if(progress_watchdog <= 0) {
      return true;
    }

    return idempotent_violation;
  }

  uint64_t backup(stats_bundle *stats) override
  {
    auto &active_stats = stats->models.back();
    active_stats.num_backups++;

    active_stats.time_between_backups += stats->cpu.cycle_count - last_backup_cycle;
    last_backup_cycle = stats->cpu.cycle_count;

    // save architectural state
    architectural_state = thumbulator::cpu;
    std::copy(std::begin(thumbulator::RAM), std::end(thumbulator::RAM), std::begin(backup_RAM));
    std::copy(std::begin(thumbulator::FLASH_MEMORY), std::end(thumbulator::FLASH_MEMORY), std::begin(backup_FLASH));
    // reset the watchdog
    progress_watchdog = WATCHDOG_PERIOD;
    // clear idempotency-tracking buffers
    clear_buffers();
    // the backup has resolved the idempotancy violation and/or exception
    idempotent_violation = false;

    active_stats.energy_for_backups += CLANK_BACKUP_ARCH_ENERGY;
    battery.consume_energy(CLANK_BACKUP_ARCH_ENERGY);

    return CLANK_BACKUP_ARCH_TIME;
  }

  uint64_t restore(stats_bundle *stats) override
  {
    progress_watchdog = WATCHDOG_PERIOD;
    last_backup_cycle = stats->cpu.cycle_count;

    // restore saved architectural state
    thumbulator::cpu_reset();
    thumbulator::cpu = architectural_state;
    std::copy(std::begin(backup_RAM), std::end(backup_RAM), std::begin(thumbulator::RAM));
    std::copy(std::begin(backup_FLASH), std::end(backup_FLASH), std::begin(thumbulator::FLASH_MEMORY));
    stats->models.back().energy_for_restore = CLANK_RESTORE_ENERGY;
    battery.consume_energy(CLANK_RESTORE_ENERGY);

    // assume memory access latency for reads and writes is the same
    return CLANK_BACKUP_ARCH_TIME;
  }

  double estimate_progress(eh_model_parameters const &eh) const override
  {
    return estimate_eh_progress(eh, dead_cycles::average_case, CLANK_OMEGA_R, CLANK_SIGMA_R, CLANK_A_R,
        CLANK_OMEGA_B, CLANK_SIGMA_B, CLANK_A_B);
  }

    void execute_startup_routine()  override
    {
      architectural_state = thumbulator::cpu;
    }

private:
  capacitor battery;

  uint64_t last_backup_cycle = 0u;
  uint64_t last_tick = 0u;

  thumbulator::cpu_state architectural_state{};
  bool active = false;

  int const WATCHDOG_PERIOD;
  size_t const READFIRST_ENTRIES;
  size_t const WRITEFIRST_ENTRIES;
  double const MAX_BACKUP_ENERGY;

  int progress_watchdog;
  bool idempotent_violation = false;

  std::set<uint32_t> readfirst_buffer;
  std::set<uint32_t> writefirst_buffer;

  enum class operation { read, write };

  uint32_t backup_RAM[RAM_SIZE_BYTES >> 2];
  uint32_t backup_FLASH[FLASH_SIZE_BYTES>>2];

  void clear_buffers()
  {
    readfirst_buffer.clear();
    writefirst_buffer.clear();
  }

  void power_on()
  {
    active = true;
  }

  void power_off()
  {
    active = false;
    clear_buffers();
  }

  bool try_insert(std::set<uint32_t> *buffer, uint32_t const address, size_t const max_buffer_size)
  {
    if(buffer->size() < max_buffer_size) {
      buffer->insert(address);

      return true;
    }

    return false;
  }

  /**
   * Detection logic for idempotency violations.
   */
  void detect_violation(uint32_t address, operation op, uint32_t old_value, uint32_t value)
  {
    auto const readfirst_it = readfirst_buffer.find(address);
    auto const readfirst_hit = readfirst_it != readfirst_buffer.end();

    auto const writefirst_it = writefirst_buffer.find(address);
    auto const writefirst_hit = writefirst_it != writefirst_buffer.end();

    if(!readfirst_hit && !writefirst_hit) {
      // the memory access is in neither buffer

      // add the memory access to the appropriate buffer
      bool was_added = false;
      if(op == operation::read) {
        was_added = try_insert(&readfirst_buffer, address, READFIRST_ENTRIES);
      } else if(op == operation::write) {
        was_added = try_insert(&writefirst_buffer, address, WRITEFIRST_ENTRIES);
      }

      if(!was_added) {
        // idempotent violation - a buffer was full
        idempotent_violation = true;
      }
    } else if(op == operation::write && readfirst_hit) {
      // idempotent violation - write to read-dominated address
      idempotent_violation = true;
    }
  }

  uint32_t process_read(uint32_t address, uint32_t value)
  {
    detect_violation(address, operation::read, value, value);

    if(idempotent_violation && battery.energy_stored() < MAX_BACKUP_ENERGY) {
      power_off();
    }

    return value;
  }

  uint32_t process_store(uint32_t address, uint32_t old_value, uint32_t value)
  {
    detect_violation(address, operation::write, old_value, value);

    if(idempotent_violation && battery.energy_stored() < MAX_BACKUP_ENERGY) {
      power_off();

      return old_value;
    }

    return value;
  }
};
}

#endif //EH_SIM_CLANK_HPP
