#!/usr/bin/python3

import subprocess
import sys
import os
import signal
import time

nvsmi_interval_ms = 100
nvsmi_cmd = f"exec nvidia-smi --query-gpu=timestamp,power.draw --format=csv -lms {nvsmi_interval_ms}"
nvsmi_proc = subprocess.Popen(nvsmi_cmd, shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

perf_cmd = "/usr/bin/perf stat -a -e power/energy-pkg/ -x, -- "
start_s = time.time()
perf_proc = subprocess.run(perf_cmd + " ".join(sys.argv[1:]), shell=True, text=True, check=True, stderr=subprocess.PIPE)
duration_s = time.time() - start_s

energy_cpu_j = perf_proc.stderr.splitlines()[-1].split(",")[0]


nvsmi_proc.stdout.flush()
nvsmi_proc.kill()

energy_gpu_j = 0
lines = nvsmi_proc.stdout.readlines()[1:]
for line in lines:
  try:
    energy_gpu_j += float(line.split()[2]) / (1e3 / nvsmi_interval_ms)
  except:
    print(line)

print()
print("GPU: duration_s,energy_gpu_j,energy_cpu_j")
print(f"{duration_s:.4f},{energy_gpu_j},{energy_cpu_j}")
