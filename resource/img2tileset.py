#!/usr/bin/env python3

from PIL import Image
import argparse

parser = argparse.ArgumentParser(description="Convert indexed image formats to raw tile data.")
parser.add_argument("-s", "--size", choices=["8x8", "8x16", "16x8", "16x16", "32x32", "320x240"], default="8x8", help="Size of individual tiles. Defaults to 8x8.")
parser.add_argument("input", help="Input file.")
parser.add_argument("output", help="Output file.")

args = parser.parse_args()

tile_x_size, tile_y_size = args.size.split("x")
tile_x_size, tile_y_size = int(tile_x_size), int(tile_y_size)
bpp = 4

if args.input == args.output:
    print("error: Input file can't be the same as the output")
    exit(1)

try:
    img = Image.open(args.input)
except:
    print("Failed to open input image.")
    exit(1)

if img.mode != "P":
    print("Input image must be in an indexed format.")
    exit(1)

packbytes = (8//bpp)
with open(args.output, mode="wb") as output:
    for y in range(0, img.size[1] // tile_y_size):
        y *= tile_y_size
        for x in range(0, img.size[0] // tile_x_size):
            x *= tile_y_size
            tile = img.crop((x, y, x + tile_x_size, y + tile_y_size))
            data=bytearray(tile.getdata())
            for i in range(0,tile_x_size * tile_y_size * bpp // 8):
                mybyte = 0x00;
                for j in range(0, packbytes):
                    srcbyte=data[(i*(8//bpp))+j] & (2**bpp-1)
                    mybyte=mybyte | (srcbyte << (bpp*(packbytes-j-1)))
                output.write( mybyte.to_bytes(15//8,'little') )

