#include "simul_timer.h"
#include <iostream>

ehsim::simul_timer::simul_timer(uint64_t _system_frequency){
    system_frequency = _system_frequency;
}


std::chrono::nanoseconds ehsim::simul_timer::current_system_time() {
    return system_time;
}

void ehsim::simul_timer::inactive_tick() {
    system_time += 1ms;
    last_ms_mark += 1ms;
    elapsed_time += 1ms;
}


void ehsim::simul_timer::active_tick(uint64_t cycles){
    uint64_t elapsed_nanoseconds = (1.0/system_frequency)*cycles*1e9;

    std::chrono::nanoseconds time_to_increase(elapsed_nanoseconds);
    elapsed_time += time_to_increase;
    system_time += time_to_increase;
}

bool ehsim::simul_timer::harvest_while_active() {
//    std::cout <<"last_ms_mark: " << last_ms_mark.count()*1e-3 << "\n";
//    std::cout <<"system_time: " <<system_time.count()*1e-9 <<"\n";
    if (system_time - last_ms_mark > 1ms){
        last_ms_mark += 1ms;
        return true;
    }

    return false;

}
//get the elapsed time since last capacitor voltage update
//this zeroes out the elapsed_time class variable
std::chrono::nanoseconds ehsim::simul_timer::get_elapsed_time(){
    std::chrono::nanoseconds temp_elapsed_time = elapsed_time;
    elapsed_time = 0ns;
    return temp_elapsed_time;

};