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

task_histories = {};


#first line is list of tasks

for entry in (lines[0].split()):
	if (entry.isdigit()):
		task_histories[int(entry)] = []


#populate hashtable of task and task  history
for line in lines:

	if (line != lines[0] and line != lines[1]):
		entries = line.split()

		task_histories[int(entries[0])].append(entries[2])	







num_successes = 0;

iteration_failure_histories = [];
#first num in each element is the pc
for pc, task_history in task_histories.iteritems():
	iteration_failure_numbers = [pc]
	for element in task_history:
		if element == "Success":
			num_successes = num_successes + 1
		else:
			
			
			iteration_failure_numbers.append(num_successes)				
			num_successes = 0;	
	
	iteration_failure_histories.append(iteration_failure_numbers)



def track_frequency(dictionary, key):
	if (key in dictionary):
		dictionary[key] = dictionary[key] + 1
	else:
		dictionary[key] = 1;


#make a hashtable of pc number as key and a hashtable displaying the frequency of iterations as keys
#{pc:{iterationFailureNum:numberOfTimesItsFailedAtThatNum}}
iteration_failure_frequencies = {}
for element in iteration_failure_histories:
	iteration_failure_frequencies[element[0]] = {}

for iteration_failure_history in iteration_failure_histories:
	for iteration_failure_rate in iteration_failure_history[1:]:
		track_frequency(iteration_failure_frequencies[iteration_failure_history[0]],iteration_failure_rate);
print (iteration_failure_frequencies)




#starting making pi charts
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
print(iteration_failure_histories)

def average(_list):
	#print (_list)
	return float(sum(_list)/float(len(_list)));

pc_list = []
average_list = []
standardDeviations = []
for iteration_failure_history in iteration_failure_histories:
	if (len(iteration_failure_history[1:]) > 1):
		pc_list.append(int(iteration_failure_history[0]));
		
	
		average_list.append(average(iteration_failure_history[1:]));
		standardDeviations.append(statistics.stdev(iteration_failure_history[1:]))


#pc_list = tuple(pc_list)
print (pc_list)
print (average_list)
print(standardDeviations)
x_bar = [i for i in range(len(average_list))]
#sort the two arrays by pc number

pc_list, average_list,standardDeviations= zip(*sorted(zip(pc_list, average_list,standardDeviations)))
print(pc_list)
print(average_list)
print(standardDeviations)
plt.bar(x_bar, average_list,width = 1/1.5, color = "blue", yerr=standardDeviations)
plt.xticks(x_bar, pc_list)
plt.xlabel("PC number")
plt.ylabel("Average failure iteration number")
plt.show()


