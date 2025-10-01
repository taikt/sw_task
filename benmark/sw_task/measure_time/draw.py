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
           'o-', color='#2E8B57', linewidth=2.5, markersize=7,
           label='SW Task Framework', markerfacecolor='lightgreen',
           markeredgecolor='#2E8B57', markeredgewidth=1.2)
    
    # Plot Tiger Looper (Red line)
    ax.plot(tiger_sorted['Task_ID'], tiger_sorted['Execution_Time_ms'],
           's-', color='#DC143C', linewidth=2.5, markersize=7,
           label='Tiger Looper', markerfacecolor='#FFB6C1',
           markeredgecolor='#DC143C', markeredgewidth=1.2)
    
    # Customize axes
    ax.set_xlabel('Task ID', fontsize=14, fontweight='bold')
    ax.set_ylabel('Execution Time (ms)', fontsize=14, fontweight='bold')
    ax.set_title('Framework Performance Comparison', fontsize=16, fontweight='bold', pad=20)
    
    # Grid and background styling (similar to scalability chart)
    ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    ax.set_facecolor('#f8f9fa')
    
    # Better legend positioning
    ax.legend(fontsize=12, loc='upper left', bbox_to_anchor=(0.02, 0.98),
              frameon=True, fancybox=True, shadow=True, framealpha=0.95,
              edgecolor='black', facecolor='white')
    
    # Set axis limits
    all_task_ids = pd.concat([sw_sorted['Task_ID'], tiger_sorted['Task_ID']])
    ax.set_xlim(all_task_ids.min() - 0.5, all_task_ids.max() + 0.5)
    
    all_times = pd.concat([sw_sorted['Execution_Time_ms'], tiger_sorted['Execution_Time_ms']])
    ax.set_ylim(0, all_times.max() * 1.1)
    
    # Calculate performance metrics for annotation
    sw_times = sw_sorted['Execution_Time_ms']
    tiger_times = tiger_sorted['Execution_Time_ms']
    
    # Improved annotations - only for final values to avoid clutter
    final_sw = sw_sorted.iloc[-1]
    final_tiger = tiger_sorted.iloc[-1]
    
    ax.annotate(f'{final_sw["Execution_Time_ms"]:.1f}ms', 
                xy=(final_sw['Task_ID'], final_sw['Execution_Time_ms']),
                xytext=(10, -15), textcoords='offset points', fontsize=11,
                color='#2E8B57', weight='bold', ha='left')
    
    ax.annotate(f'{final_tiger["Execution_Time_ms"]:.1f}ms',
                xy=(final_tiger['Task_ID'], final_tiger['Execution_Time_ms']),
                xytext=(10, 10), textcoords='offset points', fontsize=11,
                color='#DC143C', weight='bold', ha='left')
    
    # Add performance improvement text box
    if tiger_times.mean() > sw_times.mean():
        improvement = ((tiger_times.mean() - sw_times.mean()) / tiger_times.mean()) * 100
        max_ratio = tiger_times.max() / sw_times.max()
        mean_ratio = tiger_times.mean() / sw_times.mean()
        
        improvement_text = f"SW Task Performance:\n• {improvement:.1f}% faster (avg)\n• {max_ratio:.1f}x faster (max)\n• {mean_ratio:.1f}x faster (mean)"
        ax.text(0.98, 0.02, improvement_text, transform=ax.transAxes,
                bbox=dict(boxstyle="round,pad=0.4", facecolor='lightgreen', alpha=0.8, edgecolor='#2E8B57'),
                fontsize=10, ha='right', va='bottom', color='#2E8B57', weight='bold')
    
    # Tight layout
    plt.tight_layout()
    
    # Save with high DPI
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Chart saved as: {output_file}")
    plt.show()
    
    # Print comparison statistics and generate summary table
    print_comparison_statistics(sw_df, tiger_df)
    generate_performance_summary_table(sw_df, tiger_df)

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
    
    # QA-01 Compliance Check
    print(f"\nQA-01 Compliance (Response time < 1000ms):")
    sw_violations = (sw_times > 1000).sum()
    tiger_violations = (tiger_times > 1000).sum()
    
    print(f"  SW Task violations: {sw_violations}/{len(sw_times)} ({sw_violations/len(sw_times)*100:.1f}%)")
    print(f"  Tiger Looper violations: {tiger_violations}/{len(tiger_times)} ({tiger_violations/len(tiger_times)*100:.1f}%)")
    
    if sw_violations == 0:
        print("  ✅ SW Task Framework MEETS QA-01 requirement")
    else:
        print("  ❌ SW Task Framework FAILS QA-01 requirement")
        
    if tiger_violations == 0:
        print("  ✅ Tiger Looper MEETS QA-01 requirement")
    else:
        print("  ❌ Tiger Looper FAILS QA-01 requirement")

def generate_performance_summary_table(sw_df, tiger_df):
    """Generate performance comparison table"""
    sw_times = sw_df['Execution_Time_ms'] 
    tiger_times = tiger_df['Execution_Time_ms']
    
    # Calculate improvement ratios
    mean_ratio = tiger_times.mean() / sw_times.mean() if sw_times.mean() > 0 else 0
    max_ratio = tiger_times.max() / sw_times.max() if sw_times.max() > 0 else 0
    std_ratio = tiger_times.std() / sw_times.std() if sw_times.std() > 0 else 0
    
    summary = {
        'Metric': ['Tasks Count', 'Mean (ms)', 'Max (ms)', 'Min (ms)', 'Std Dev (ms)', 'QA-01 Violations'],
        'SW Task': [
            len(sw_times), 
            f"{sw_times.mean():.1f}", 
            f"{sw_times.max():.1f}", 
            f"{sw_times.min():.1f}", 
            f"{sw_times.std():.1f}",
            f"{(sw_times > 1000).sum()}"
        ],
        'Tiger Looper': [
            len(tiger_times), 
            f"{tiger_times.mean():.1f}",
            f"{tiger_times.max():.1f}", 
            f"{tiger_times.min():.1f}",
            f"{tiger_times.std():.1f}",
            f"{(tiger_times > 1000).sum()}"
        ],
        'SW Task Advantage': [
            '-', 
            f"{mean_ratio:.1f}x faster",
            f"{max_ratio:.1f}x faster", 
            '-',
            f"{std_ratio:.1f}x more stable" if std_ratio < 1 else f"{1/std_ratio:.1f}x less stable",
            'Meets QA-01' if (sw_times > 1000).sum() == 0 else 'Fails QA-01'
        ]
    }
    
    df_summary = pd.DataFrame(summary)
    print("\n" + "="*80)
    print("PERFORMANCE SUMMARY TABLE")
    print("="*80) 
    print(df_summary.to_string(index=False))
    print("="*80)
    
    return df_summary

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
            print("Framework Performance Comparison Tool")
            print("="*40)
            print("Usage:")
            print("  python3 draw_line.py                              # Use default files")
            print("  python3 draw_line.py sw.csv tiger.csv             # Specify both files")
            print("  python3 draw_line.py sw.csv tiger.csv output.png  # Specify all files")
            print("  python3 draw_line.py help                         # Show this help")
            print()
            print("Default files:")
            print(f"  SW Task CSV: {sw_csv}")
            print(f"  Tiger Looper CSV: {tiger_csv}")
            print(f"  Output PNG: {output_file}")
            print()
            print("Required CSV format:")
            print("  Task_ID,Task_Type,Execution_Time_ms")
            print("  0,LIGHT,245.3")
            print("  1,HEAVY,1234.5")
            print()
            print("Features:")
            print("  • Line chart comparison")
            print("  • Performance statistics")
            print("  • QA-01 compliance check")
            print("  • Summary table generation")
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