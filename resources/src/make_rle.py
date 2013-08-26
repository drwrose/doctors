#! /usr/bin/env python

import PIL.Image
import sys
import os
import getopt

help = """
make_rle.py

Converts an image from a standard format (for instance, a png) into an
.rle file for loading pre-compressed into a Pebble watch app.

make_rle.py [opts]
        
"""

def usage(code, msg = ''):
    print >> sys.stderr, help
    print >> sys.stderr, msg
    sys.exit(code)


def generate_pixels(image):
    """ This generator yields a sequence of 0/1 values for the pixels
    of the image. """
    
    w, h = image.size
    for y in range(h):
        for x in range(w):
            value = image.getpixel((x, y))
            if value:
                yield 1
            else:
                yield 0
    raise StopIteration

def generate_rle_binary(source):
    """ This generator yields a sequence of run lengths of a binary
    input--the input is either 0 or 1, so the rle is a simple sequence
    of positive numbers, and values are not necessary. """

    current = 0
    count = 0
    next = source.next()
    while True:
        while current == next:
            count += 1
            next = source.next()
        yield count
        current = next
        count = 0

def generate_rle_advanced(source):
    """ This generator yields a sequence of run lengths of a
    non-binary sequence.  The input is a sequence of nonnegative
    integers, so we need to record the actual values as well as the
    run lengths. """

    current = None
    count = 0
    next = source.next()
    while True:
        while current == next:
            count += 1
            next = source.next()
        if count == 1:
            # If there's only a singleton, yield that value.
            yield current
        elif count > 0:
            # If there's a run, yield the negative count, then the value.
            yield -count
            yield current
        current = next
        count = 0

def make_rle(filename):
    image = PIL.Image.open(filename)
    image = image.convert('1')
    w, h = image.size
    assert w <= 0xff and h <= 0xff

    #result = list(generate_rle_advanced(generate_rle_binary(generate_pixels(image))))
    result = list(generate_rle_binary(generate_pixels(image)))
    assert max(result) <= 0xff

    basename = os.path.splitext(filename)[0]
    rleFilename = basename + '.rle'
    rle = open(rleFilename, 'wb')
    rle.write('%c%c' % (w, h))
    rle.write(''.join(map(chr, result)))
    rle.close()
    
    print '%s: %s vs. %s' % (rleFilename, 2 + len(result), w * h / 8)
    

# Main.
try:
    opts, args = getopt.getopt(sys.argv[1:], 'h')
except getopt.error, msg:
    usage(1, msg)

for opt, arg in opts:
    if opt == '-h':
        usage(0)

print args
for filename in args:
    make_rle(filename)
