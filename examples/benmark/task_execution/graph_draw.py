import pandas as pd
import matplotlib.pyplot as plt
import sys
import os
from matplotlib.lines import Line2D

def main():
    # Check command line arguments
    if len(sys.argv) < 2:
        print("Usage:")
        print(f"  {sys.argv[0]} <csv_file>")
        print(f"  {sys.argv[0]} <csv_file> [output_image]")
        print()
        print("Examples:")
        print(f"  {sys.argv[0]} tiger_execution_times.csv")
        print(f"  {sys.argv[0]} execution_times.csv result_chart.png")
        print(f"  {sys.argv[0]} test_data.csv")
        return
    
    csv_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    # Check if CSV file exists
    if not os.path.exists(csv_file):
        print(f"❌ Error: File '{csv_file}' not found!")
        print("Available CSV files in current directory:")
        csv_files = [f for f in os.listdir('.') if f.endswith('.csv')]
        if csv_files:
            for f in csv_files:
                print(f"  - {f}")
        else:
            print("  No CSV files found")
        return
    
    try:
        # Read CSV file
        print(f"📊 Reading CSV: {csv_file}")
        df = pd.read_csv(csv_file)
        
        # Display data preview
        print("\nData preview:")
        print(df.head())
        print(f"\nColumns: {list(df.columns)}")
        print(f"Total rows: {len(df)}")
        
        # Check required columns
        required_cols = ['Task_ID', 'Execution_Time_ms']
        missing_cols = [col for col in required_cols if col not in df.columns]
        if missing_cols:
            print(f"❌ Error: Missing required columns: {missing_cols}")
            print(f"Available columns: {list(df.columns)}")
            return
        
        # Check for Task_Type column (optional but recommended)
        has_task_type = 'Task_Type' in df.columns
        if has_task_type:
            print(f"Task types: {df['Task_Type'].value_counts().to_dict()}")
        
        # Create visualization
        create_execution_time_chart(df, has_task_type, output_file)
        
        # Print statistics
        print_statistics(df, has_task_type)
        
    except Exception as e:
        print(f"❌ Error reading CSV: {e}")
        return

def create_execution_time_chart(df, has_task_type, output_file=None):
    """Create execution time bar chart with optional task type coloring"""
    
    plt.figure(figsize=(14, 8))
    
    if has_task_type:
        # Create colors based on task type
        colors = []
        color_map = {
            'LIGHT': 'skyblue',
            'HEAVY': 'orange', 
            'light': 'skyblue',
            'heavy': 'orange'
        }
        
        for task_type in df['Task_Type']:
            colors.append(color_map.get(str(task_type).upper(), 'gray'))
        
        # Bar chart with colors
        bars = plt.bar(df['Task_ID'], df['Execution_Time_ms'], color=colors, alpha=0.8)
        
        # Add legend
        unique_types = df['Task_Type'].unique()
        legend_elements = []
        for task_type in unique_types:
            color = color_map.get(str(task_type).upper(), 'gray')
            legend_elements.append(
                Line2D([0], [0], marker='s', color='w', 
                      markerfacecolor=color, markersize=12, 
                      label=str(task_type).upper())
            )
        plt.legend(handles=legend_elements, loc='upper right')
        
        title = 'Execution Time per Task (by Type)'
    else:
        # Simple bar chart without task type
        bars = plt.bar(df['Task_ID'], df['Execution_Time_ms'], 
                      color='steelblue', alpha=0.7)
        title = 'Execution Time per Task'
    
    # Customize chart
    plt.title(title, fontsize=16, pad=20)
    plt.xlabel('Task ID', fontsize=12)
    plt.ylabel('Execution Time (ms)', fontsize=12)
    plt.grid(True, alpha=0.3, axis='y')
    
    # Add value labels on bars (for smaller datasets)
    if len(df) <= 20:
        for i, bar in enumerate(bars):
            height = bar.get_height()
            plt.text(bar.get_x() + bar.get_width()/2., height + max(df['Execution_Time_ms']) * 0.01,
                    f'{height:.1f}', ha='center', va='bottom', fontsize=9)
    
    # Set x-axis ticks
    plt.xticks(df['Task_ID'])
    
    # Auto-adjust layout
    plt.tight_layout()
    
    # Save or show
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"📈 Chart saved: {output_file}")
    else:
        # Auto-generate output filename
        base_name = os.path.splitext(os.path.basename(sys.argv[1]))[0]
        auto_output = f"{base_name}_chart.png"
        plt.savefig(auto_output, dpi=300, bbox_inches='tight')
        print(f"📈 Chart saved: {auto_output}")
    
    plt.show()

def print_statistics(df, has_task_type):
    """Print execution time statistics"""
    
    print("\n" + "="*50)
    print("📊 EXECUTION TIME STATISTICS")
    print("="*50)
    
    if has_task_type:
        # Statistics by task type
        for task_type in df['Task_Type'].unique():
            subset = df[df['Task_Type'] == task_type]
            times = subset['Execution_Time_ms']
            
            print(f"\n{str(task_type).upper()} TASKS:")
            print(f"  Count: {len(subset)}")
            print(f"  Average: {times.mean():.2f} ms")
            print(f"  Min: {times.min():.2f} ms")
            print(f"  Max: {times.max():.2f} ms")
            print(f"  Median: {times.median():.2f} ms")
            print(f"  Std Dev: {times.std():.2f} ms")
    
    # Overall statistics
    times = df['Execution_Time_ms']
    print(f"\nOVERALL:")
    print(f"  Total tasks: {len(df)}")
    print(f"  Average: {times.mean():.2f} ms")
    print(f"  Min: {times.min():.2f} ms")
    print(f"  Max: {times.max():.2f} ms")
    print(f"  Median: {times.median():.2f} ms")
    print(f"  Total time: {times.sum():.1f} ms")
    print(f"  Std Dev: {times.std():.2f} ms")
    
    # Performance targets (if task types available)
    if has_task_type:
        light_tasks = df[df['Task_Type'].str.upper() == 'LIGHT']
        heavy_tasks = df[df['Task_Type'].str.upper() == 'HEAVY']
        
        if len(light_tasks) > 0:
            violations = len(light_tasks[light_tasks['Execution_Time_ms'] > 500])
            print(f"\nLIGHT TASK SLA (target: ≤500ms):")
            print(f"  Violations: {violations}/{len(light_tasks)} ({violations/len(light_tasks)*100:.1f}%)")
        
        if len(heavy_tasks) > 0:
            violations = len(heavy_tasks[heavy_tasks['Execution_Time_ms'] > 1000])
            print(f"\nHEAVY TASK SLA (target: ≤1000ms):")
            print(f"  Violations: {violations}/{len(heavy_tasks)} ({violations/len(heavy_tasks)*100:.1f}%)")

if __name__ == "__main__":
    main()