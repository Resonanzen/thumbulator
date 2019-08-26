import argparse
import matplotlib.pyplot as plt
import numpy as np

#parse arguments
parser = argparse.ArgumentParser(description = 'Graph task patterns');

parser.add_argument('-p', action="store", dest = "Path", help ='Path of Input Doc');

parsedArguments = parser.parse_args();

inputFilePath = parsedArguments.Path;

file = open(inputFilePath, "r");
lines = file.read().splitlines()
file.close()

task_histories = {}

for entry in (lines[0].split()):
	if (entry.isdigit()):
		task_histories[int(entry)] = []


#populate hashtable of task and task history
for line in lines:

	if (line != lines[0] and line != lines[1]):
		entries = line.split()

		task_histories[int(entries[0])].append(entries[2])	


iteration_failure_histories = [];
#first num in each element is the pc
for pc, task_history in task_histories.iteritems():
	iteration_failure_numbers = [pc]
	num_successes = 0;
	for element in task_history:
		if element == "Success":
			num_successes = num_successes + 1
		else:
			
			
			iteration_failure_numbers.append(num_successes)				
			num_successes = 0;	
	
	iteration_failure_histories.append(iteration_failure_numbers)



def plot_bar(iteration_failure_history):
	x_bar = [i for i in range(len(iteration_failure_history[1:]))]
	if (len(iteration_failure_history) > 1):
		plt.bar(x_bar, iteration_failure_history[1:])
		plt.xlabel('active period')
		plt.ylabel('iteration failure number')
		plt.title('PC: '+ str(iteration_failure_history[0]))
		plt.show()
print("Tasks and histories: first element in each list is the task PC")
print(iteration_failure_histories)

for i in range(len(iteration_failure_histories)):
	plot_bar(iteration_failure_histories[i])

