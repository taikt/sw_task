import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import sys
import os

def create_framework_comparison_line_chart(sw_csv='execution_times.csv', 
                                         tiger_csv='tiger_execution_times.csv',
                                         output_file='framework_comparison_line.png'):
    """
    Compare SW Task vs Tiger Looper in single line chart
    Each framework shows one line combining all tasks (light + heavy)
    """
    
    # Check if files exist
    if not os.path.exists(sw_csv):
        print(f"Error: {sw_csv} not found!")
        return
    if not os.path.exists(tiger_csv):
        print(f"Error: {tiger_csv} not found!")
        return
    
    # Read data
    try:
        sw_df = pd.read_csv(sw_csv)
        tiger_df = pd.read_csv(tiger_csv)
        print(f"Loaded SW Task: {len(sw_df)} tasks")
        print(f"Loaded Tiger Looper: {len(tiger_df)} tasks")
    except Exception as e:
        print(f"Error reading CSV files: {e}")
        return
    
    # Create the plot
    fig, ax = plt.subplots(figsize=(12, 8))
    
    # Sort by Task_ID for proper line connection
    sw_sorted = sw_df.sort_values('Task_ID')
    tiger_sorted = tiger_df.sort_values('Task_ID')
    
    # Plot SW Task Framework (Green line)
    ax.plot(sw_sorted['Task_ID'], sw_sorted['Execution_Time_ms'],
           'o-', color='green', linewidth=3, markersize=8,
           label='SW Task Framework', markerfacecolor='lightgreen',
           markeredgecolor='darkgreen', markeredgewidth=1.5)
    
    # Plot Tiger Looper (Red line)
    ax.plot(tiger_sorted['Task_ID'], tiger_sorted['Execution_Time_ms'],
           's-', color='red', linewidth=3, markersize=8,
           label='Tiger Looper', markerfacecolor='lightcoral',
           markeredgecolor='darkred', markeredgewidth=1.5)
    
    # Customize axes
    ax.set_xlabel('Task ID', fontsize=14, fontweight='bold')
    ax.set_ylabel('Execution Time (ms)', fontsize=14, fontweight='bold')
    ax.set_title('Framework Performance Comparison', fontsize=16, fontweight='bold', pad=20)
    
    # Grid and background styling (similar to scalability chart)
    ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    ax.set_facecolor('#f8f9fa')
    
    # Legend with styling
    ax.legend(fontsize=13, loc='upper left', frameon=True,
              fancybox=True, shadow=True, framealpha=0.9,
              edgecolor='black', facecolor='white')
    
    # Set axis limits
    all_task_ids = pd.concat([sw_sorted['Task_ID'], tiger_sorted['Task_ID']])
    ax.set_xlim(all_task_ids.min() - 0.5, all_task_ids.max() + 0.5)
    
    all_times = pd.concat([sw_sorted['Execution_Time_ms'], tiger_sorted['Execution_Time_ms']])
    ax.set_ylim(0, all_times.max() * 1.1)
    
    # Add value annotations for key points (optional)
    # Annotate max values
    sw_max_idx = sw_sorted['Execution_Time_ms'].idxmax()
    tiger_max_idx = tiger_sorted['Execution_Time_ms'].idxmax()
    
    sw_max_task = sw_sorted.loc[sw_max_idx]
    tiger_max_task = tiger_sorted.loc[tiger_max_idx]
    
    ax.annotate(f'{sw_max_task["Execution_Time_ms"]:.1f}ms', 
                xy=(sw_max_task['Task_ID'], sw_max_task['Execution_Time_ms']),
                xytext=(5, 5), textcoords='offset points', fontsize=10,
                color='darkgreen', weight='bold')
    
    ax.annotate(f'{tiger_max_task["Execution_Time_ms"]:.1f}ms',
                xy=(tiger_max_task['Task_ID'], tiger_max_task['Execution_Time_ms']),
                xytext=(5, 5), textcoords='offset points', fontsize=10,
                color='darkred', weight='bold')
    
    # Tight layout
    plt.tight_layout()
    
    # Save with high DPI
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Chart saved as: {output_file}")
    plt.show()
    
    # Print comparison statistics
    print_comparison_statistics(sw_df, tiger_df)

def print_comparison_statistics(sw_df, tiger_df):
    """Print detailed comparison statistics"""
    print("\n" + "="*60)
    print("FRAMEWORK PERFORMANCE COMPARISON")
    print("="*60)
    
    sw_times = sw_df['Execution_Time_ms']
    tiger_times = tiger_df['Execution_Time_ms']
    
    print(f"\nSW Task Framework:")
    print(f"  Tasks: {len(sw_times)}")
    print(f"  Mean: {sw_times.mean():.2f} ms")
    print(f"  Min: {sw_times.min():.2f} ms")
    print(f"  Max: {sw_times.max():.2f} ms")
    print(f"  Median: {sw_times.median():.2f} ms")
    print(f"  Std Dev: {sw_times.std():.2f} ms")
    
    print(f"\nTiger Looper:")
    print(f"  Tasks: {len(tiger_times)}")
    print(f"  Mean: {tiger_times.mean():.2f} ms")
    print(f"  Min: {tiger_times.min():.2f} ms")
    print(f"  Max: {tiger_times.max():.2f} ms")
    print(f"  Median: {tiger_times.median():.2f} ms")
    print(f"  Std Dev: {tiger_times.std():.2f} ms")
    
    # Performance comparison
    print(f"\nPerformance Improvement:")
    if tiger_times.mean() > sw_times.mean():
        improvement = ((tiger_times.mean() - sw_times.mean()) / tiger_times.mean()) * 100
        print(f"  SW Task is {improvement:.1f}% faster on average")
    else:
        degradation = ((sw_times.mean() - tiger_times.mean()) / tiger_times.mean()) * 100
        print(f"  SW Task is {degradation:.1f}% slower on average")
    
    print(f"  Max time ratio: {tiger_times.max() / sw_times.max():.2f}x")
    print(f"  Mean time ratio: {tiger_times.mean() / sw_times.mean():.2f}x")

def main():
    """Main function with flexible argument handling"""
    
    # Default file names
    sw_csv = 'execution_times.csv'
    tiger_csv = 'tiger_execution_times.csv'
    output_file = 'framework_comparison_line.png'
    
    if len(sys.argv) == 1:
        # No arguments - use defaults
        print("Using default files:")
        print(f"  SW Task: {sw_csv}")
        print(f"  Tiger Looper: {tiger_csv}")
        print(f"  Output: {output_file}")
        
    elif len(sys.argv) == 2:
        # One argument - show help
        if sys.argv[1] in ['-h', '--help', 'help']:
            print("Usage:")
            print("  python3 draw_line.py                              # Use default files")
            print("  python3 draw_line.py sw.csv tiger.csv             # Specify both files")
            print("  python3 draw_line.py sw.csv tiger.csv output.png  # Specify all files")
            print()
            print("Default files:")
            print(f"  SW Task: {sw_csv}")
            print(f"  Tiger Looper: {tiger_csv}")
            print(f"  Output: {output_file}")
            return
        else:
            print("Error: Need both SW Task and Tiger Looper CSV files")
            print("Use 'python3 draw_line.py help' for usage info")
            return
            
    elif len(sys.argv) == 3:
        # Two arguments - both CSV files
        sw_csv = sys.argv[1]
        tiger_csv = sys.argv[2]
        
    elif len(sys.argv) == 4:
        # Three arguments - both CSV files + output
        sw_csv = sys.argv[1]
        tiger_csv = sys.argv[2]
        output_file = sys.argv[3]
        
    else:
        print("Error: Too many arguments")
        print("Use 'python3 draw_line.py help' for usage info")
        return
    
    # Create the comparison chart
    create_framework_comparison_line_chart(sw_csv, tiger_csv, output_file)

if __name__ == "__main__":
    main()