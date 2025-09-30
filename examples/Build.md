# 1. Production (không debug)
./build_debug.sh production

# 2. Timer debug only
./build_debug.sh timer

# 3. SLLooper debug only  
./build_debug.sh looper

# 4. EventQueue debug only
./build_debug.sh queue

# 5. All debug
./build_debug.sh all

# 6. Combo (Timer + SLLooper)
./build_debug.sh combo


# manual build

# Production
rm -rf build && mkdir build && cd build
cmake ..
make -j$(nproc)

# Timer debug
rm -rf build && mkdir build && cd build  
cmake .. -DENABLE_TIMER_DEBUG=ON
make -j$(nproc)

# All debug
rm -rf build && mkdir build && cd build
cmake .. -DENABLE_ALL_DEBUG=ON
make -j$(nproc)

# Multiple components
rm -rf build && mkdir build && cd build
cmake .. -DENABLE_TIMER_DEBUG=ON -DENABLE_SLLOOPER_DEBUG=ON
make -j$(nproc)