# run script
python3 ../monitor.py -o sw_result.json -d 60 -- ./../../../../../examples/build/sw
python3 ../monitor.py -o tiger_result.json -d 60 -- ./../../../../tiger_looper/build/tiger

python3 plot.py sw_result.json tiger_result.json



