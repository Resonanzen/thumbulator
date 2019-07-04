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


backupsPerLoop = []
index = [i for i in range(1,len(lines)-1, 1)]
for i in range(1,len(lines)-1, 1):
	backupsPerLoop.append(float(lines[i].split()[0]) - float(lines[i-1].split()[0]))
plt.bar(index,backupsPerLoop)
plt.show()
