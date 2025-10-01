# Hoặc chạy manual từng test
python3 -m venv venv
source venv/bin/activate
pip install pandas seaborn matplotlib psutil

python3.13 cpu_bound_task/monitor_cpu_test.py build/cpu_task 20 cpu_bound_task/cpu_task.json

python3.13 cpu_bound_task/monitor_cpu_test.py ../../../tiger_looper/benmark/build/tiger_cpu_task 20 cpu_bound_task/tiger_cpu_task.json

# Generate comparison plots
python3.13 plot_performance.py cpu_task.json tiger_cpu_task.json