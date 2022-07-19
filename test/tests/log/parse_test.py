import re
import sys

if len(sys.argv) != 25:
    exit(-1)

count_regex = r"(\d+)"
float_regex = r"(\d+(?:\.\d+)?)"
measure_regex = f"{float_regex}(?:[munKMGTPEZ]i?)?"

stats_regex_str = f'''TOTAL SIMULATION TIME ..... : {measure_regex}s
TOTAL KERNELS ............. : {count_regex}
TOTAL_THREADS ............. : {count_regex}
TOTAL_LPs ................. : {count_regex}
TOTAL EXECUTED EVENTS ..... : {count_regex}
TOTAL COMMITTED EVENTS..... : {count_regex}
TOTAL REPROCESSED EVENTS... : {count_regex}
TOTAL SILENT EVENTS........ : {count_regex}
TOTAL ROLLBACKS EXECUTED... : {count_regex}
TOTAL ANTIMESSAGES......... : {count_regex}
ROLLBACK FREQUENCY......... : {float_regex}%
ROLLBACK LENGTH............ : {float_regex}
EFFICIENCY................. : {float_regex}%
AVERAGE EVENT COST......... : {measure_regex}s
AVERAGE EVENT COST \\(EMA\\)... : {measure_regex}s
AVERAGE CHECKPOINT COST.... : {measure_regex}s
AVERAGE RECOVERY COST...... : {measure_regex}s
AVERAGE LOGGED STATE SIZE.. : {measure_regex}B
LAST COMMITTED GVT ........ : {float_regex}
NUMBER OF GVT REDUCTIONS... : {count_regex}
SIMULATION TIME SPEED...... : {float_regex}
AVERAGE MEMORY USAGE....... : {measure_regex}B
PEAK MEMORY USAGE.......... : {measure_regex}B
'''

stats_regex = re.compile(stats_regex_str, re.MULTILINE)

with open(sys.argv[1], "r") as f:
    data = f.read()

m = stats_regex.fullmatch(data)
if m is None:
    exit(-1)

for i, arg in enumerate(sys.argv[2:]):
    if arg != 'I' and arg != m[i + 1]:
        print(m[i + 1])
        exit(-1)
