import argparse
import matplotlib.pyplot as plt
import numpy as np

#parse arguments
parser = argparse.ArgumentParser(description = 'Graph task patterns');

parser.add_argument('-eh_sim', action="store", dest = "eh_sim", help ='eh-sim results');
parser.add_argument('-simulink', action="store", dest = "simulink", help ='simulink_results');

parsedArguments = parser.parse_args();

simulink_path = parsedArguments.simulink;
eh_sim_path = parsedArguments.eh_sim;


file = open(simulink_path, "r");
simulink_lines= file.read().splitlines()
file.close()

file = open(eh_sim_path,"r");
eh_sim_lines = file.read().splitlines();
file.close()



simulink_x = []
simulink_y = []

eh_sim_x = [];
eh_sim_y = []

total_lines = min(len(simulink_lines), len(eh_sim_lines));
print(len(simulink_lines))
print(len(eh_sim_lines))
difference_percentageS = []
ABSOLUTE_DIFFERENCES = []
for i in range(total_lines):

	simulink_voltage = float(simulink_lines[i].split()[1])
	eh_sim_voltage = float(eh_sim_lines[i].split()[2])



	eh_sim_x.append(float(eh_sim_lines[i].split()[1]));
	eh_sim_y.append(float(eh_sim_lines[i].split()[2]));


	simulink_x.append(float(simulink_lines[i].split()[0]));
	simulink_y.append(float(simulink_lines[i].split()[1]));
	


	if (simulink_voltage != 0):

		absolute_difference = abs(eh_sim_voltage - simulink_voltage);
		difference_percentage = (abs(eh_sim_voltage - simulink_voltage)/simulink_voltage)
		ABSOLUTE_DIFFERENCES.append(absolute_difference)
		
		difference_percentageS.append(difference_percentage);
		
print("Average percentage difference is : " + str((sum(difference_percentageS)/len(difference_percentageS)) * 100) + "%")
print("Average voltage difference is : " + str(sum(ABSOLUTE_DIFFERENCES)/len(ABSOLUTE_DIFFERENCES))+ "V")	

plt.plot(simulink_x, simulink_y, 'r');
plt.plot(eh_sim_x, eh_sim_y, 'b');
#plt.legend()
plt.show();
