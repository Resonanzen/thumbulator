//
// Created by xulingx1 on 6/25/19.
//

#ifndef ENERGY_HARVESTING_TASK_HISTORY_H
#define ENERGY_HARVESTING_TASK_HISTORY_H

#include <map>
#include <vector>
#include "stats.hpp"
#include "simul_timer.h"
#include <chrono>
using namespace std::chrono_literals;
namespace ehsim{


    class task_history_tracker{
    public:
        task_history_tracker();
        void  update_if_task_change(uint64_t pc_of_task);

        void update_task_history(uint64_t pc_of_backup, stats_bundle *stats);

        //need to build accessor function, but for some reason it doesn't work in the for loop
        std::map<uint64_t, std::vector<int> > task_history;

        void task_failed();
    private:

        uint64_t current_task = 0;




        std::chrono::nanoseconds last_task_transition_time = 0ns;
        bool recently_switched_task = false;
    };


}

#endif //ENERGY_HARVESTING_TASK_HISTORY_H
