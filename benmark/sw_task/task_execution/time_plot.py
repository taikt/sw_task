import pandas as pd
import matplotlib.pyplot as plt

# Đọc CSV
df = pd.read_csv('tiger_execution_times.csv')

# Vẽ biểu đồ execution time
plt.figure(figsize=(12, 8))
plt.subplot(1, 1, 1) 
# plt.subplot(2, 2, 1)
plt.bar(df['Task_ID'], df['Execution_Time_ms'])
plt.title('Execution Time per Task')
plt.xlabel('Task ID')
plt.ylabel('Time (ms)')

# plt.subplot(2, 2, 2)
# plt.bar(df['Task_ID'], df['Queue_Latency_ms'])
# plt.title('Queue Latency per Task')
# plt.xlabel('Task ID')
# plt.ylabel('Time (ms)')

# plt.subplot(2, 2, 3)
# plt.bar(df['Task_ID'], df['Total_Response_ms'])
# plt.title('Total Response Time per Task')
# plt.xlabel('Task ID')
# plt.ylabel('Time (ms)')

# plt.subplot(2, 2, 4)
# plt.hist(df['Execution_Time_ms'], bins=5, alpha=0.7)
# plt.title('Execution Time Distribution')
# plt.xlabel('Time (ms)')
# plt.ylabel('Frequency')

plt.tight_layout()
plt.savefig('execution_analysis.png', dpi=300)
plt.show()