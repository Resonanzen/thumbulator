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

energy_in_battery = []
x_axis = []




num_backups = 0;
count = 0;
for i in range(len(lines)):
	

	if (lines[i].startswith("Time:")):
		x_axis.append(float(lines[i].split()[1]));
		energy_in_battery.append(float(lines[i].split()[2]));



#plot line graph
print("DONE!")
plt.plot(x_axis,energy_in_battery,'.')
plt.xlabel('Time')
plt.ylabel('Energy')
plt.show()

