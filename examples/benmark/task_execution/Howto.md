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

# light task
Operations: 200K math, 15K array sort, 60×60 matrix, 5K primes

Mathematical Operations (200K iterations)

Trigonometric: sin(), cos(), sqrt(), atan(), tanh()
Power/Log: pow(), log()
Mixed floating-point calculations
Array Operations (15K elements)

Vector initialization với complex math
Multiple sorting: sort(), reverse(), sort(greater<>)
Statistical: accumulate() for average
String Operations (3K iterations)

String concatenation với to_string()
Cubic calculations: i * i * i
Periodic string reversal
Matrix Operations (60x60 matrix)

Matrix initialization với sin(), cos()
Element-wise multiplication
Accumulation results
Prime Calculation (up to 5K numbers)

Prime detection algorithm O(√n)
Mathematical enhancement với sqrt()


# heavy task
Operations: 800K math, 150×150 matrix multiply (O(n³)), 40K array, 16K primes, gamma functions

Massive Mathematical Operations (800K iterations)

Complex trigonometric combinations
Exponential operations: exp(), log()
Inverse trigonometric: asin(), acos()
High-frequency calculations
Large Matrix Operations (150x150 matrix)

Matrix Multiplication O(n³) - Most expensive operation
3 matrices: A, B, C where C = A × B
3,375,000 multiplications (150³)
Complex initialization với nested math functions
Large Array Operations (40K elements)

Complex element calculation with 5+ math functions
5 consecutive expensive sorts:
sort() → reverse() → sort() → reverse() → sort(greater<>)
Memory-intensive accumulation
Complex String Operations (8K iterations)

Heavy string concatenation
Triple calculations per iteration
Periodic string operations: reverse, sort, substring
Memory management để control size
Enhanced Prime Calculation (up to 16K numbers)

Extended range prime detection
Enhanced result với sqrt() * log()
Additional CPU-Intensive Operations (800K iterations)

Gamma function: std::tgamma() - mathematically expensive
Factorial calculation: manual implementation
High-frequency mathematical operations
