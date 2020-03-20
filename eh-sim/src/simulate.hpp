#ifndef EH_SIM_SIMULATE_HPP
#define EH_SIM_SIMULATE_HPP

#include <chrono>
#include <cstdint>
#include <string>
#include <thumbulator/memory.hpp>
#include <thumbulator/cpu.hpp>
#include "stats.hpp"
#include "capacitor.hpp"
#include "simul_timer.h"
#include "task_history.h"

namespace ehsim {

class eh_scheme;
struct stats_bundle;
class voltage_trace;

/**
 * Simulate an energy harvesting device.
 *
 * @param binary_file The path to the application binary file.
 * @param power The power supply over time.
 * @param scheme The energy harvesting scheme to use.
 *
 * @return The statistics tracked during the simulation.
 */
stats_bundle simulate(char const *binary_file,
    std::string output_folder,
    ehsim::voltage_trace const &power,
    eh_scheme *scheme, bool full_sim, bool oriko,
    bool yachi,
    uint64_t active_periods_to_simulate);


    std::chrono::nanoseconds get_time(uint64_t const cycle_count, uint32_t const frequency);
    std::chrono::milliseconds to_milliseconds(std::chrono::nanoseconds const &time);



    void update_active_period_stats(ehsim::stats_bundle *stats);
    void update_backup_stats(ehsim::stats_bundle *stats);


struct sim_backup_bundle{

    double backup_battery_voltage;
    double backup_battery_charge;
    //uint32_t backup_FLASH[FLASH_SIZE_BYTES>>2];
    //uint32_t backup_RAM[RAM_SIZE_BYTES >> 2];
    thumbulator::cpu_state backup_ARCHITECTURE = thumbulator::cpu;
    std::map<double, double> backup_energy_time_map;
    std::map<double, double> backup_backup_time_map;
    stats_bundle backup_stats{};
    simul_timer backup_simul_timer;
    task_history_tracker backup_task_history;
};
}

#endif //EH_SIM_SIMULATE_HPP
