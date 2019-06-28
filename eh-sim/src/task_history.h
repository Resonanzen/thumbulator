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


    struct task_data{
        std::vector<int> task_history;
        uint64_t forward_progress = 0;
        std::chrono::nanoseconds total_run_time = 0ms;
    };

    class task_history_tracker{
    public:

        void  update_if_task_change(uint64_t pc_of_task);

        void update_task_history(uint64_t pc_of_task, stats_bundle *stats);

        std::map<uint64_t, task_data> task_history_data;


        uint64_t get_current_task(){
            return current_task;
        };
        void task_failed();
    private:
        std::map<uint64_t, std::vector<int> > task_history;
        uint64_t current_task = 0;




        std::chrono::nanoseconds last_task_transition_time = 0ns;
        bool recently_switched_task = false;
    };


}

#endif //ENERGY_HARVESTING_TASK_HISTORY_H
