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

tasks_per_period = [ ]

active_period_starts = []

num_backups = 0;
for line in lines:
    if (line.startswith("BACKUP!")):
        num_backups = num_backups + 1
    if (line.startswith("Turning off")):
        tasks_per_period.append(num_backups)
        num_backups = 0
    if (line.startswith("Time is :")):
	print(str(line))

index = [ num for num in np.arange(1,len(tasks_per_period) + 1)]        


print (str(len(index)) + " ")
print(len(tasks_per_period))

#plot bar graph


plt.bar(index, tasks_per_period)
plt.xlabel('Active Period')
plt.ylabel('# of tasks per active period')
plt.show()

