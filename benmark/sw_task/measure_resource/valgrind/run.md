valgrind --tool=massif --stacks=yes ./../../../../examples/build/sw
valgrind --tool=massif --stacks=yes ./../../../tiger_looper/build/tiger
python3 monitor.py tiger_massif.out.7217 sw_massif.out.7234


# CPU profiling
perf record -g ./../../../../examples/build/sw
perf record -g ./../../../tiger_looper/build/tiger
perf report

# Memory allocation profiling  
valgrind --tool=massif --detailed-freq=1 ./../../../../examples/build/sw
valgrind --tool=massif --detailed-freq=1 ./../../../tiger_looper/build/tiger

# Thread context switch
perf stat -e context-switches ./../../../../examples/build/sw
perf stat -e context-switches ./../../../tiger_looper/build/tiger