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
void initialize_system(char const *binary_file, eh_scheme * scheme)
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

  uint32_t instruction_ticks = 0;

  //fetch the instruction to be run
  uint16_t instruction;
  thumbulator::fetch_instruction(thumbulator::cpu_get_pc() - 0x4, &instruction);

  //handling backup signal (WFI)
  if(instruction == 0xBF30){
      //std::cout <<"BACKUP encountered at " << thumbulator::cpu_get_pc() - 0x5 << "\n";
      //assume backup takes 0 cycles for now, will be handled in scheme->backup(stats)
      task_history_tracker -> update_task_history(thumbulator::cpu_get_pc()-0x5, stats);
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

  stats->current_task_start_time = stats->cpu.cycle_count;
  auto &active_stats = stats->models.back();
  active_stats.time_for_backups += backup_time;
  active_stats.energy_forward_progress = active_stats.energy_for_instructions;
  active_stats.time_forward_progress = stats->cpu.cycle_count - active_stats.active_start;
  simul_timer->active_tick(backup_time);
  stats->cpu.cycle_count += backup_time;
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
  stats->current_task_start_time = stats->cpu.cycle_count;

  if(stats->cpu.instruction_count != 0) {
    // consume energy for restore
    auto const restore_time = scheme->restore(stats);
    simul_timer->active_tick(restore_time);
    stats->models.back().time_for_restores += restore_time;
    stats->cpu.cycle_count += restore_time;
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
      std::cout << "You can check the size of your tasks by turning off energy consumption in capacitor::consume, and printing out the task lengths in simulate()\n";
      return false;
    }
  }
  if (stats->recently_backed_up){
    stats->recently_backed_up = false;
  }

  return true;
}
//the value of the current source at this time
double get_current_input(ehsim::voltage_trace power, simul_timer simul_timer){
    auto env_voltage = power.get_voltage(to_milliseconds(simul_timer.current_system_time()));

    double current_source_value = env_voltage/(30000);

    return current_source_value;
}

//backup: PC, RAM, NVM, battery, stats, simul timer, energy time map, backup time map, task history tracker
sim_backup_bundle sim_backup(
    eh_scheme *scheme,
    stats_bundle stats,
    simul_timer simul,
    std::map<double, double> energy_time_map,
    std::map<double, double> backup_time_map,
    task_history_tracker task_history){

    sim_backup_bundle oriko_box{};
    oriko_box.backup_ARCHITECTURE = thumbulator::cpu;
    //std::copy(std::begin(thumbulator::RAM), std::end(thumbulator::RAM), std::begin(oriko_box.backup_RAM));
    //std::copy(std::begin(thumbulator::FLASH_MEMORY), std::end(thumbulator::FLASH_MEMORY), std::begin(oriko_box.backup_FLASH));
    oriko_box.backup_battery_voltage = scheme->get_battery().voltage();
    oriko_box.backup_battery_charge = scheme->get_battery().charge();
    oriko_box.backup_stats = stats;
    oriko_box.backup_simul_timer = simul;
    oriko_box.backup_energy_time_map = energy_time_map;
    oriko_box.backup_backup_time_map = backup_time_map;
    oriko_box.backup_task_history = task_history;
    return oriko_box;
}

void sim_restore(sim_backup_bundle oriko_box,
    eh_scheme *scheme,
    stats_bundle &stats,
    simul_timer &simul,
    std::map<double, double> &energy_time_map,
    std::map<double, double> &backup_time_map,
    task_history_tracker &task_history){

    thumbulator::cpu = oriko_box.backup_ARCHITECTURE;
    //std::copy(std::begin(scheme->backup_RAM), std::end(oriko_box.backup_RAM), std::begin(thumbulator::RAM));
    //std::copy(std::begin(oriko_box.backup_FLASH), std::end(oriko_box.backup_FLASH), std::begin(thumbulator::FLASH_MEMORY));
    scheme->restore(&stats);
    scheme->get_battery().set_voltage(oriko_box.backup_battery_voltage);
    scheme->get_battery().set_charge(oriko_box.backup_battery_charge);
    stats = oriko_box.backup_stats;
    simul = oriko_box.backup_simul_timer;
    energy_time_map = oriko_box.backup_energy_time_map ;
    backup_time_map = oriko_box.backup_backup_time_map;
    task_history = oriko_box.backup_task_history;
}

stats_bundle simulate(char const *binary_file,
    std::string output_folder,
    ehsim::voltage_trace const &power,
    eh_scheme *scheme,
    bool full_sim,
    bool oriko,
    bool yachi,
    uint64_t active_periods_to_simulate){

    using namespace std::chrono_literals;

    // init stats
    stats_bundle stats{};
    sim_backup_bundle oriko_box{};
    stats.system.time = 0ns;
    int64_t active_periods = 0;
    simul_timer simul_timer(scheme->clock_frequency());
    task_history_tracker task_history_tracker;
    std::map<double, double> energy_time_map; //record energy every ms
    std::map<double, double> backup_time_map; //record energy at time of backup
    bool just_backed_up = false;
    uint64_t yachi_counter = 0;
    uint64_t yachi_target = 0;
    uint64_t yachi_last_pc = 0;
    uint64_t yachi_reset = 0;
    std::chrono::nanoseconds beginnning_of_task{0};

    // init system
    initialize_system(binary_file, scheme);
    auto was_active = false;

    std::cout.setf(std::ios::unitbuf);
    // Execute the program
    std::cout << "Starting simulation\n";
    // Simulation will terminate when it executes insn == 0xBFAA
    thumbulator::EXIT_INSTRUCTION_ENCOUNTERED = false;

    // main execution loop
    while (!thumbulator::EXIT_INSTRUCTION_ENCOUNTERED) {
        /*recording voltage every millisecond. You can change how often the voltage is recorded, but the
        recording should be in milliseconds to compare with simulink */
        energy_time_map.insert({to_milliseconds(simul_timer.current_system_time()).count()*1e-3, scheme->get_battery().voltage()});

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
            if(just_backed_up){
                just_backed_up = false;
            }

            // consume energy for execution
            // scheme->execute_instruction(instruction_ticks, &stats)
            // execute backup
            if (scheme->will_backup(&stats)) {
                backup_time_map.insert({simul_timer.current_system_time().count()*1E-9, scheme->get_battery().voltage()});
                backup_and_collect_stats(&stats, scheme, &simul_timer);

                just_backed_up = true;

                //if sim is operating in oracle mode, need to back up sim stats to return to this point in case of failure
                //backup: PC, RAM, NVM, battery, stats, simul timer, energy time map, backup time map, task history tracker
                if(oriko){
                    oriko_box = sim_backup(scheme, stats, simul_timer, energy_time_map, backup_time_map, task_history_tracker);
                }
                if(yachi){
                    yachi_counter++;
                    if(yachi_counter != 0 && yachi_target != 0 && yachi_counter >= yachi_target){
                        stats.force_off = true;
                        yachi_counter = 0;
                        //yachi_reset++;
                    }
//                    if(yachi_reset == 10){
//                        yachi_reset = 0;
//                        yachi_target = 0;
//                    }
                    if(yachi_last_pc != task_history_tracker.get_current_task()){
                        yachi_target = 0;
                        yachi_last_pc = task_history_tracker.get_current_task();
                    }
                }
            }
            double input_current = get_current_input(power, simul_timer);
            double load_current_drain = 1.3e-3;

            double elapsed_time = simul_timer.get_elapsed_time().count() *1e-9;
            //  std::cout << elapsed_time << "\n";
            scheme->get_battery().charge(input_current,elapsed_time);
            scheme->get_battery().drain(load_current_drain, elapsed_time);

        }
        else {
            if(stats.force_off){
                stats.force_off = false;
            }
            // system was just on, start of off period
            if (was_active) {
                //check if you've turned off in the middle of a task
                if(!just_backed_up) {
                    //oracle sim: reverse failed task
                    if(oriko){
                        sim_restore(oriko_box, scheme, stats, simul_timer, energy_time_map, backup_time_map, task_history_tracker);
                        stats.force_off = true;
                        just_backed_up = true;
                        continue;
                    }
                    // not oracle: record failed task
                    else {
                        task_history_tracker.task_failed(&stats);
                        if(yachi){
                            yachi_target = yachi_counter;
                            yachi_counter = 0;
                        }
                    }

                }
                //wipe out volatile elements of the system
                std::memset(thumbulator::RAM, 0, sizeof(thumbulator::RAM));
                thumbulator::cpu_reset();

                update_active_period_stats(&stats, scheme);
                std::cout << "Active period finished in " << stats.models.back().time_total << " cycles.\n";

                if (!making_forward_progress(&stats)) {
                    break;
                }

            }
            was_active = false;
            simul_timer.inactive_tick();
            //update current conditions
            double input_current = get_current_input(power, simul_timer);
            double elapsed_time = simul_timer.get_elapsed_time().count() *1e-9;
            scheme->get_battery().charge(input_current,elapsed_time);
        }
    }
    update_final_stats(&stats, scheme, &simul_timer);

    //get benchmark name
    std::string binary_file_name(binary_file);
    size_t i = binary_file_name.rfind('/', binary_file_name.length());
    if (i != std::string::npos) {
        binary_file_name = binary_file_name.substr(i+1, binary_file_name.length() - i);
    }
    i = binary_file_name.rfind('.', binary_file_name.length());
    if (i != std::string::npos) {
        binary_file_name = binary_file_name.substr(0, i);
    }
    if(oriko){
        binary_file_name = binary_file_name + "_oracle";
    }
    if(yachi){
        binary_file_name = binary_file_name + "_pred";
    }

    //print out task pattern data
    //first, record last task as success
    task_history_tracker.update_task_history(0, &stats);
    std::ofstream task_pattern_data;
    task_pattern_data.open(output_folder + "task_pattern_data_" + binary_file_name + ".txt");
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
            }
            else{
                task_pattern_data << "Fail";
            }
            task_pattern_data << "\n";
        }
    }
    task_pattern_data.close();

    //print out time/energy data
    std::ofstream time_energy_data;
    time_energy_data.open(output_folder + "time_energy_data_" + binary_file_name + ".txt");
    for (auto it = energy_time_map.begin(); it != energy_time_map.end(); it++){
        time_energy_data.precision(10);
        time_energy_data <<"Time/Energy " << it->first << " " << it->second << "\n";
    }

    for (auto it = backup_time_map.begin(); it != backup_time_map.end(); it++){
        time_energy_data << "Backup " << it->first << " " << it->second << "\n";
    }
    time_energy_data.close();

    //print out all of ram,flash,and register values onto a file
    std::ofstream memory_dump;
    std::string file_name = scheme->get_scheme_name() + "_memory_dump";
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
    memory_dump.close();

    //print out time ratio data
    uint64_t cycles_for_instructions = 0;
    uint64_t cycles_for_backup = 0;
    uint64_t cycles_for_restore = 0;
    for (auto it = stats.models.begin(); it != stats.models.end(); it++){
        cycles_for_instructions += it->time_for_instructions;
        cycles_for_backup += it->time_for_backups;
        cycles_for_restore += it->time_for_restores;
    }
    std::ofstream time_ratio_data;
    time_ratio_data.open(output_folder + "time_ratio_data_" + binary_file_name + ".txt");
    time_ratio_data << "Total elapsed time(ns): " << (stats.system.time).count() <<"\n";
    time_ratio_data << "Total time(ns) for forward progress: " << get_time((cycles_for_instructions - stats.dead_cycles), scheme->clock_frequency()).count() << "\n";
    time_ratio_data << "Total time(ns) for backup: " << get_time(cycles_for_backup, scheme->clock_frequency()).count()<< "\n";
    time_ratio_data << "Total time(ns) for restore: " << get_time(cycles_for_restore, scheme->clock_frequency()).count() << "\n";
    time_ratio_data << "Total time(ns) for re-execution (dead cycles): " << get_time(stats.dead_cycles, scheme->clock_frequency()).count() << "\n";
    time_ratio_data << "CPU instructions executed (excluding for backups and restores): " << stats.cpu.instruction_count << "\n";
    //time_ratio_data << "CPU time (cycles): " << stats.cpu.cycle_count << "\n";
    //time_ratio_data << "Energy harvested (J): " << stats.system.energy_harvested  << "\n";
    //time_ratio_data << "Energy remaining (J): " << stats.system.energy_remaining  << "\n";
    time_ratio_data.close();

    std::cout << "Done simulation\n";

    return stats;
}
}
