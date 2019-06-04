#include "simulate.hpp"

#include <thumbulator/cpu.hpp>
#include <thumbulator/memory.hpp>

#include "scheme/eh_scheme.hpp"
#include "capacitor.hpp"
#include "stats.hpp"
#include "voltage_trace.hpp"

#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>

namespace ehsim {

void load_program(char const *file_name)
{
  std::FILE *fd = std::fopen(file_name, "r");
  if(fd == nullptr) {
    throw std::runtime_error("Could not open binary file.\n");
  }

  std::fread(&thumbulator::FLASH_MEMORY, sizeof(uint32_t),
      sizeof(thumbulator::FLASH_MEMORY) / sizeof(uint32_t), fd);
  std::fclose(fd);
}

void initialize_system(char const *binary_file)
{
  // Reset memory, then load program to memory
  std::memset(thumbulator::RAM, 0, sizeof(thumbulator::RAM));
  std::memset(thumbulator::FLASH_MEMORY, 0, sizeof(thumbulator::FLASH_MEMORY));
  load_program(binary_file);

  // Initialize CPU state
  thumbulator::cpu_reset();

  // PC seen is PC + 4
  thumbulator::cpu_set_pc(thumbulator::cpu_get_pc() + 0x4);
}

/**
 * Execute one instruction.
 *
 * @return Number of cycles to execute that instruction.
 */
uint32_t step_cpu(std::map<int, int> &temp_pc_map)
{
  thumbulator::BRANCH_WAS_TAKEN = false;

  if((thumbulator::cpu_get_pc() & 0x1) == 0) {
    printf("Oh no! Current PC: 0x%08X\n", thumbulator::cpu.gpr[15]);
    throw std::runtime_error("PC moved out of thumb mode.");
  }

  // store current pc
  auto current_pc = thumbulator::cpu_get_pc();
  temp_pc_map[current_pc]++;

  // fetch
  uint16_t instruction;
  thumbulator::fetch_instruction(thumbulator::cpu_get_pc() - 0x4, &instruction);
  // decode
  auto const decoded = thumbulator::decode(instruction);
  // execute, memory, and write-back
  uint32_t const instruction_ticks = thumbulator::exmemwb(instruction, &decoded);

  // advance to next PC
  if(!thumbulator::BRANCH_WAS_TAKEN) {
    thumbulator::cpu_set_pc(thumbulator::cpu_get_pc() + 0x2);
  } else {
    //TODO: why does branch taken imply pc+4?
    thumbulator::cpu_set_pc(thumbulator::cpu_get_pc() + 0x4);
  }

  return instruction_ticks;
}

std::chrono::nanoseconds get_time(uint64_t const cycle_count, uint32_t const frequency)
{
  double const CPU_PERIOD = 1.0 / frequency;
  auto const time = static_cast<uint64_t>(CPU_PERIOD * cycle_count * 1e9);

  return std::chrono::nanoseconds(time);
}

// duration_cast from smaller interval to larger interval truncates (floors) the result
std::chrono::milliseconds to_milliseconds(std::chrono::nanoseconds const &time)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(time);
}

template<typename A, typename B>
std::pair<B,A> flip_pair(const std::pair<A,B> &p)
{
  return std::pair<B,A>(p.second, p.first);
}

template<typename A, typename B>
std::multimap<B,A> flip_map(const std::map<A,B> &src)
{
  std::multimap<B,A> dst;
  std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()),
                 flip_pair<A,B>);
  return dst;
}

stats_bundle simulate(char const *binary_file,
    ehsim::voltage_trace const &power,
    eh_scheme *scheme)
{
  using namespace std::chrono_literals;

  // init stats
  stats_bundle stats{};
  stats.system.time = 0ns;
  std::vector<std::tuple<uint64_t, uint64_t>> temp_stats;
  std::map<int, int> temp_pc_map;
  uint64_t active_start = 0u;
  uint64_t elapsed_cycles = 0;
  uint64_t temp_elapsed_cycles = 0;

  // init system
  initialize_system(binary_file);
  auto &battery = scheme->get_battery();
  auto was_active = false;

  // frequency in Hz, sample period in ms
  auto cycles_per_sample = static_cast<uint64_t>(
      scheme->clock_frequency() * std::chrono::duration<double>(power.sample_period()).count());
  std::cout.setf(std::ios::unitbuf);
  std::cout << "cycles per sample: " << cycles_per_sample << "\n";

  // Execute the program
  // Simulation will terminate when it executes insn == 0xBFAA
  std::cout << "Starting simulation\n";
  while(!thumbulator::EXIT_INSTRUCTION_ENCOUNTERED) {
    // init stats
    elapsed_cycles = 0;

    // system on
    if(scheme->is_active(&stats)) {
      // system just powered on, start of active period
      if(!was_active) {
        std::cout << "Powering on\n";
        // active period stats
        stats.models.emplace_back();
        active_start = stats.cpu.cycle_count;
        stats.models.back().energy_start = battery.energy_stored();

        if(stats.cpu.instruction_count != 0) {
          // consume energy for restore
          auto const restore_time = scheme->restore(&stats);

          elapsed_cycles += restore_time;
          temp_elapsed_cycles += restore_time;

          stats.models.back().time_for_restores += restore_time;
        }
      }
      was_active = true;

      // run one instruction
      auto const instruction_ticks = step_cpu(temp_pc_map);

      // update stats
      stats.cpu.instruction_count++;
      stats.cpu.cycle_count += instruction_ticks;
      stats.models.back().time_for_instructions += instruction_ticks;
      elapsed_cycles += instruction_ticks;
      temp_elapsed_cycles += instruction_ticks;

      // consume energy for execution
      scheme->execute_instruction(&stats);

      // execute backup
      if(scheme->will_backup(&stats)) {
        // consume energy for backing up
        auto const backup_time = scheme->backup(&stats);
        elapsed_cycles += backup_time;
        temp_elapsed_cycles += backup_time;

        auto &active_stats = stats.models.back();
        active_stats.time_for_backups += backup_time;
        active_stats.energy_forward_progress = active_stats.energy_for_instructions;
        active_stats.time_forward_progress = stats.cpu.cycle_count - active_start;
      }
      auto elapsed_time = get_time(elapsed_cycles, scheme->clock_frequency());

      // check if time has passed the ms threshold while in active mode
      // if time crosses the ms boundary, harvest energy
      auto elapsed_ms = to_milliseconds(stats.system.time + elapsed_time) - to_milliseconds(stats.system.time);
      for (int x = 0; x < elapsed_ms.count(); x++)
      {
        auto env_voltage = power.get_voltage(to_milliseconds(stats.system.time + std::chrono::milliseconds(x)));
        auto available_energy = (env_voltage * env_voltage / 30000) * 0.001;
        // cap should not harvest if source voltage is higher than cap voltage
        if (battery.voltage() < env_voltage) {
          auto battery_energy = battery.harvest_energy(available_energy);
          stats.system.energy_harvested += battery_energy;
        }
      }
      stats.system.time += elapsed_time;
      // temp stat: system time, battery energy
      temp_stats.emplace_back(std::make_tuple(stats.system.time.count(), battery.energy_stored()*1.e9));
    }

    // system off
    else {
      // system was just on, start of off period
      if(was_active) {
        std::cout << "Active period finished in " << temp_elapsed_cycles << " cycles.\n";
        temp_elapsed_cycles = 0;
        // we just powered off
        auto &active_period = stats.models.back();
        active_period.time_total = active_period.time_for_instructions +
                                   active_period.time_for_backups + active_period.time_for_restores;
        active_period.energy_consumed = active_period.energy_for_instructions +
                                        active_period.energy_for_backups +
                                        active_period.energy_for_restore;
        active_period.progress =
            active_period.energy_forward_progress / active_period.energy_consumed;
        active_period.eh_progress = scheme->estimate_progress(eh_model_parameters(active_period));
      }
      was_active = false;

      // harvest energy while off: jump clock to next ms
      stats.system.time = to_milliseconds(stats.system.time) + 1ms;
      // get voltage based current time
      auto env_voltage = power.get_voltage(to_milliseconds(stats.system.time));
      auto harvested_energy = (env_voltage * env_voltage / 30000) * 0.001;
      battery.harvest_energy(harvested_energy);
      stats.system.energy_harvested += harvested_energy;
      //std::cout << "Powered off. Time (s): " << stats.system.time.count() * 1e-9 << " Current energy (nJ): " << battery.energy_stored() * 1e9 << "\n";
      // temp stat: system time, battery energy
      temp_stats.emplace_back(std::make_tuple(stats.system.time.count(), battery.energy_stored()*1.e9));
    }
  }

  // TODO: remove temp stats output
  std::ofstream out("temp_stats.out");
  out.setf(std::ios::fixed);
  out << "time, energy\n";
  for (auto const &value : temp_stats)
  {
    out << std::get<0>(value) << "," << std::get<1>(value) << "\n";
  }

  std::multimap<int, int> temp_sorted_pc_map = flip_map(temp_pc_map);

  std::cout << "Done simulation\n";

  auto &active_period = stats.models.back();
  active_period.time_total = active_period.time_for_instructions + active_period.time_for_backups +
                             active_period.time_for_restores;

  active_period.energy_consumed = active_period.energy_for_instructions +
                                  active_period.energy_for_backups +
                                  active_period.energy_for_restore;

  active_period.progress = active_period.energy_forward_progress / active_period.energy_consumed;
  active_period.eh_progress = scheme->estimate_progress(eh_model_parameters(active_period));

  stats.system.energy_remaining = battery.energy_stored();

  return stats;
}
}
