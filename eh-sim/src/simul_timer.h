//
// Created by xulingx1 on 6/18/19.
//

#ifndef ENERGY_HARVESTING_SIMUL_TIMER_H
#define ENERGY_HARVESTING_SIMUL_TIMER_H

#include <chrono>

using namespace std::chrono_literals;
namespace ehsim {

    class simul_timer {
    public:
        //Initialize simul timer with system frequency
        simul_timer();
        simul_timer(uint64_t _system_frequency);

        //Return whether or not it is time for the system to harvest during active period
        bool harvest_while_active();
        //Return the elapsed time since last capacitor voltage update
        std::chrono::nanoseconds get_elapsed_time();
        //Return the system time
        std::chrono::nanoseconds current_system_time();

        //Increase system time by the number of cycles the last operation took (instruction, backup, restore)
        void active_tick(uint64_t cycles);
        //Increase system time by a millisecond
        void inactive_tick();

    private:
        std::chrono::milliseconds last_ms_mark = 0ms;
        //nanoseconds since start of simulation
        std::chrono::nanoseconds system_time = 0ns;

        uint64_t system_frequency;
        //elapsed time since last voltage condition update
        std::chrono::nanoseconds elapsed_time = 0ns;
    };
}

#endif //ENERGY_HARVESTING_SIMUL_TIMER_H
