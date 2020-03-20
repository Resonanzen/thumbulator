//
// Created by xulingx1 on 6/25/19.
//

#include "task_history.h"

namespace ehsim{
    task_history_tracker::task_history_tracker() {

        //initialize task_history_vector with initial task
        std::vector<int> task_history_vector;
        task_history.insert({current_task, task_history_vector});
    }

    void task_history_tracker::update_task_history(uint64_t pc_of_backup, ehsim::stats_bundle *stats) {
        //update task history of the last task
        task_history.find(current_task)->second.push_back(1);

        //check whether or not a new task has started
        //if it's different then start tracking a new task
        if (pc_of_backup != current_task){
            current_task = pc_of_backup;
            std::vector<int> task_history_vector;
            //insert will do nothing if new pc already seen before
            task_history.insert({current_task, task_history_vector});
        }

        //update the task length
        task_lengths.push_back(stats->cpu.cycle_count - stats->current_task_start_time);
    }

    void task_history_tracker::task_failed(ehsim::stats_bundle *stats) {
        task_history.find(current_task)->second.push_back(0);
        stats->dead_cycles += stats->cpu.cycle_count - stats->current_task_start_time;
    }

    uint64_t task_history_tracker::get_num_failed() {
        //task_history.find(current_task)->second.length();
        return 0;
    };
}