## cd to build folder
# Test 15 light tasks
./response_time light 15

# Test 8 heavy tasks  
./response_time heavy 8

# Test 10 light + 5 heavy mixed
./response_time mixed 10 5

# Default values
./response_time light    # 10 light tasks
./response_time heavy    # 5 heavy tasks  
./response_time mixed    # 5 light + 3 heavy


python3.13 time_plot.py

./../build/response_time mixed 30 20

python3.13 graph_draw.py execution_times.csv


./../../../../tiger_looper/benmark/build/tiger_response_time mixed 30 20

python3.13 graph_draw.py tiger_execution_times.csv


