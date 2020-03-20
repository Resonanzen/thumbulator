import argparse
import matplotlib.pyplot as plt
import numpy as np
import glob
import re
import os

#parse arguments
parser = argparse.ArgumentParser(description = 'Graph percentage of time spent by CPU making forward progress');
parser.add_argument('-p', action="store", dest = "path", help ='Path of Input Doc');
parsed_arguments = parser.parse_args();
input_file_path = parsed_arguments.path;

input_file_list = glob.glob(input_file_path + 'time_ratio*')

time_ratios = {}
files = []
for file_name in input_file_list:
    current_file = open(file_name, 'r')
    file_name = os.path.basename(os.path.normpath(file_name))
    file_name = re.sub('^time_ratio_data_','',file_name)
    file_name = re.sub('.txt$','',file_name)
    files.append(file_name)
    time_ratios[file_name] = {}
    on_time = 0;
    total_time = 0;
    for line in current_file:
        if re.search("^Total elapsed time.*$", line):
            l = line.split(':')
            total_time = int(l[1].strip())
            time_ratios[file_name]['total']=total_time
        if re.search("^.*forward progress.*$", line):
            l = line.split(':')
            time_spent = int(l[1].strip())
            time_ratios[file_name]['useful']=time_spent
            on_time += time_spent
        if re.search("^.*backup:.*$", line):
            l = line.split(':')
            time_spent = int(l[1].strip())
            time_ratios[file_name]['backup']=time_spent
            on_time += time_spent
        if re.search("^.*restore:.*$", line):
            l = line.split(':')
            time_spent = int(l[1].strip())
            time_ratios[file_name]['restore']=time_spent
            on_time += time_spent
        if re.search("^.*re-execution.*$", line):
            l = line.split(':')
            time_spent = int(l[1].strip())
            time_ratios[file_name]['dead']=time_spent
            on_time += time_spent
    time_ratios[file_name]['on']=on_time
    time_ratios[file_name]['off']=total_time-on_time

#print(time_ratios)

# put stuff to graph into labels, keys, data
labels = files
keys = ['useful', 'dead', 'backup', 'restore']
category_names = ['Forward progress', 'Re-execution', 'Backup', 'Restore']
data = []
tempdata = []
for file in files:
    for key in keys:
        tempdata.append(time_ratios[file][key]/time_ratios[file]['on'])
    data.append(tempdata)
    tempdata = []
data = np.array(data)

#print(data)

# i have no idea what the following code does
# it is a graph template i copied
data_cum = data.cumsum(axis=1)
category_colors = plt.get_cmap('RdYlGn')(np.linspace(0.15, 0.85, data.shape[1]))

fig, ax = plt.subplots(figsize=(9.2, 5))
ax.invert_yaxis()
ax.xaxis.set_visible(False)
ax.set_xlim(0, np.sum(data, axis=1).max())

for i, (colname, color) in enumerate(zip(category_names, category_colors)):
    widths = data[:, i]
    starts = data_cum[:, i] - widths
    ax.barh(labels, widths, left=starts, height=0.5,
            label=colname, color=color)
    xcenters = starts + widths / 2

    r, g, b, _ = color
    text_color = 'white' if r * g * b < 0.5 else 'darkgrey'
    for y, (x, c) in enumerate(zip(xcenters, widths)):
        ax.text(x, y, str(round(c*100,2))+'%', ha='center', va='center',
                color=text_color)
ax.legend(ncol=len(category_names), bbox_to_anchor=(0, 1),
          loc='lower left', fontsize='small')

plt.show()
