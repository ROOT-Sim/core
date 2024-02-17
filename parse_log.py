import pandas as pd
import re
import sys

log_path = sys.argv[1]
res_file = log_path.replace('.txt','.processed.txt')
data = []
background_phase = 1  # Assume 0 for COLD, switch to 1 for HOT

in_challenge = False
y_value = 0
x_value = 0
line_style = 0

last_before_challenge = None

with open(log_path, 'r') as file:
    for line in file:
        if "Starting challenge" in line:
            in_challenge = True
        elif "the challenge is completed" in line:
            in_challenge = False
            if last_before_challenge:
                last_before_challenge[2] = 1 - last_before_challenge[2]
                data.append(last_before_challenge)
        elif "ENTER COLD PHASE" in line:
            background_phase = 0
            data.append([x_value, y_value, line_style, background_phase])
        elif "ENTER HOT PHASE" in line:
            background_phase = 1
            data.append([x_value, y_value, line_style, background_phase])
        elif "GPU GVT" in line or "CPU GVT" in line:
            if in_challenge:
                continue
            # Extract GVT data
            if ("GPU" in line or "CPU" in line) and "GVT" in line:
                line = " ".join(line.split(",")[0].split(" ")[0:2]+[",".join(line.split(",")[-2:])])
            match = re.search(r'GVT\s+(\d+\.\d+),\s*(\d+\.\d+)', line)
            print(line)
            if match:
                y_value = float(match.group(1))
                x_value = float(match.group(2))
                line_style = 1 if "GPU GVT" in line else 0  # 0 for dashed (GPU), 1 for solid (CPU)
                # Append the extracted data along with the current background phase
                last_before_challenge = [x_value, y_value, line_style, background_phase]
                data.append(last_before_challenge)

# Create a pandas DataFrame
df = pd.DataFrame(data, columns=['x', 'y', 'line_style', 'background_phase'])

# Save the DataFrame to a text file
df.to_csv(res_file, sep=' ', index=False, header=False)
print(f"Data has been written to {res_file}")
