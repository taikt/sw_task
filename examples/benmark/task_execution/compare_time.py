import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read data from two CSV files
sw_df = pd.read_csv('execution_times.csv')
tiger_df = pd.read_csv('tiger_execution_times.csv')

# Ensure the same number of tasks (if not, take the minimum)
num_tasks = min(len(sw_df), len(tiger_df))
sw_df = sw_df.iloc[:num_tasks]
tiger_df = tiger_df.iloc[:num_tasks]

# Create positions for the bars
bar_width = 0.35
index = np.arange(num_tasks)

plt.figure(figsize=(16,8))

# Plot SW Task
plt.bar(index - bar_width/2, sw_df['Execution_Time_ms'], bar_width, 
        label='SW Task', color='skyblue', edgecolor='black', alpha=0.8)

# Plot Tiger Looper
plt.bar(index + bar_width/2, tiger_df['Execution_Time_ms'], bar_width, 
        label='Tiger Looper', color='orange', edgecolor='black', alpha=0.8)

# Annotate values on bars
for i in range(num_tasks):
    plt.text(index[i] - bar_width/2, sw_df['Execution_Time_ms'].iloc[i] + 10, 
             f"{sw_df['Execution_Time_ms'].iloc[i]:.1f}", ha='center', fontsize=8, color='blue')
    plt.text(index[i] + bar_width/2, tiger_df['Execution_Time_ms'].iloc[i] + 10, 
             f"{tiger_df['Execution_Time_ms'].iloc[i]:.1f}", ha='center', fontsize=8, color='darkorange')

plt.xlabel('Task ID')
plt.ylabel('Execution Time (ms)')
plt.title('Execution Time Comparison: SW Task vs Tiger Looper')
plt.xticks(index, sw_df['Task_ID'])
plt.legend()
plt.tight_layout()
plt.grid(axis='y', linestyle='--', alpha=0.5)

# Save PNG image
plt.savefig('compare_execution_time.png', dpi=150)
print("Image saved: compare_execution_time.png")

plt.show()