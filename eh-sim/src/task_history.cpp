//
// Created by xulingx1 on 6/25/19.
//

#include "task_history.h"
#include "simul_timer.h"
#include <vector>
#include <iostream>

namespace ehsim{

task_history_tracker::task_history_tracker() {

    //initialize task_history_vector with initial task
    std::vector<int> task_history_vector;
    task_history.insert({current_task, task_history_vector});
}

void task_history_tracker::update_task_history(uint64_t pc_of_backup, ehsim::stats_bundle *stats) {
    //check whether or not a new task has started
    //if it's different then start tracking a new task
    if (pc_of_backup != current_task){

        task_history.find(current_task)->second.push_back(1);

        current_task = pc_of_backup;

        std::vector<int> task_history_vector;

        task_history.insert({current_task, task_history_vector});
    }

    //update task history of the current task
    task_history.find(current_task)->second.push_back(1);
}


void task_history_tracker::task_failed() {
    task_history.find(current_task)->second.push_back(0);

}




}