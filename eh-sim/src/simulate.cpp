#include "simulate.hpp"

#include <thumbulator/cpu.hpp>
#include <thumbulator/memory.hpp>
#include "scheme/data_sheet.hpp"
#include "scheme/eh_scheme.hpp"
#include "capacitor.hpp"
#include "stats.hpp"
#include "voltage_trace.hpp"
#include "simul_timer.h"
#include "task_history.h"
#include <typeinfo>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <thumbulator/memory.hpp>
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
/**
 * Initialize system
 *
 * @return void
 */
void initialize_system(char const *binary_file, eh_scheme * scheme , stats_bundle* stats)
{
  // Reset memory, then load program to memory
  std::memset(thumbulator::RAM, 0, sizeof(thumbulator::RAM));
  std::memset(thumbulator::FLASH_MEMORY, 0, sizeof(thumbulator::FLASH_MEMORY));
  load_program(binary_file);


  scheme->execute_startup_routine();
  // Initialize CPU state
  thumbulator::cpu_reset();


  // PC seen is PC + 4
  thumbulator::cpu_set_pc(thumbulator::cpu_get_pc() + 0x4);



}

/**
 * Execute one instruction. Handle backup instruction
 *
 * @return Number of cycles to execute that instruction.
 */
uint32_t step_cpu(eh_scheme * scheme, stats_bundle *stats, task_history_tracker *task_history_tracker)
{
  thumbulator::BRANCH_WAS_TAKEN = false;

  if((thumbulator::cpu_get_pc() & 0x1) == 0) {
    printf("Oh no! Current PC: 0x%08X\n", thumbulator::cpu.gpr[15]);

    throw std::runtime_error("PC moved out of thumb mode.");
  }

  // store current pc
  auto current_pc = thumbulator::cpu_get_pc();

  uint32_t instruction_ticks = 0;

  //fetch the instruction to be run
  uint16_t instruction;
  thumbulator::fetch_instruction(thumbulator::cpu_get_pc() - 0x4, &instruction);

  //handling backup signal (WFI)
  if(instruction == 0xBF30){
      std::cout <<"BACKUP encountered at " << thumbulator::cpu_get_pc() - 0x5 << "\n";
      //assume backup takes 0 cycles for now, will be handled in scheme->backup(stats)
      task_history_tracker ->update_task_history(thumbulator::cpu_get_pc()-0x5, stats);
      stats->backup_requested = true;

  }else{

      stats->backup_requested = false;
      //decode the fetched memory (that's not a wfi instruction
      auto const decoded = thumbulator::decode(instruction);
      // execute, memory, and write-back
      instruction_ticks = thumbulator::exmemwb(instruction, &decoded);
  }
  // advance to next PC
  if(!thumbulator::BRANCH_WAS_TAKEN) {
    thumbulator::cpu_set_pc(thumbulator::cpu_get_pc() + 0x2);
  } else {
    //TODO: why does branch taken imply pc+4? ...smthing to do with pipelining
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
/**
 * Update statistics collected during an active period
 * @param stats
 * @param scheme
 */

void update_active_period_stats(ehsim::stats_bundle *stats, ehsim::eh_scheme * scheme){
  auto &active_period = stats->models.back();
  active_period.time_total = active_period.time_for_instructions +
                             active_period.time_for_backups + active_period.time_for_restores;
  active_period.energy_consumed = active_period.energy_for_instructions +
                                  active_period.energy_for_backups +
                                  active_period.energy_for_restore;
  active_period.progress =
          active_period.energy_forward_progress / active_period.energy_consumed;
  active_period.eh_progress = scheme->estimate_progress(eh_model_parameters(active_period));
}
/**
 * Backup system and collect relevant statistics to backup
 * @param stats
 * @param scheme
 * @param simul_timer
 */
void backup_and_collect_stats(ehsim::stats_bundle * stats, ehsim::eh_scheme *scheme, simul_timer * simul_timer){
  stats->recently_backed_up = true;
  stats->dead_tasks = 0;
  auto const backup_time = scheme->backup(stats);


  auto &active_stats = stats->models.back();
  active_stats.time_for_backups += backup_time;

  simul_timer->active_tick(backup_time);

  active_stats.energy_forward_progress = active_stats.energy_for_instructions;
  active_stats.time_forward_progress = stats->cpu.cycle_count - active_stats.active_start;
}

//TODO:: Make a charging graph curve the right way
/**
 * Harvest energy into the system
 * @param simul_timer
 * @param power
 * @param stats
 * @param scheme
 */
void harvest_energy_from_environment(simul_timer * simul_timer, ehsim::voltage_trace power, ehsim::stats_bundle *stats, eh_scheme *scheme){
  auto env_voltage = power.get_voltage(to_milliseconds(simul_timer->current_system_time()));
  auto available_energy = (env_voltage * env_voltage / 30000) * 0.001;//0.001 is a microsecond //using 30kOhm resistor
  // cap should not harvest if source voltage is higher than cap voltage
  //if (scheme->get_battery().voltage() < env_voltage) {
    auto battery_energy = scheme->get_battery().harvest_energy(available_energy);
//     std::cout << "Active_Harvested_Energy: " << simul_timer->current_system_time().count()*1E-9 << " "<<available_energy << "\n";
//     std::cout << stats->system.energy_harvested << "\n";
//     std::cout << "Energy_Harvested " << battery_energy << "\n";
     stats->system.energy_harvested += battery_energy;

  //}
}
/**
 * Start a new active period and collect relevant statistics
 * @param stats
 * @param scheme
 * @param simul_timer
 */
void start_new_active_period(stats_bundle *stats, eh_scheme *scheme, simul_timer * simul_timer){
  stats->models.emplace_back();
  stats->models.back().active_start = stats->cpu.cycle_count;
  stats->models.back().energy_start = scheme->get_battery().energy_stored();

  if(stats->cpu.instruction_count != 0) {
    // consume energy for restore
    auto const restore_time = scheme->restore(stats);
    simul_timer->active_tick(restore_time);
    stats->models.back().time_for_restores += restore_time;
  }
}


 /**
  * Update statistics after running an instruction in step_cpu
  * @param stats
  * @param instruction_ticks
  */
void update_stats_after_instruction(stats_bundle *stats, uint64_t instruction_ticks){
  stats->cpu.instruction_count++;
  stats->cpu.cycle_count += instruction_ticks;
  stats->models.back().time_for_instructions += instruction_ticks;
}


void update_final_stats(stats_bundle *stats, eh_scheme * scheme, simul_timer * simul_timer){
    auto &active_period = stats->models.back();

    active_period.time_total = active_period.time_for_instructions + active_period.time_for_backups +
                                   active_period.time_for_restores;


    active_period.energy_consumed = active_period.energy_for_instructions +
                                        active_period.energy_for_backups +
                                        active_period.energy_for_restore;

    active_period.progress = active_period.energy_forward_progress / active_period.energy_consumed;
      active_period.eh_progress = scheme->estimate_progress(eh_model_parameters(active_period));

    stats->system.energy_remaining = scheme->get_battery().energy_stored();
    stats->system.time = simul_timer->current_system_time();
}

/**
 * Check if you are making forward progress
 * @param stats
 * @return bool
 */
bool making_forward_progress(stats_bundle *stats){
  if (!stats->recently_backed_up){
    stats->dead_tasks++;
    if (stats->dead_tasks == 2){
      std::cout << "DEAD TASK! Not progressing" << "\n";
      return false;
    }
  }
  if (stats->recently_backed_up){
    stats->recently_backed_up = false;
  }

  return true;
}







stats_bundle simulate(char const *binary_file,
    ehsim::voltage_trace const &power,
    eh_scheme *scheme, bool full_sim, uint64_t active_periods_to_simulate, std::string scheme_select) {
  using namespace std::chrono_literals;


  // init stats
  stats_bundle stats{};
  stats.system.time = 0ns;
  int64_t active_periods = 0;
  simul_timer simul_timer(scheme->clock_frequency());
  task_history_tracker task_history_tracker;
  std::map<double,double> energy_time_map;

  // init system
  initialize_system(binary_file, scheme, &stats);
  auto was_active = false;


  std::cout.setf(std::ios::unitbuf);
  // Execute the program
  std::cout << "Starting simulation\n";
  // Simulation will terminate when it executes insn == 0xBFAA

  thumbulator::EXIT_INSTRUCTION_ENCOUNTERED = false;
  while (!thumbulator::EXIT_INSTRUCTION_ENCOUNTERED) {



    energy_time_map.insert({simul_timer.current_system_time().count()*1E-9, scheme->get_battery().energy_stored()});

    if (scheme->is_active(&stats)) {

      // system just powered on, start of active period
      if (!was_active) {
        active_periods++;
          if (active_periods == active_periods_to_simulate && !full_sim){
              break;
          }
        std::cout << "Powering on\n";
        //track the start of a new period
        start_new_active_period(&stats, scheme, &simul_timer);
      }
      was_active = true;

      // run one instruction
      auto const instruction_ticks = step_cpu(scheme, &stats, &task_history_tracker);
      simul_timer.active_tick(instruction_ticks);
      // update stats after instruction
      update_stats_after_instruction(&stats, instruction_ticks);

      // consume energy for execution
      scheme->execute_instruction(instruction_ticks, &stats);

      // execute backup
      if (scheme->will_backup(&stats)) {

        backup_and_collect_stats(&stats, scheme, &simul_timer);
      }

      if (simul_timer.harvest_while_active()) {
          harvest_energy_from_environment(&simul_timer, power, &stats, scheme);
      }

    } else {
      // system was just on, start of off period
      if (was_active) {

        //wipe out volatile elements of the system
        std::memset(thumbulator::RAM, 0, sizeof(thumbulator::RAM));
        thumbulator::cpu_reset();

        //by definition, if you've turned off, you've turned off in the middle of a task
        task_history_tracker.task_failed();

        fprintf(stderr, "Turning off at state pc= %X\n", thumbulator::cpu_get_pc() - 0x5);
        update_active_period_stats(&stats, scheme);
        std::cout << "Active period finished in " << stats.models.back().time_total << " cycles.\n";

        if (!making_forward_progress(&stats)) {
          break;
        }

      }
      was_active = false;

      simul_timer.inactive_tick();

      harvest_energy_from_environment(&simul_timer, power, &stats, scheme);
    }
  }

//  //print out task pattern data
  std::ofstream task_pattern_data;
  task_pattern_data.open("task_pattern_data.txt");
  //print out all of the task PCs:
  for (auto it = task_history_tracker.task_history.begin(); it != task_history_tracker.task_history.end();it++){
      task_pattern_data << it->first << " ";
  }
  task_pattern_data << "\n";

  task_pattern_data << "Task Iteration Result" << "\n";

  for (auto it = task_history_tracker.task_history.begin(); it != task_history_tracker.task_history.end(); it++) {


    std::vector <int> task_history = it->second;
    for (int i = 0; i< task_history.size(); i++){
       task_pattern_data << it->first <<" "<< i << " ";

      if (task_history[i] == 1){
        task_pattern_data << "Success";
      }else{
        task_pattern_data << "Fail";
      }
      task_pattern_data << "\n";
    }


  }
  task_pattern_data.close();

//
  //print out time/energy data

  std::ofstream time_energy_data;
  time_energy_data.open("time_energy_data.txt");
  for (auto it = energy_time_map.begin(); it != energy_time_map.end(); it++){
      time_energy_data <<"Time/Energy " << it->first << " " << it->second << "\n";
  }
  time_energy_data.close();
//
//
//
//
//
//  //dump out memory
//
//
//dump out all of ram,flash,and register values onto a file
  std::ofstream memory_dump;
  std::string file_name = scheme_select + "_memory_dump";
  memory_dump.open(file_name);
  memory_dump << "FLASH_MEMORY\n";
  for (int i = 0; i < FLASH_SIZE_ELEMENTS; i++){
      if (i % 10 == 0){
          memory_dump<< "\n";
      }
      memory_dump<< thumbulator::FLASH_MEMORY[i] << " ";


  }
  memory_dump<< "\n";
  memory_dump << "RAM memory: \n";
  for (int i = 0; i < RAM_SIZE_ELEMENTS; i++){
    if (i % 10 == 0){
      memory_dump<< "\n";
    }
    memory_dump<< thumbulator::RAM[i]<< " ";


  }

  memory_dump<< "\n";
  memory_dump << "\n register values: \n";
  for (int i = 0; i < 16; i++){
      memory_dump << thumbulator::cpu.gpr[i] << " ";
  }
//
//
  memory_dump.close();


  std::cout <<"Writing output files... \n";



  std::cout << "Done simulation\n";

  update_final_stats(&stats, scheme, &simul_timer);
  return stats;
}
}
