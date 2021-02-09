#!/usr/bin/env python3
import os
import re
import sys
import datetime

year = datetime.datetime.now().year
root_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

for root, _, files in os.walk(root_path):
    for filename in files:
        file_path = os.path.join(root, filename)

        with open(f_name, 'r') as f:
            file_text = f.read()

        file_text = re.sub(r"2008-[0-9]{4} HPDCS Group", 
                           f"2008-{year} HPDCS Group", file_text)

        with open(f_name, 'w') as f:
            f.write(file_text)

