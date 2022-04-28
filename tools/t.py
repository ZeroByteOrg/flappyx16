#!/usr/bin/env python3

import argparse

parser = argparse.ArgumentParser(description="test sandbox")
parser.add_argument ("-i", help="specify one or more input files", action='append', required=True)

args = parser.parse_args()

for i in (args.i):
    print(i);

