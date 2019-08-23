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
    /**
     * Return whether or not it is time for the system to harvest during active period
     * @return
     */

        bool harvest_while_active();
        /**
         * Return the system time
         * @return std::chrono::nanoseconds
         */

        std::chrono::nanoseconds get_elapsed_time();



        std::chrono::nanoseconds current_system_time(); //return system time

        /**
         * Increase system time by the number of cycles the last operation took (instruction, backup, restore)
         * @param cycles
         */
        void active_tick(uint64_t cycles); //increase system time by the length of a clock cycle
        /**
         * Increase system time by a millisecond
         */
        void inactive_tick(); //increase system time by the length of 1ms
    private:

        std::chrono::milliseconds last_ms_mark = 0ms;
        std::chrono::nanoseconds system_time = 0ns; //nanoseconds since start of simulation

        uint64_t system_frequency;
        //elapsed time since last voltage condition update
        std::chrono::nanoseconds elapsed_time = 0ns;
    };
}

#endif //ENERGY_HARVESTING_SIMUL_TIMER_H
