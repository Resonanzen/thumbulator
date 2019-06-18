

#ifndef ENERGY_HARVESTING_TIMING_H
#define ENERGY_HARVESTING_TIMING_H

#include "stats.hpp"
#include "capacitor.hpp"
#include "voltage_trace.hpp"
#include "scheme/eh_scheme.hpp"
//handle energy and timing
class energyController {

public:
    energyController(ehsim::eh_scheme *_scheme,  ehsim::voltage_trace const *_power);

    bool system_active();
    void time_stamped_voltage();

    void time_stamped_energy();

    void track_new_active_period();

    void run_instruction_and_track_energy(int instruction_ticks);
    void harvest_while_inactive();
    ehsim::stats_bundle stats{};
private:


    uint64_t active_start;
    uint64_t elapsed_cycles = 0;
    uint64_t temp_elapsed_cycles= 0;
    ehsim::eh_scheme *scheme ;
    bool was_active;

    ehsim::voltage_trace const * power;
};
#endif //ENERGY_HARVESTING_TIMING_H