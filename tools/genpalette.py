#!/usr/bin/env python3

from PIL import Image
import sys
import struct
import argparse
import numpy as np

def check_range(arg):
    try:
        value = int(arg)
    except ValueError as err:
       raise argparse.ArgumentTypeError(str(err))

    if value < 1 or value > 16:
        message = "Expected 1 <= value <= 16, got value = {}".format(value)
        raise argparse.ArgumentTypeError(message)

    return value

parser = argparse.ArgumentParser(description="Generate 16-color palette from an indexed PNG.")
parser.add_argument("--steps", "-s", help="Number of steps in the color ramp (1..16)",default=1, type=check_range, nargs="?")
parser.add_argument("--color", "-c", help="Final color of the fade-out color ramp (0xRGB)",default=0, nargs="+")
parser.add_argument("--outfile", "-o", help="Output file.")
parser.add_argument("-i", help="Input file. (may specify multiple times)", action='append', required=True)

args, leftovers = parser.parse_known_args()

for c in args.color:
	try:
		targetcolor = int(c, 16)
	except:
		sys.stderr.write("Error: you must specify the color as a 12-bit hex value (" + str(c) + ")\n")
		exit(1)
	if targetcolor > 0xfff or targetcolor < 0:
		sys.stderr.write("Error: your specified color must be from 0x000 to 0xfff (" + hex(targetcolor) + ")\n")
		exit(1)

palette = []
for f in (args.i):
	try:
		img = Image.open(f)
		tmp = img.getpalette()
		for i in range(0,48):
			palette.append(tmp[i]//16)
		img.close()
	except:
		sys.stderr.write("Failed to open input image: ", f)
		exit(1)

if ( len(palette)//3 > 256 ):
	sys.stderr.write("Error: More than 256 colors have been loaded.")
	exit(1)

# palette is now an array of bytes R, G, B, R, G, B, ....
# concatenated from the first 16 colors of the palettes of the images
# specified as -i arguments.
		
# indexed = np.array(im)
try:
	with open(args.outfile, mode="wb") as out:
		out.write((0xa000).to_bytes(2,byteorder='little', signed=False))
		for color in args.color:
			r1 = (int(color,16) & 0xf00) >> 8
			g1 = (int(color,16) & 0x0f0) >> 4
			b1 = (int(color,16) & 0x00f)
			for step in range (0,args.steps+1):
				for i in range(0,len(palette)//3):
					r = palette[3*i]
					g = palette[3*i+1]
					b = palette[3*i+2]
					r += int((r1-r)*step/args.steps)
					g += int((g1-g)*step/args.steps)
					b += int((b1-b)*step/args.steps)
					color = (r << 8) | (g << 4) | b
					out.write((color).to_bytes(2,byteorder='little', signed=False))
except:
	sys.stderr.write("Cannot open output file: ", args.outfile)
