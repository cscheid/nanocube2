#!/usr/bin/env python

import csv
import os
import sys
import io
import gzip

reader = csv.reader(io.TextIOWrapper(gzip.open(sys.argv[1], "r"), newline="", write_through=True))

columns = next(reader)

n = int(sys.argv[2])
for row in reader:
    if len(row) != len(columns):
        continue
    pickup = row[1]
    (hour, minute, _) = pickup.split()[1].split(":")
    hour = int(hour)
    minute = int(minute)
    print(hour, minute)
    n -= 1
    if n == 0:
        break
