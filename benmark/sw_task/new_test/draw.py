#!/usr/bin/env python3
# plot_single_performance.py - Visualize performance metrics from a single JSON file
import json
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys
import os
from datetime import datetime

class SinglePerformanceVisualizer:
    def __init__(self):
        self.data = None
        self.metrics = None
        self.basename = None

    def load_data(self, json_file):
        try:
            with open(json_file, 'r') as f:
                self.data = json.load(f)
            self.metrics = self.data.get('metrics', [])
            self.basename = os.path.splitext(os.path.basename(json_file))[0]
            print(f"Loaded {len(self.metrics)} samples from {json_file}")
            return True
        except Exception as e:
            print(f"Error loading data: {e}")
            return False

    def plot_cpu(self):
        if not self.metrics:
            print("No metrics data for CPU plot.")
            return
        times = [(m['timestamp'] - self.metrics[0]['timestamp']) for m in self.metrics]
        cpu = [m['process_cpu'] for m in self.metrics]
        plt.figure(figsize=(10, 6))
        plt.plot(times, cpu, linewidth=2, color='#2E86AB')
        plt.title('CPU Usage Over Time', fontweight='bold')
        plt.xlabel('Time (seconds)')
        plt.ylabel('CPU Usage (%)')
        plt.grid(True, alpha=0.3)
        filename = f"{self.basename}_cpu_usage.png"
        plt.savefig(filename, dpi=200, bbox_inches='tight')
        plt.close()
        print(f"Saved CPU usage plot: {filename}")

    def plot_memory(self):
        if not self.metrics:
            print("No metrics data for Memory plot.")
            return
        times = [(m['timestamp'] - self.metrics[0]['timestamp']) for m in self.metrics]
        memory = [m['process_memory_mb'] for m in self.metrics]
        plt.figure(figsize=(10, 6))
        plt.plot(times, memory, linewidth=2, color='#F18F01')
        plt.title('Memory Usage Over Time', fontweight='bold')
        plt.xlabel('Time (seconds)')
        plt.ylabel('Memory (MB)')
        plt.grid(True, alpha=0.3)
        filename = f"{self.basename}_memory_usage.png"
        plt.savefig(filename, dpi=200, bbox_inches='tight')
        plt.close()
        print(f"Saved Memory usage plot: {filename}")

    def plot_threads(self):
        if not self.metrics:
            print("No metrics data for Thread plot.")
            return
        times = [(m['timestamp'] - self.metrics[0]['timestamp']) for m in self.metrics]
        threads = [m['process_threads'] for m in self.metrics]
        plt.figure(figsize=(10, 6))
        plt.plot(times, threads, linewidth=2, color='#4ECDC4', marker='o', markersize=3)
        plt.title('Thread Count Over Time', fontweight='bold')
        plt.xlabel('Time (seconds)')
        plt.ylabel('Number of Threads')
        plt.grid(True, alpha=0.3)
        filename = f"{self.basename}_thread_count.png"
        plt.savefig(filename, dpi=200, bbox_inches='tight')
        plt.close()
        print(f"Saved Thread count plot: {filename}")

    def plot_cores(self):
        if not self.metrics:
            print("No metrics data for Core plot.")
            return
        times = [(m['timestamp'] - self.metrics[0]['timestamp']) for m in self.metrics]
        cores = [m['cores_active'] for m in self.metrics]
        plt.figure(figsize=(10, 6))
        plt.plot(times, cores, linewidth=2, color='#9B59B6')
        plt.title('Active CPU Cores Over Time', fontweight='bold')
        plt.xlabel('Time (seconds)')
        plt.ylabel('Number of Active Cores')
        plt.grid(True, alpha=0.3)
        filename = f"{self.basename}_active_cores.png"
        plt.savefig(filename, dpi=200, bbox_inches='tight')
        plt.close()
        print(f"Saved Active cores plot: {filename}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 plot_single_performance.py <results.json>")
        sys.exit(1)
    json_file = sys.argv[1]
    if not os.path.exists(json_file):
        print(f"Error: Results file not found: {json_file}")
        sys.exit(1)
    viz = SinglePerformanceVisualizer()
    if not viz.load_data(json_file):
        sys.exit(1)
    viz.plot_cpu()
    viz.plot_memory()
    viz.plot_threads()
    viz.plot_cores()

if __name__ == "__main__":
    main()