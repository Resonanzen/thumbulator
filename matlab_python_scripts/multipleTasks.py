import argparse
import matplotlib.pyplot as plt
import numpy as np
import statistics

#parse arguments
parser = argparse.ArgumentParser(description = 'Graph task patterns');
parser.add_argument('-p', action="store", dest = "Path", help ='Path of Input Doc');
parsedArguments = parser.parse_args();
inputFilePath = parsedArguments.Path;
file = open(inputFilePath, "r");
lines = file.read().splitlines()
file.close()

task_histories = {}

#get all pcs in first line of .txt to populate index of hash
for entry in (lines[0].split()):
    if (entry.isdigit()):
        task_histories[int(entry)] = []

#populate hashtable of task and task history
for line in lines:
    if (line != lines[0] and line != lines[1]): #skip first two lines in .txt
        entries = line.split()
        task_histories[int(entries[0])].append(entries[2])	

iteration_failure_histories = [];
#first num in each element is the pc
for pc, task_history in task_histories.items():
    iteration_failure_numbers = [pc]
    num_successes = 0;
    for element in task_history:
        if element == "Success":
            num_successes = num_successes + 1
        else:
            iteration_failure_numbers.append(num_successes)				
            num_successes = 0;	
    iteration_failure_numbers.append(num_successes)				
    iteration_failure_histories.append(iteration_failure_numbers)
#print("Tasks and histories: first element in each list is the task PC")
#print(iteration_failure_histories)


def track_frequency(dictionary, key):
    if (key in dictionary):
        dictionary[key] = dictionary[key] + 1
    else:
        dictionary[key] = 1;

#make a hashtable of pc number as key and a hashtable displaying the frequency of iterations as keys
#{pc:{iterationFailureNum:numberOfTimesItsFailedAtThatNum}}
iteration_failure_frequencies = {}
for element in iteration_failure_histories:
    iteration_failure_frequencies[element[0]] = {} #index by pc

#ignore last element in iteration_failure_history since last element records num of times task ran before moving to another task
for iteration_failure_history in iteration_failure_histories:
    for iteration_failure_rate in iteration_failure_history[1:-1]:
        track_frequency(iteration_failure_frequencies[iteration_failure_history[0]],iteration_failure_rate);
#print("Iteration failure frequencies")
#print(iteration_failure_frequencies)
#print()

def pie_graph(failure_frequency_dict):
    labels = [];
    frequency = [];
    for iteration_fail_num, num_failures in failure_frequency_dict.iteritems():
        print(num_failures)
        labels.append(str(iteration_fail_num))
        frequency.append((num_failures))

    plt.pie(frequency,labels=labels, autopct = '%1.1f%%', shadow=True, startangle = 90)
    plt.show()

def bar_graph(failure_frequency_dict, pc):
    labels = [];
    frequency = [];
    
    for iteration_fail_num, num_failures in failure_frequency_dict.iteritems():
    	print(num_failures)
    	labels.append(str(iteration_fail_num))
    	frequency.append((num_failures))
    
    plt.bar(labels,frequency, align='center');
    plt.set_title("Task PC: " + str(pc), fontsize = 14)
    
    plt.set_xlabel('Iteration # where task failed',fontsize=12)

#for graph_number,(keys,frequency_list) in enumerate(iteration_failure_frequencies.iteritems()):
    #bar_graph(frequency_list,keys,axs,graph_number);

#for ax in axs.flat:
#	ax.label_outer();
#plt.tight_layout();
#plt.show()

def average(_list):
    #print (_list)
    return float(sum(_list)/float(len(_list)));

pc_list = []
average_list = []
stdev_list = []
quantity_list = []
for iteration_failure_history in iteration_failure_histories:
    if(int(iteration_failure_history[0]) and len(iteration_failure_history) > 2):
        pc_list.append(int(iteration_failure_history[0]));
        #0 is pc
        #-1 is last time task is seen, may finish before active period is over resulting in no failure
        average_list.append(average(iteration_failure_history[1:-1])); 
        stdev_list.append(statistics.stdev(iteration_failure_history[1:-1]))
        quantity_list.append(len(iteration_failure_history)-2)
x_bar = [i for i in range(len(pc_list))]
#sort the arrays by pc number
#pc_list, average_list, stdev_list, quantity_list= zip(*sorted(zip(pc_list, average_list, stdev_list, quantity_list)))
print(pc_list)
print(average_list)
print(stdev_list)
print(quantity_list)
fig, ax = plt.subplots()
for (x, avg), quantity in zip(enumerate(average_list), quantity_list):
    ax.text(x, avg, 'n = ' + str(quantity), color = 'blue', fontweight = 'bold')
plt.bar(x_bar, average_list,width = 1/1.5, color = "purple", yerr=stdev_list, capsize = 10)
plt.xticks(x_bar, pc_list)
plt.xlabel("PC of task")
plt.ylabel("Successful commits before task failure")
plt.show()


