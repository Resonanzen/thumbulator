

#include "energy_controller.h"
#include <chrono>

using namespace std::chrono_literals;

energyController::energyController(ehsim::eh_scheme * _scheme, double const capacitance, double const maximum_voltage, double const maximum_current){
    //initialize the time to 0
    stats.system.time = 0ns;
    scheme = _scheme;

}

bool energyController::system_active() {

    return scheme->is_active(&stats);


}

void energyController::track_new_active_period() {
    stats.models.emplace_back();
    active_start = stats.cpu.cycle_count;
    stats.models.back().energy_start = scheme->get_battery().energy_stored();

    if(stats.cpu.instruction_count != 0) {
        // consume energy for restore
        auto const restore_time = scheme->restore(&stats);

        cycles_til_ms += restore_time;
        cycles_in_active_period += restore_time;

        stats.models.back().time_for_restores += restore_time;
    }
}


void energyController::run_instruction_and_track_energy(int instruction_ticks) {
    stats.cpu.instruction_count++;
    stats.cpu.cycle_count += instruction_ticks;
    //std::cout << "ticks: " << instruction_ticks << "\n";
    stats.models.back().time_for_instructions += instruction_ticks;
    cycles_til_ms += instruction_ticks;
    cycles_in_active_period += instruction_ticks;

    // consume energy for execution
    scheme->execute_instruction(&stats);

    // execute backup
    if(scheme->will_backup(&stats)) {
        // consume energy for backing up
        stats.recentlyBackedUp = true;
        stats.deadTasks = 0;
        auto const backup_time = scheme->backup(&stats);
        cycles_til_ms += backup_time;
        cycles_in_active_period += backup_time;

        auto &active_stats = stats.models.back();
        active_stats.time_for_backups += backup_time;
        active_stats.energy_forward_progress = active_stats.energy_for_instructions;
        active_stats.time_forward_progress = stats.cpu.cycle_count - active_start;
    }
}


//
// Created by xulingx1 on 6/14/19.
//

