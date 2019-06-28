//
// Created by xulingx1 on 6/25/19.
//

#include "task_history.h"
#include "simul_timer.h"
#include <vector>
#include <iostream>

namespace ehsim{

void task_history_tracker::update_if_task_change(uint64_t pc_of_task) {

}

void task_history_tracker::update_task_history(uint64_t pc_of_task, ehsim::stats_bundle *stats) {

    //check whether or not a new task has started
    if (pc_of_task != current_task){


        //update the run time of the previous task



        current_task = pc_of_task;

        std::vector<int> task_history_vector;

        task_data data;
        task_history_data.insert({current_task,data});



        task_history.insert({current_task, task_history_vector});



        recently_switched_task = true;
    }else {
        recently_switched_task = false;
    }


    //update task history
    if (!recently_switched_task){
        //std::cout << simulTimer->current_system_time().count() << "\n";

            task_history.find(current_task)->second.push_back(1);
            task_history_data.find(current_task)->second.task_history.push_back((1));


    }

}

void task_history_tracker::task_failed() {
    task_history.find(current_task)->second.push_back(0);
    task_history_data.find(current_task)->second.task_history.push_back((0));


}

}