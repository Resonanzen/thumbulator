//
// Created by xulingx1 on 6/25/19.
//

#ifndef ENERGY_HARVESTING_TASK_HISTORY_H
#define ENERGY_HARVESTING_TASK_HISTORY_H

#include <map>
#include <vector>
#include "stats.hpp"

namespace ehsim{

    class task_history_tracker{
    public:
        task_history_tracker();
        //update the task history after encountering a WFI instruction
        void update_task_history(uint64_t pc_of_backup, stats_bundle *stats);

        //need to build accessor function, but for some reason it doesn't work in the for loop
        std::map<uint64_t, std::vector<int> > task_history;

        std::vector<uint64_t> get_task_lengths(){
            return task_lengths;
        }

        //update the task history after power shuts off during an active period
        void task_failed(stats_bundle *stats);

        uint64_t get_current_task();
    private:
        //pc of the current task
        uint64_t current_task = 0;

        //length of all the tasks run in terms of cycles, in order of when they were completed
        std::vector<uint64_t> task_lengths;
    };
}

#endif //ENERGY_HARVESTING_TASK_HISTORY_H
