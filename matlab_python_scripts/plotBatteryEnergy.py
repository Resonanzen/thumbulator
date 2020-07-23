# for a given benchmark, plot energy in battery/capacitor over time 
# Xs mark commits upon task completion
# args: -p time_energy_data*

import argparse
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import LineCollection
from matplotlib.colors import ListedColormap, BoundaryNorm

#parse arguments
parser = argparse.ArgumentParser(description = 'Graph task patterns');

parser.add_argument('-p', action="store", dest = "Path", help ='Path to input file (time_energy_data_*.txt)');
parser.add_argument('--color', action="store_const", dest = "Color", const=True, default=False, help ='Annotate graph with charging, computing, backups, and wasted cycles (default: no annotations)');


parsedArguments = parser.parse_args();
inputFilePath = parsedArguments.Path;
annotate = parsedArguments.Color;

file = open(inputFilePath, "r");
lines = file.read().splitlines()
file.close()

energy_in_battery = []
x_axis = []
backup_x = []
backup_y = []
traceY = []

d = []
shutoff_x = []
shutoff_y = []

is_charging = True
der_last = 0
num_backups = 0
count = 0

for i in range(len(lines)):
    if (lines[i].startswith("Time")):
        x_axis.append(float(lines[i].split()[1]))
        energy_in_battery.append(float(lines[i].split()[2]))
    if (lines[i].startswith("Backup")):
        backup_x.append(float(lines[i].split()[1]));
        backup_y.append(float(lines[i].split()[2]));

if(annotate):
    for n in range(1, len(x_axis)-1):
        der = (energy_in_battery[n]-energy_in_battery[n-1])
        d.append(der)
        if der_last < 0 and der > 0:
            shutoff_x.append(x_axis[n-1])
            shutoff_y.append(energy_in_battery[n-1])
        der_last = der
    d.append(d[-1])
    
    x = np.array(x_axis)
    y = np.array(energy_in_battery)
    dydx = np.array(d)
    points = np.array([x, y]).T.reshape(-1, 1, 2)
    segments = np.concatenate([points[:-1], points[1:]], axis=1)
    fig, axs = plt.subplots()
    
    # Use a boundary norm instead
    cmap = ListedColormap(['xkcd:chocolate', 'sandybrown'])
    norm = BoundaryNorm([-1, 0, 1], cmap.N)
    lc = LineCollection(segments, cmap=cmap, norm=norm)
    lc.set_array(dydx)
    lc.set_linewidth(2)
    line = axs.add_collection(lc)
    
    axs.set_xlim(x.min(), x.max()+3)
    axs.set_ylim(-1.1, 5.1)

else:
    plt.plot(x_axis,energy_in_battery, color='xkcd:chocolate')

plt.scatter(shutoff_x, shutoff_y, marker='x', color='red', zorder=2)
plt.scatter(backup_x,backup_y, marker='o', color = 'yellowgreen', zorder=3)
plt.xlabel('Time (s)')
plt.ylabel('Voltage (V)');

backup_x_np = np.array(backup_x)
for n in range(0,len(shutoff_x)):
    last_backup_idx = np.searchsorted(backup_x_np, shutoff_x[n], side="left") - 1
    if(last_backup_idx < len(backup_x) and last_backup_idx >= 0):
        plt.plot([backup_x[last_backup_idx],shutoff_x[n]], [backup_y[last_backup_idx],shutoff_y[n]], color = 'red', zorder=2)

print("Benchmark start: ", backup_x[0])
print("Benchmark end: ", x_axis[-1])
print("Total elapsed time: ", x_axis[-1] - backup_x[0])

plt.show()
    
