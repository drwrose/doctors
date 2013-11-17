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

def generate_pixels(image, stride):
    """ This generator yields a sequence of 0/255 values for the pixels
    of the image.  We extend the row to stride * 8 pixels. """
    
    w, h = image.size
    for y in range(h):
        for x in range(w):
            value = image.getpixel((x, y))
            yield value
        for x in range(w, stride * 8):
            # Pad out the row with zeroes.
            yield 0

    raise StopIteration

def generate_rle(source):
    """ This generator yields a sequence of run lengths of a binary
    input--the input is either 0 or 255, so the rle is a simple sequence
    of positive numbers representing alternate values, and explicit
    values are not necessary. """

    current = 0
    count = 0
    next = source.next()
    while True:
        while current == next:
            count += 1
            if count > 0xff:
                # Can't exceed this number.
                yield 0xff
                yield 0
                count -= 0xff
            next = source.next()
        yield count
        current = next
        count = 0

def generate_rl2(source):
    """ This generator yields a sequence of run lengths of run
    lengths.  The expectation is that the input will be a series of
    nonnegative numbers, with several sequences of 1.  The sequences
    of 1 are replaced by negative run-counts. """

    count = 0
    next = source.next()
    while True:
        if next == 1:
            while next == 1:
                count += 1
                if count > 0x80:
                    # Can't exceed this number.
                    yield -0x80
                    count -= 0x80
                next = source.next()
            yield -count
            count = 0
        else:
            if next > 0x7f:
                yield 0x7f
                yield 0
                yield next - 0x7f
            else:
                yield next
            next = source.next()

def make_rle(filename):
    image = PIL.Image.open(filename)
    image = image.convert('1')
    w, h = image.size
    assert w <= 0xff and h <= 0xff
    assert w % 8 == 0  # must be a multiple of 8 pixels wide.

    # The number of bytes in a row.  Must be a multiple of 4, per
    # Pebble conventions.
    stride = (w + 7) / 8
    stride = 4 * ((stride + 3) / 4)
    assert stride <= 0xff

    result = list(generate_rle(generate_pixels(image, stride)))
    print result
    assert max(result) <= 0xff

    basename = os.path.splitext(filename)[0]
    rleFilename = basename + '.rle'
    rle = open(rleFilename, 'wb')
    rle.write('%c%c%c' % (w, h, stride))
    rle.write(''.join(map(chr, result)))
    rle.close()
    
    print '%s: %s vs. %s' % (rleFilename, 2 + len(result), w * h / 8)
    

def make_rl2(filename):
    image = PIL.Image.open(filename)
    image = image.convert('1')
    w, h = image.size
    assert w <= 0xff and h <= 0xff
    assert w % 8 == 0  # must be a multiple of 8 pixels wide.

    # The number of bytes in a row.  Must be a multiple of 4, per
    # Pebble conventions.
    stride = (w + 7) / 8
    stride = 4 * ((stride + 3) / 4)
    assert stride <= 0xff

    result = list(generate_rl2(generate_rle(generate_pixels(image, stride))))
    print result
    assert max(result) <= 0xff

    basename = os.path.splitext(filename)[0]
    rl2Filename = basename + '.rl2'
    rl2 = open(rl2Filename, 'wb')
    rl2.write('%c%c%c' % (w, h, stride))
    rl2.write(''.join(map(chr, result)))
    rl2.close()
    
    print '%s: %s vs. %s' % (rl2Filename, 2 + len(result), w * h / 8)
    

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
    make_rl2(filename)
