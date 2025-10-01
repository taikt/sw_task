#!/usr/bin/env python3
import re
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend cho server

def parse_massif_file(filename):
    timestamps = []
    memory_usage = []
    with open(filename, 'r') as f:
        lines = f.readlines()
    for i, line in enumerate(lines):
        if line.startswith('snapshot='):
            time_ms = 0
            mem_bytes = 0
            for j in range(i+1, min(i+10, len(lines))):
                if lines[j].startswith('time='):
                    time_ms = int(lines[j].split('=')[1])
                elif lines[j].startswith('mem_heap_B='):
                    mem_bytes = int(lines[j].split('=')[1])
                    break
            if time_ms > 0 and mem_bytes >= 0:
                timestamps.append(time_ms / 1000.0)
                memory_usage.append(mem_bytes / (1024*1024))
    return timestamps, memory_usage

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3 or len(sys.argv) % 2 != 1:
        print("Usage: python3 plot_massif.py <massif_file1> <label1> [<massif_file2> <label2> ...]")
        sys.exit(1)

    plt.figure(figsize=(12, 8))
    colors = ['b', 'r', 'g', 'c', 'm', 'y', 'k']
    for i in range(1, len(sys.argv), 2):
        massif_file = sys.argv[i]
        label = sys.argv[i+1]
        timestamps, memory_usage = parse_massif_file(massif_file)
        plt.plot(timestamps, memory_usage, color=colors[(i//2)%len(colors)], linewidth=2, marker='o', markersize=4, label=label)
        # plt.fill_between(timestamps, memory_usage, alpha=0.2, color=colors[(i//2)%len(colors)])

    plt.xlabel('Time (seconds)', fontsize=12)
    plt.ylabel('Memory Usage (MB)', fontsize=12)
    plt.title('Memory Usage Profile (Valgrind Massif)', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig('massif_compare.png', dpi=300, bbox_inches='tight')
    print("✅ Graph saved: massif_compare.png")