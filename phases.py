import pandas as pd
import subprocess
import sys

max_x = 260
max_y = 64*1000*1000
phase_period = 8*1000*1000

# Load the dataset
data_path = sys.argv[1]
output_script_path = 'phases.plt'  # The output Gnuplot script file
out_file = data_path.replace('txt', 'pdf')
data = pd.read_csv(data_path, sep='\s+', header=None)  # Adjust separator as needed
data.columns = ['x', 'y', 'line_style', 'background']

# Initialize the Gnuplot script content
gnuplot_script = f"""
set terminal pdfcairo enhanced color size 5in,3in font "Linux Libertine, 12"
set output '{out_file}'

set style line 1 lt 1 lw 2 lc rgb "blue" # Solid line
set style line 2 dt "." lw 2 lc rgb "blue" # Dashed line

set style line 101 lc rgb "black" lt 1 lw 2 # Line style for border

# Background styles (semi-transparent colors)
set style fill transparent solid 0.5

set xlabel "Wall Clock Time"
set ylabel "Global Virtual Time"
set xrange [0:{max_x}]
set yrange [0:{max_y}]
set key below
set bmargin 5.5
set ytics {phase_period}
set grid noxtics ytics
"""

# Initialize variables to keep track of the current phase and its start
current_phase = None
phase_start = None
rectangles = []

# Iterate through the DataFrame
for index, row in data.iterrows():
    # Check if we're entering a new phase
    if row['background'] != current_phase:
        # If not the first row and a phase was active, close the previous rectangle
        if current_phase is not None:
            rectangles.append((phase_start, data.at[index - 1, 'x'], current_phase))
        # Update current phase and start point
        current_phase = row['background']
        phase_start = row['x']
# Ensure the last phase is closed out
if current_phase is not None:
    rectangles.append((phase_start, data.at[index, 'x'], current_phase))

# Colors for the rectangles
colors = {0: "green", 1: "yellow"}

# Generate Gnuplot commands for rectangles
for i, (start, end, phase) in enumerate(rectangles, start=1):
    gnuplot_script += f"\nset object {i} rect from {start}, graph 0 to {end}, graph 1 back fc rgb \"{colors[phase]}\" fs transparent solid 0.2 noborder"

# Add plot command
gnuplot_script += "\n\n# Plot command to iterate through unique line styles in the third column"
gnuplot_script += f"\nplot for [i=0:1] '{data_path}' u 1:($3==i?$2:1/0) w lines linestyle i+1 notitle,\\"
gnuplot_script += "\nNaN  title 'Unbalanced' with boxes ls 101 lc rgb \"yellow\" fs transparent solid 0.2,\\"
gnuplot_script += "\nNaN  title 'Balanced' with boxes ls 101 lc rgb \"green\" fs transparent solid 0.2,\\"
gnuplot_script += "\nNaN  title 'On CPU' w lines linestyle 1,\\"
gnuplot_script += "\nNaN  title 'On GPU' w lines linestyle 2"


# Write the Gnuplot script to file
with open(output_script_path, 'w') as file:
    file.write(gnuplot_script)

# Run the Gnuplot script using subprocess
subprocess.run(['gnuplot', output_script_path])

print(f"Gnuplot script has been executed. Output saved to {out_file}")
