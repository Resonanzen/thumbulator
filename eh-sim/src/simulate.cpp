#include "simulate.hpp"

#include "capacitor.hpp"
#include "voltage_trace.hpp"

#include <stdexcept>
#include <cstring>

extern "C" {
#include <thumbulator/cpu.h>
#include <thumbulator/sim_support.h>
}

namespace ehsim {

void load_program(char const *file_name)
{
  std::FILE *fd = std::fopen(file_name, "r");
  if(fd == nullptr) {
    throw std::runtime_error("Could not open binary file.\n");
  }

  std::fread(&FLASH_MEMORY, sizeof(uint32_t), sizeof(FLASH_MEMORY) / sizeof(uint32_t), fd);
  std::fclose(fd);
}

uint64_t cycles_to_ms(uint64_t const cycle_count)
{
  constexpr double CPU_PERIOD = 1.0 / CPU_FREQ;
  auto const time = CPU_PERIOD * cycle_count * 1000;

  return static_cast<uint64_t >(time);
}

uint64_t ms_to_cycles(uint64_t const ms)
{
  return ms * static_cast<uint64_t>(CPU_FREQ * 0.001);
}

void initialize_system(char const *binary_file)
{
  std::fprintf(stderr, "Simulating file %s\n", binary_file);
  std::fprintf(stderr, "Flash start:\t0x%8.8X\n", FLASH_START);
  std::fprintf(stderr, "Flash end:\t0x%8.8X\n", (FLASH_START + FLASH_SIZE));
  std::fprintf(stderr, "Ram start:\t0x%8.8X\n", RAM_START);
  std::fprintf(stderr, "Ram end:\t0x%8.8X\n", (RAM_START + RAM_SIZE));

  // Reset memory, then load program to memory
  std::memset(RAM, 0, sizeof(RAM));
  std::memset(FLASH_MEMORY, 0, sizeof(FLASH_MEMORY));
  load_program(binary_file);

  // Initialize CPU state
  cpu_reset();

  // PC seen is PC + 4
  cpu_set_pc(cpu_get_pc() + 0x4);
}

/**
 * Execute one instruction.
 *
 * @return Number of cycles to execute that instruction.
 */
uint32_t step_cpu()
{
  BRANCH_WAS_TAKEN = false;

  // Backup CPU state
  cpu_state lastCPU{};
  std::memcpy(&lastCPU, &cpu, sizeof(cpu_state));

  if((cpu_get_pc() & 0x1) == 0) {
    throw std::runtime_error("PC moved out of thumb mode.");
  }

  // fetch
  uint16_t instruction;
  fetch_instruction(cpu_get_pc() - 0x4, &instruction);
  // decode
  decode_result const decoded = decode(instruction);
  // execute, memory, and write-back
  uint32_t const instruction_ticks = exmemwb(instruction, &decoded);

  // advance to next PC
  if(!BRANCH_WAS_TAKEN) {
    if(cpu_get_pc() != lastCPU.gpr[15]) {
      throw std::runtime_error("Unexpected control flow change.");
    }

    cpu_set_pc(cpu_get_pc() + 0x2);
  } else {
    cpu_set_pc(cpu_get_pc() + 0x4);
  }

  return instruction_ticks;
}

double calculate_energy(double const voltage, double const capacitance)
{
  return 0.5 * capacitance * voltage * voltage;
}

stats_bundle simulate(char const *binary_file, char const *voltage_trace_file)
{
  initialize_system(binary_file);

  // stats tracking
  stats_bundle stats{};

  // energy harvesting
  //constexpr double EPSILON = 1.18e-3;
  constexpr double EPSILON = 1.18e-6;
  constexpr double CURRENT = EPSILON / 1.8;

  uint64_t current_ms = cycles_to_ms(stats.system.cycle_count);
  uint64_t last_ms = current_ms;
  voltage_trace power(voltage_trace_file);
  capacitor battery(4.7e-5);

  auto const initial_voltage = power.get_voltage(current_ms);
  auto const initial_energy = calculate_energy(initial_voltage, battery.capacitance());
  battery.harvest_energy(initial_energy);

  // Execute the program
  // Simulation will terminate when it executes insn == 0xBFAA
  while(!EXIT_INSTRUCTION_ENCOUNTERED) {
    if(battery.energy_stored() > EPSILON) {
      // active period
      auto const instruction_ticks = step_cpu();

      stats.cpu.instruction_count++;
      stats.cpu.cycle_count += instruction_ticks;
      stats.system.cycle_count += instruction_ticks;

      battery.consume_energy(EPSILON);
    } else {
      // charging period
      stats.system.cycle_count += ms_to_cycles(1);
    }

    current_ms = cycles_to_ms(stats.system.cycle_count);
    if(current_ms != last_ms) {
      last_ms = current_ms;

      auto const next_voltage = power.get_voltage(current_ms);
      auto const harvested_energy = calculate_energy(next_voltage, battery.capacitance());
      battery.harvest_energy(harvested_energy);
    }
  }

  return stats;
}
}