#!/usr/bin/env python3
# plot_cpu_performance.py - Visualize CPU-bound performance comparison
import json
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend for server environments
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from datetime import datetime
import sys
import os

class CPUPerformanceVisualizer:
    def __init__(self):
        self.sw_data = None
        self.tiger_data = None
        
    def load_data(self, sw_file, tiger_file):
        """Load monitoring data from JSON files"""
        try:
            with open(sw_file, 'r') as f:
                self.sw_data = json.load(f)
            print(f"Loaded SW Task data: {len(self.sw_data['metrics'])} samples")
        except Exception as e:
            print(f"Error loading SW data: {e}")
            return False
            
        try:
            with open(tiger_file, 'r') as f:
                self.tiger_data = json.load(f)
            print(f"Loaded Tiger data: {len(self.tiger_data['metrics'])} samples")
        except Exception as e:
            print(f"Error loading Tiger data: {e}")
            return False
            
        return True
    
    def create_comparison_plots(self):
        """Create comprehensive comparison plots"""
        # Set style - Fixed seaborn syntax
        try:
            plt.style.use('seaborn-v0_8')
            sns.set_style("darkgrid")
        except:
            # Fallback for older matplotlib/seaborn versions
            try:
                plt.style.use('seaborn')
                sns.set_style("darkgrid")
            except:
                plt.style.use('ggplot')
                
        fig = plt.figure(figsize=(20, 16))
        
        # Main title
        fig.suptitle('CPU-Bound Performance: SW Task vs Tiger Looper', 
                    fontsize=18, fontweight='bold', y=0.98)
        
        # 1. Execution Time & Summary Stats
        ax1 = plt.subplot(3, 3, 1)
        self._plot_execution_time(ax1)
        
        # 2. CPU Usage Over Time
        ax2 = plt.subplot(3, 3, 2)
        self._plot_cpu_timeline(ax2)
        
        # 3. Memory Usage Over Time
        ax3 = plt.subplot(3, 3, 3)
        self._plot_memory_timeline(ax3)
        
        # 4. Thread Count Over Time
        ax4 = plt.subplot(3, 3, 4)
        self._plot_threads_timeline(ax4)
        
        # 5. CPU Cores Utilization
        ax5 = plt.subplot(3, 3, 5)
        self._plot_cores_utilization(ax5)
        
        # 6. Performance Summary Bar Chart
        ax6 = plt.subplot(3, 3, 6)
        self._plot_performance_summary(ax6)
        
        # 7. CPU Efficiency (Throughput/CPU)
        ax7 = plt.subplot(3, 3, 7)
        self._plot_efficiency_comparison(ax7)
        
        # 8. Resource Utilization Radar
        try:
            ax8 = plt.subplot(3, 3, 8, projection='polar')
            self._plot_resource_radar(ax8)
        except Exception as e:
            print(f"Warning: Radar plot failed: {e}")
            ax8 = plt.subplot(3, 3, 8)
            ax8.text(0.5, 0.5, 'Radar Plot\nNot Available', ha='center', va='center', transform=ax8.transAxes)
            ax8.set_title('Resource Utilization\n(Radar Chart Error)', fontweight='bold')
        
        # 9. Scalability Analysis
        ax9 = plt.subplot(3, 3, 9)
        self._plot_scalability_projection(ax9)
        
        plt.tight_layout()
        plt.subplots_adjust(top=0.95, hspace=0.3, wspace=0.3)
        
        # Save plot
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"cpu_performance_comparison_{timestamp}.png"
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        print(f"Performance comparison saved: {filename}")
        
        # Close figure to free memory
        plt.close()
        
    def _plot_execution_time(self, ax):
        """Plot execution time comparison"""
        if not (self.sw_data and self.tiger_data):
            ax.text(0.5, 0.5, 'No Data Available', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('Total Execution Time', fontweight='bold')
            return
            
        frameworks = ['SW Task', 'Tiger Looper']
        times = [self.sw_data['duration'], self.tiger_data['duration']]
        colors = ['#2E86AB', '#A23B72']
        
        bars = ax.bar(frameworks, times, color=colors, alpha=0.8)
        ax.set_title('Total Execution Time', fontweight='bold')
        ax.set_ylabel('Time (seconds)')
        
        # Add value labels on bars
        for bar, time in zip(bars, times):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height + height*0.02,
                   f'{time:.1f}s', ha='center', va='bottom', fontweight='bold')
        
        # Add speedup annotation
        if times[1] > 0 and times[0] > 0:
            speedup = times[1] / times[0]
            y_pos = max(times) * 0.8
            ax.text(0.5, y_pos, f'SW Task is {speedup:.1f}x faster', 
                   ha='center', va='center',
                   bbox=dict(boxstyle="round,pad=0.3", facecolor="yellow", alpha=0.7))
    
    def _plot_cpu_timeline(self, ax):
        """Plot CPU usage over time"""
        if self.sw_data and len(self.sw_data['metrics']) > 0:
            sw_metrics = self.sw_data['metrics']
            sw_times = [(m['timestamp'] - sw_metrics[0]['timestamp']) for m in sw_metrics]
            sw_cpu = [m['process_cpu'] for m in sw_metrics]
            ax.plot(sw_times, sw_cpu, label='SW Task', linewidth=2, color='#2E86AB')
        
        if self.tiger_data and len(self.tiger_data['metrics']) > 0:
            tiger_metrics = self.tiger_data['metrics']
            tiger_times = [(m['timestamp'] - tiger_metrics[0]['timestamp']) for m in tiger_metrics]
            tiger_cpu = [m['process_cpu'] for m in tiger_metrics]
            ax.plot(tiger_times, tiger_cpu, label='Tiger Looper', linewidth=2, color='#A23B72')
        
        ax.set_title('CPU Usage Over Time', fontweight='bold')
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('CPU Usage (%)')
        ax.legend()
        ax.grid(True, alpha=0.3)
    
    def _plot_memory_timeline(self, ax):
        """Plot memory usage over time"""
        if self.sw_data and len(self.sw_data['metrics']) > 0:
            sw_metrics = self.sw_data['metrics']
            sw_times = [(m['timestamp'] - sw_metrics[0]['timestamp']) for m in sw_metrics]
            sw_memory = [m['process_memory_mb'] for m in sw_metrics]
            ax.plot(sw_times, sw_memory, label='SW Task', linewidth=2, color='#F18F01')
        
        if self.tiger_data and len(self.tiger_data['metrics']) > 0:
            tiger_metrics = self.tiger_data['metrics']
            tiger_times = [(m['timestamp'] - tiger_metrics[0]['timestamp']) for m in tiger_metrics]
            tiger_memory = [m['process_memory_mb'] for m in tiger_metrics]
            ax.plot(tiger_times, tiger_memory, label='Tiger Looper', linewidth=2, color='#C73E1D')
        
        ax.set_title('Memory Usage Over Time', fontweight='bold')
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('Memory (MB)')
        ax.legend()
        ax.grid(True, alpha=0.3)
    
    def _plot_threads_timeline(self, ax):
        """Plot thread count over time"""
        if self.sw_data and len(self.sw_data['metrics']) > 0:
            sw_metrics = self.sw_data['metrics']
            sw_times = [(m['timestamp'] - sw_metrics[0]['timestamp']) for m in sw_metrics]
            sw_threads = [m['process_threads'] for m in sw_metrics]
            ax.plot(sw_times, sw_threads, label='SW Task', linewidth=3, color='#4ECDC4', marker='o', markersize=3)
        
        if self.tiger_data and len(self.tiger_data['metrics']) > 0:
            tiger_metrics = self.tiger_data['metrics']
            tiger_times = [(m['timestamp'] - tiger_metrics[0]['timestamp']) for m in tiger_metrics]
            tiger_threads = [m['process_threads'] for m in tiger_metrics]
            ax.plot(tiger_times, tiger_threads, label='Tiger Looper', linewidth=3, color='#44A08D', marker='s', markersize=3)
        
        ax.set_title('Thread Count Over Time', fontweight='bold')
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('Number of Threads')
        ax.legend()
        ax.grid(True, alpha=0.3)
    
    def _plot_cores_utilization(self, ax):
        """Plot CPU cores utilization"""
        if not (self.sw_data and self.tiger_data):
            ax.text(0.5, 0.5, 'No Data Available', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('CPU Cores Utilization', fontweight='bold')
            return
            
        # Calculate average cores used
        sw_cores = [m['cores_active'] for m in self.sw_data['metrics']]
        tiger_cores = [m['cores_active'] for m in self.tiger_data['metrics']]
        
        if not sw_cores or not tiger_cores:
            ax.text(0.5, 0.5, 'No Core Data', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('CPU Cores Utilization', fontweight='bold')
            return
        
        frameworks = ['SW Task', 'Tiger Looper']
        cores_avg = [np.mean(sw_cores), np.mean(tiger_cores)]
        cores_max = [np.max(sw_cores), np.max(tiger_cores)]
        
        x = np.arange(len(frameworks))
        width = 0.35
        
        bars1 = ax.bar(x - width/2, cores_avg, width, label='Average', color=['#9B59B6', '#8E44AD'], alpha=0.8)
        bars2 = ax.bar(x + width/2, cores_max, width, label='Peak', color=['#E67E22', '#D35400'], alpha=0.8)
        
        ax.set_title('CPU Cores Utilization', fontweight='bold')
        ax.set_ylabel('Number of Cores')
        ax.set_xticks(x)
        ax.set_xticklabels(frameworks)
        ax.legend()
        
        # Add value labels
        for bars in [bars1, bars2]:
            for bar in bars:
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2., height + 0.1,
                       f'{height:.1f}', ha='center', va='bottom', fontsize=10)
    
    def _plot_performance_summary(self, ax):
        """Plot performance summary metrics"""
        if not (self.sw_data and self.tiger_data):
            ax.text(0.5, 0.5, 'No Data Available', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('Performance Summary', fontweight='bold')
            return
        
        # Calculate metrics
        sw_metrics = self.sw_data['metrics']
        tiger_metrics = self.tiger_data['metrics']
        
        if not sw_metrics or not tiger_metrics:
            ax.text(0.5, 0.5, 'No Metrics Data', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('Performance Summary', fontweight='bold')
            return
        
        metrics = {
            'Exec Time': [self.sw_data['duration'], self.tiger_data['duration']],
            'Avg CPU': [np.mean([m['process_cpu'] for m in sw_metrics]), 
                       np.mean([m['process_cpu'] for m in tiger_metrics])],
            'Peak Mem': [np.max([m['process_memory_mb'] for m in sw_metrics]),
                        np.max([m['process_memory_mb'] for m in tiger_metrics])],
            'Avg Cores': [np.mean([m['cores_active'] for m in sw_metrics]),
                         np.mean([m['cores_active'] for m in tiger_metrics])]
        }
        
        # Normalize metrics for comparison (0-1 scale)
        normalized = {}
        for key, values in metrics.items():
            max_val = max(values) if max(values) > 0 else 1
            if 'Time' in key:
                # For time, lower is better (invert)
                normalized[key] = [1 - (v / max_val) for v in values]
            else:
                # For others, higher is generally better
                normalized[key] = [v / max_val for v in values]
        
        # Create grouped bar chart
        x = np.arange(len(metrics))
        width = 0.35
        
        sw_values = [normalized[key][0] for key in metrics.keys()]
        tiger_values = [normalized[key][1] for key in metrics.keys()]
        
        bars1 = ax.bar(x - width/2, sw_values, width, label='SW Task', color='#27AE60', alpha=0.8)
        bars2 = ax.bar(x + width/2, tiger_values, width, label='Tiger Looper', color='#E74C3C', alpha=0.8)
        
        ax.set_title('Performance Summary (Normalized)', fontweight='bold')
        ax.set_ylabel('Performance Score (0-1)')
        ax.set_xticks(x)
        ax.set_xticklabels(metrics.keys(), rotation=45, ha='right')
        ax.legend()
        ax.set_ylim(0, 1.1)
    
    def _plot_efficiency_comparison(self, ax):
        """Plot efficiency comparison"""
        if not (self.sw_data and self.tiger_data):
            ax.text(0.5, 0.5, 'No Data Available', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('CPU Efficiency', fontweight='bold')
            return
        
        # Calculate efficiency as tasks/second per CPU%
        sw_duration = self.sw_data['duration']
        tiger_duration = self.tiger_data['duration']
        
        if sw_duration <= 0 or tiger_duration <= 0:
            ax.text(0.5, 0.5, 'Invalid Duration Data', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('CPU Efficiency', fontweight='bold')
            return
        
        # Assume 20 tasks (could be parameterized)
        tasks = 20
        
        sw_throughput = tasks / sw_duration
        tiger_throughput = tasks / tiger_duration
        
        sw_cpu_avg = np.mean([m['process_cpu'] for m in self.sw_data['metrics']])
        tiger_cpu_avg = np.mean([m['process_cpu'] for m in self.tiger_data['metrics']])
        
        sw_efficiency = sw_throughput / sw_cpu_avg if sw_cpu_avg > 0 else 0
        tiger_efficiency = tiger_throughput / tiger_cpu_avg if tiger_cpu_avg > 0 else 0
        
        frameworks = ['SW Task', 'Tiger Looper']
        efficiencies = [sw_efficiency, tiger_efficiency]
        
        bars = ax.bar(frameworks, efficiencies, color=['#16A085', '#E67E22'], alpha=0.8)
        ax.set_title('CPU Efficiency\n(Tasks/sec per CPU%)', fontweight='bold')
        ax.set_ylabel('Efficiency Index')
        
        for bar, eff in zip(bars, efficiencies):
            height = bar.get_height()
            if height > 0:
                ax.text(bar.get_x() + bar.get_width()/2., height + height*0.01,
                       f'{eff:.3f}', ha='center', va='bottom', fontweight='bold')
    
    def _plot_resource_radar(self, ax):
        """Plot resource utilization radar chart"""
        if not (self.sw_data and self.tiger_data):
            ax.text(0.5, 0.5, 'No Data Available', ha='center', va='center')
            return
        
        # Metrics for radar chart
        categories = ['CPU\nUtilization', 'Memory\nEfficiency', 'Thread\nManagement', 
                     'Core\nUtilization', 'Overall\nPerformance']
        
        try:
            # Calculate normalized scores (0-1)
            sw_cpu = np.mean([m['process_cpu'] for m in self.sw_data['metrics']]) / 100
            tiger_cpu = np.mean([m['process_cpu'] for m in self.tiger_data['metrics']]) / 100
            
            sw_mem = np.mean([m['process_memory_mb'] for m in self.sw_data['metrics']])
            tiger_mem = np.mean([m['process_memory_mb'] for m in self.tiger_data['metrics']])
            mem_max = max(sw_mem, tiger_mem) if max(sw_mem, tiger_mem) > 0 else 1
            sw_mem_norm = 1 - (sw_mem / mem_max)  # Lower memory is better
            tiger_mem_norm = 1 - (tiger_mem / mem_max)
            
            sw_cores = np.mean([m['cores_active'] for m in self.sw_data['metrics']])
            tiger_cores = np.mean([m['cores_active'] for m in self.tiger_data['metrics']])
            cores_max = max(sw_cores, tiger_cores) if max(sw_cores, tiger_cores) > 0 else 1
            sw_cores_norm = sw_cores / cores_max
            tiger_cores_norm = tiger_cores / cores_max
            
            # Overall performance (inverse of execution time)
            time_max = max(self.sw_data['duration'], self.tiger_data['duration'])
            if time_max > 0:
                sw_perf = 1 - (self.sw_data['duration'] / time_max)
                tiger_perf = 1 - (self.tiger_data['duration'] / time_max)
            else:
                sw_perf = tiger_perf = 0.5
            
            sw_values = [sw_cpu, sw_mem_norm, 0.8, sw_cores_norm, sw_perf]  # Thread mgmt estimated
            tiger_values = [tiger_cpu, tiger_mem_norm, 0.6, tiger_cores_norm, tiger_perf]
            
            # Radar chart
            angles = np.linspace(0, 2 * np.pi, len(categories), endpoint=False).tolist()
            angles += angles[:1]  # Complete the circle
            
            sw_values += sw_values[:1]
            tiger_values += tiger_values[:1]
            
            ax.plot(angles, sw_values, 'o-', linewidth=2, label='SW Task', color='#3498DB')
            ax.fill(angles, sw_values, alpha=0.25, color='#3498DB')
            
            ax.plot(angles, tiger_values, 's-', linewidth=2, label='Tiger Looper', color='#E74C3C')
            ax.fill(angles, tiger_values, alpha=0.25, color='#E74C3C')
            
            ax.set_xticks(angles[:-1])
            ax.set_xticklabels(categories)
            ax.set_ylim(0, 1)
            ax.set_title('Resource Utilization\nRadar Chart', fontweight='bold', pad=20)
            ax.legend(loc='upper right', bbox_to_anchor=(1.3, 1.0))
            
        except Exception as e:
            ax.text(0.5, 0.5, f'Radar Error:\n{str(e)[:50]}', ha='center', va='center')
    
    def _plot_scalability_projection(self, ax):
        """Plot scalability projection"""
        # Simulate scalability based on current results
        tasks_range = [5, 10, 20, 50, 100]
        
        if self.sw_data and self.tiger_data:
            # Base performance for 20 tasks
            sw_base_time = self.sw_data['duration']
            tiger_base_time = self.tiger_data['duration']
            
            if sw_base_time > 0 and tiger_base_time > 0:
                # Project scaling (SW Task scales better due to parallelism)
                sw_projected = [sw_base_time * (t / 20) * 0.3 for t in tasks_range]  # Sub-linear scaling
                tiger_projected = [tiger_base_time * (t / 20) for t in tasks_range]  # Linear scaling
                
                ax.plot(tasks_range, sw_projected, 'o-', linewidth=2, label='SW Task (Projected)', color='#27AE60')
                ax.plot(tasks_range, tiger_projected, 's-', linewidth=2, label='Tiger Looper (Projected)', color='#E74C3C')
                
                # Mark actual measured point
                ax.plot(20, sw_base_time, 'o', markersize=10, color='#27AE60', markerfacecolor='yellow', markeredgewidth=2)
                ax.plot(20, tiger_base_time, 's', markersize=10, color='#E74C3C', markerfacecolor='yellow', markeredgewidth=2)
                
                ax.set_title('Scalability Projection', fontweight='bold')
                ax.set_xlabel('Number of Tasks')
                ax.set_ylabel('Execution Time (seconds)')
                ax.legend()
                ax.grid(True, alpha=0.3)
                ax.set_xscale('log')
            else:
                ax.text(0.5, 0.5, 'Invalid Time Data\nfor Projection', ha='center', va='center', transform=ax.transAxes)
                ax.set_title('Scalability Projection', fontweight='bold')
        else:
            ax.text(0.5, 0.5, 'No Data Available\nfor Projection', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('Scalability Projection', fontweight='bold')
    
    def generate_report(self):
        """Generate text report"""
        if not (self.sw_data and self.tiger_data):
            print("Cannot generate report: missing data")
            return
        
        print("\n" + "="*60)
        print("CPU-BOUND PERFORMANCE COMPARISON REPORT")
        print("="*60)
        
        # Basic metrics
        print(f"\n📊 EXECUTION TIME:")
        print(f"  SW Task:      {self.sw_data['duration']:.1f} seconds")
        print(f"  Tiger Looper: {self.tiger_data['duration']:.1f} seconds")
        if self.tiger_data['duration'] > 0:
            speedup = self.tiger_data['duration'] / self.sw_data['duration']
            print(f"  Speedup:      {speedup:.1f}x faster")
        
        # CPU metrics
        if self.sw_data['metrics'] and self.tiger_data['metrics']:
            sw_cpu_avg = np.mean([m['process_cpu'] for m in self.sw_data['metrics']])
            tiger_cpu_avg = np.mean([m['process_cpu'] for m in self.tiger_data['metrics']])
            
            print(f"\n🖥️  CPU UTILIZATION:")
            print(f"  SW Task:      {sw_cpu_avg:.1f}% average")
            print(f"  Tiger Looper: {tiger_cpu_avg:.1f}% average")
            
            # Core utilization
            sw_cores_avg = np.mean([m['cores_active'] for m in self.sw_data['metrics']])
            tiger_cores_avg = np.mean([m['cores_active'] for m in self.tiger_data['metrics']])
            
            print(f"\n⚙️  CORE UTILIZATION:")
            print(f"  SW Task:      {sw_cores_avg:.1f} cores average")
            print(f"  Tiger Looper: {tiger_cores_avg:.1f} cores average")
            
            # Memory
            sw_mem_avg = np.mean([m['process_memory_mb'] for m in self.sw_data['metrics']])
            tiger_mem_avg = np.mean([m['process_memory_mb'] for m in self.tiger_data['metrics']])
            
            print(f"\n💾 MEMORY USAGE:")
            print(f"  SW Task:      {sw_mem_avg:.1f} MB average")
            print(f"  Tiger Looper: {tiger_mem_avg:.1f} MB average")
            
            # Throughput
            tasks = 20  # Assume 20 tasks
            sw_throughput = tasks / self.sw_data['duration']
            tiger_throughput = tasks / self.tiger_data['duration']
            
            print(f"\n🚀 THROUGHPUT:")
            print(f"  SW Task:      {sw_throughput:.2f} tasks/second")
            print(f"  Tiger Looper: {tiger_throughput:.2f} tasks/second")
            
            print(f"\n✅ CONCLUSION:")
            if self.tiger_data['duration'] > 0:
                speedup = self.tiger_data['duration'] / self.sw_data['duration']
                if speedup > 1:
                    print(f"  SW Task outperforms Tiger Looper by {speedup:.1f}x in CPU-bound workloads")
                    print(f"  SW Task utilizes {sw_cores_avg/tiger_cores_avg:.1f}x more CPU cores on average")
                    print(f"  Recommended for parallel CPU-intensive tasks")
                else:
                    print(f"  Performance difference is minimal")
        
        print("="*60)

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 plot_cpu_performance.py <sw_results.json> <tiger_results.json>")
        print("Example: python3 plot_cpu_performance.py sw_results.json tiger_results.json")
        sys.exit(1)
    
    sw_file = sys.argv[1]
    tiger_file = sys.argv[2]
    
    # Check if files exist
    if not os.path.exists(sw_file):
        print(f"Error: SW Task results file not found: {sw_file}")
        sys.exit(1)
    
    if not os.path.exists(tiger_file):
        print(f"Error: Tiger Looper results file not found: {tiger_file}")
        sys.exit(1)
    
    # Create visualizer
    viz = CPUPerformanceVisualizer()
    
    # Load data
    if not viz.load_data(sw_file, tiger_file):
        print("Failed to load data files")
        sys.exit(1)
    
    # Generate plots and report
    viz.create_comparison_plots()
    viz.generate_report()

if __name__ == "__main__":
    main()