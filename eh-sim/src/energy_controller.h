

#ifndef ENERGY_HARVESTING_TIMING_H
#define ENERGY_HARVESTING_TIMING_H

#include "stats.hpp"
#include "capacitor.hpp"
#include "voltage_trace.hpp"
#include "scheme/eh_scheme.hpp"
//handle energy and timing
class energyController {

public:
    energyController(ehsim::eh_scheme *_scheme, double const capacitance, double const maximum_voltage,
                     double const maximum_current);

    bool system_active();
    void time_stamped_voltage();

    void time_stamped_energy();

    void track_new_active_period();

    void run_instruction_and_track_energy(int instruction_ticks);


private:
    ehsim::stats_bundle stats{};

    uint64_t active_start;
    uint64_t cycles_til_ms = 0;
    uint64_t cycles_in_active_period = 0;
    ehsim::eh_scheme *scheme ;
    bool was_active;
};
#endif //ENERGY_HARVESTING_TIMING_H