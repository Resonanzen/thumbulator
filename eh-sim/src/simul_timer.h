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
/**
 *  Initialize system timer
 * @param _system_frequency
 */
        simul_timer(uint64_t _system_frequency);

        bool harvest_while_active();

        std::chrono::nanoseconds current_system_time(); //return system time
        void active_tick(uint64_t cycles); //increase system time by the length of a clock cycle
        void inactive_tick(); //increase system time by the length of 1ms
    private:

        std::chrono::milliseconds last_ms_mark = 0ms;
        std::chrono::nanoseconds system_time = 0ns; //nanoseconds since start of simulation



        uint64_t system_frequency;
    };
}

#endif //ENERGY_HARVESTING_SIMUL_TIMER_H
