

#include "energy_controller.h"
#include <chrono>
#include "simulate.hpp"
#include <iostream>

using namespace std::chrono_literals;

energyController::energyController(ehsim::eh_scheme * _scheme,  ehsim::voltage_trace const *_power){
    //initialize the time to 0
    stats.system.time = 0ns;
    scheme = _scheme;
    power = _power;
}

bool energyController::system_active() {

    return scheme->is_active(&stats);


}

void energyController::track_new_active_period() {
    stats.models.emplace_back();
    active_start = stats.cpu.cycle_count;
    stats.models.back().energy_start = scheme->get_battery().energy_stored();

    if (stats.cpu.instruction_count != 0) {
        // consume energy for restore
        auto const restore_time = scheme->restore(&stats);

        elapsed_cycles += restore_time;
        temp_elapsed_cycles += restore_time;

        stats.models.back().time_for_restores += restore_time;
    }

}
void energyController::run_instruction_and_track_energy(int instruction_ticks) {
    // update stats
    stats.cpu.instruction_count++;
    stats.cpu.cycle_count += instruction_ticks;
    //std::cout << "ticks: " << instruction_ticks << "\n";
    stats.models.back().time_for_instructions += instruction_ticks;
    elapsed_cycles += instruction_ticks;
    temp_elapsed_cycles += instruction_ticks;

    // consume energy for execution
    scheme->execute_instruction(&stats);

    // execute backup
    if(scheme->will_backup(&stats)) {
        // consume energy for backing up
        stats.recently_backed_up = true;
        stats.dead_tasks = 0;
        auto const backup_time = scheme->backup(&stats);
        elapsed_cycles += backup_time;
        temp_elapsed_cycles += backup_time;

        auto &active_stats = stats.models.back();
        active_stats.time_for_backups += backup_time;
        active_stats.energy_forward_progress = active_stats.energy_for_instructions;
        active_stats.time_forward_progress = stats.cpu.cycle_count - active_start;
    }
    auto elapsed_time = ehsim::get_time(elapsed_cycles, scheme->clock_frequency());

    // check if time has passed the ms threshold while in active mode
    // if time crosses the ms boundary, harvest energy
    auto elapsed_ms = ehsim::to_milliseconds(stats.system.time + elapsed_time) - ehsim::to_milliseconds(stats.system.time);
    for (int x = 0; x < elapsed_ms.count(); x++)
    {
        auto env_voltage = power->get_voltage(ehsim::to_milliseconds(stats.system.time + std::chrono::milliseconds(x)));
        auto available_energy = (env_voltage * env_voltage / 30000) * 0.001;
        // cap should not harvest if source voltage is higher than cap voltage
        if (scheme->get_battery().voltage() < env_voltage) {
            //auto battery_energy = battery.harvest_energy(available_energy);
            std::cout << "Active_Harvested_Energy: " << stats.system.time.count()*1E-9 << " "<<available_energy << "\n";

            //  stats.system.energy_harvested += battery_energy;
        }
    }
    stats.system.time += elapsed_time;
    // temp stat: system time, battery energy
    //temp_stats.emplace_back(std::make_tuple(stats.system.time.count(), battery.energy_stored()*1.e9)); //need to deal

}



void energy_control::run_instruction_and_track_energy(int instruction_ticks, ehsim::stats_bundle &stats, uint64_t &elapsed_cycles, uint64_t & temp_elapsed_cycles, ehsim::eh_scheme *scheme, ehsim::voltage_trace const power) {
    // update stats
    stats.cpu.instruction_count++;
    stats.cpu.cycle_count += instruction_ticks;
    //std::cout << "ticks: " << instruction_ticks << "\n";
    stats.models.back().time_for_instructions += instruction_ticks;
    elapsed_cycles += instruction_ticks;
    temp_elapsed_cycles += instruction_ticks;

    // consume energy for execution
    scheme->execute_instruction(&stats);

    // execute backup
    if(scheme->will_backup(&stats)) {
        // consume energy for backing up
        stats.recently_backed_up= true;
        stats.dead_tasks = 0;
        auto const backup_time = scheme->backup(&stats);
        elapsed_cycles += backup_time;
        temp_elapsed_cycles += backup_time;

//        auto &active_stats = stats.models.back();
//        active_stats.time_for_backups += backup_time;
//        active_stats.energy_forward_progress = active_stats.energy_for_instructions;
//        active_stats.time_forward_progress = stats.cpu.cycle_count - active_start;
    }
    auto elapsed_time = ehsim::get_time(elapsed_cycles, scheme->clock_frequency());

    // check if time has passed the ms threshold while in active mode
    // if time crosses the ms boundary, harvest energy
    auto elapsed_ms = ehsim::to_milliseconds(stats.system.time + elapsed_time) - ehsim::to_milliseconds(stats.system.time);
    for (int x = 0; x < elapsed_ms.count(); x++)
    {
        auto env_voltage = power.get_voltage(ehsim::to_milliseconds(stats.system.time + std::chrono::milliseconds(x)));
        auto available_energy = (env_voltage * env_voltage / 30000) * 0.001;
        // cap should not harvest if source voltage is higher than cap voltage
        if (scheme->get_battery().voltage() < env_voltage) {
            //auto battery_energy = battery.harvest_energy(available_energy);
            std::cout << "Active_Harvested_Energy: " << stats.system.time.count()*1E-9 << " "<<available_energy << "\n";

            //  stats.system.energy_harvested += battery_energy;
        }
    }
    stats.system.time += elapsed_time;
    // temp stat: system time, battery energy
    //temp_stats.emplace_back(std::make_tuple(stats.system.time.count(), battery.energy_stored()*1.e9)); //need to deal

}


void energy_control::harvest_while_inactive(ehsim::stats_bundle &stats, ehsim::eh_scheme * scheme, ehsim::voltage_trace const power) {
    // harvest energy while off: jump clock to next ms
    stats.system.time = ehsim::to_milliseconds(stats.system.time) + 1ms;
    // get voltage based current time
    auto env_voltage = power.get_voltage(ehsim::to_milliseconds(stats.system.time));
    auto harvested_energy = (env_voltage * env_voltage / 30000) * 0.001;
    scheme->get_battery().harvest_energy(harvested_energy);
    stats.system.energy_harvested += harvested_energy;
    std::cout << "Harvested_Energy: " << stats.system.time.count() *1E-9<< " "<<harvested_energy << "\n";
    //std::cout << "Powered off. Time (s): " << stats.system.time.count() * 1e-9 << " Current energy (nJ): " << battery.energy_stored() * 1e9 << "\n";
    // temp stat: system time, battery energy
    //temp_stats.emplace_back(std::make_tuple(stats.system.time.count(), battery.energy_stored()*1.e9)); //need to fix this
}
//
// Created by xulingx1 on 6/14/19.
//

