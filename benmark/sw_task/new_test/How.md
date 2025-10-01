
# Đo task_runner
python3 measure_psutil.py -- ./task_runner 10 2

# Đo với custom output và duration
python3 measure_psutil.py -o results.json -d 60 -- ./task_runner 5 8

# Đo chương trình không có tham số
python3 measure_psutil.py -- ./simple_program

# ve bieu do
python3 draw.py results.json

