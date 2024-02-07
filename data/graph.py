import matplotlib.pyplot as plt
import numpy as np

# Data for performance time graphs
operations = ['Insert', 'Lookup', 'Remove']
scales = ['1E7', '5E7', 'File']

# std::map and EYT performance times (seconds) for best, worst, and avg scenarios
std_map_times = {
    '1E7': {
        'Insert': [14.68, 15.28, 14.79],
        'Lookup': [14.61, 14.72, 14.72],
        'Remove': [15.06, 15.38, 15.24]
    },
    '5E7': {
        'Insert': [100.83, 103.01, 101.27],
        'Lookup': [103.82, 104.11, 103.49],
        'Remove': [109.28, 112.85, 110.09]
    },
    'File': {
        'Insert': [10.21, 11.44, 10.47],
        'Lookup': [10.45, 11.48, 11.09],
        'Remove': [11.51, 11.57, 12.33]
    }
}

eyt_times = {
    '1E7': {
        'Insert': [3.81, 3.94, 3.90],
        'Lookup': [3.04, 3.12, 3.07],
        'Remove': [3.61, 3.73, 3.60]
    },
    '5E7': {
        'Insert': [24.59, 24.84, 25.20],
        'Lookup': [17.35, 17.82, 17.59],
        'Remove': [19.94, 20.62, 20.33]
    },
    'File': {
        'Insert': [5.64, 5.72, 5.74],
        'Lookup': [3.86, 4.08, 3.93],
        'Remove': [4.16, 4.22, 4.27]
    }
}
# Correcting the issue and re-importing matplotlib for plotting
import matplotlib.pyplot as plt

# Plotting
fig, axs = plt.subplots(3, 3, figsize=(18, 12), sharey='row')

for i, scale in enumerate(scales):
    for j, operation in enumerate(operations):
        axs[i, j].bar('std::map Best', std_map_times[scale][operation][0], color='blue')
        axs[i, j].bar('std::map Worst', std_map_times[scale][operation][1], color='lightblue')
        axs[i, j].bar('std::map Avg', std_map_times[scale][operation][2], color='blue', alpha=0.7)
        axs[i, j].bar('EYT Best', eyt_times[scale][operation][0], color='green')
        axs[i, j].bar('EYT Worst', eyt_times[scale][operation][1], color='lightgreen')
        axs[i, j].bar('EYT Avg', eyt_times[scale][operation][2], color='lime', alpha=0.7)
        axs[i, j].set_title(f'{operation} Time ({scale})')
        axs[i, j].set_ylabel('Time (sec)')
        axs[i, j].tick_params(labelrotation=45)

plt.tight_layout()
plt.show()


# Create figures and axes for the plots
fig, axs = plt.subplots(len(areas), len(metrics), figsize=(20, 15))

for i, area in enumerate(areas):
    if area == '1.00E+07':
        normal_data = normal_1e7
        eyt_data = eyt_1e7
    elif area == '5.00E+07':
        normal_data = normal_5e7
        eyt_data = eyt_5e7
    else:  # File
        normal_data = normal_file
        eyt_data = eyt_file
    
    for j, metric in enumerate(metrics):
        # Indexing data
        normal_values = normal_data[metric]
        eyt_values = eyt_data[metric]

        # X locations for the groups
        ind = np.arange(len(normal_values))  
        width = 0.35  # the width of the bars

        # Plotting
        axs[i, j].bar(ind - width/2, normal_values, width, label='Normal')
        axs[i, j].bar(ind + width/2, eyt_values, width, label='EYT')

        # Labels and titles
        axs[i, j].set_title(f'{metric} ({area})')
        axs[i, j].set_xticks(ind)
        axs[i, j].set_xticklabels(['Best', 'Worst', 'Avg'])
        axs[i, j].set_ylabel('Time (sec)')
        axs[i, j].legend()

# Layout adjustment
# plt.tight_layout()


# Data for graphing
areas = ['1.00E+07', '5.00E+07', 'File']
metrics = ['insert', 'scan', 'lookup', 'remove']

# Define your data for normal and EYT here...

# Loop through each area and metric to create and save individual graphs
for i, area in enumerate(areas):
    if area == '1.00E+07':
        normal_data = normal_1e7
        eyt_data = eyt_1e7
    elif area == '5.00E+07':
        normal_data = normal_5e7
        eyt_data = eyt_5e7
    else:  # File
        normal_data = normal_file
        eyt_data = eyt_file
    
    for j, metric in enumerate(metrics):
        # Set up figure
        fig, ax = plt.subplots(figsize=(6, 4))

        # Indexing data
        normal_values = normal_data[metric]
        eyt_values = eyt_data[metric]

        # X locations for the groups
        ind = np.arange(len(normal_values))
        width = 0.35  # the width of the bars

        # Plotting
        ax.bar(ind - width/2, normal_values, width, label='Normal')
        ax.bar(ind + width/2, eyt_values, width, label='EYT')

        # Labels and titles
        ax.set_title(f'{metric} ({area})')
        ax.set_xticks(ind)
        ax.set_xticklabels(['Best', 'Worst', 'Avg'])
        ax.set_ylabel('Time (sec)')
        ax.legend()

        # Save the figure
        fig.savefig(f'{area}_{metric}.png', dpi=300)  # High-quality image

        # Close the plot to save memory
        plt.close(fig)


plt.show()
