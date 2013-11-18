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
    next = 0  # hack: we must begin with a black pixel.
    while True:
        while current == next:
            count += 1
            next = source.next()
        yield count
        current = next
        count = 0

def trim_rle(source):
    """ This generator ensures the rle string does not exceed 0xff. """

    for ch in source:
        while ch > 0xff:
            # Can't exceed this number.
            yield 0xff
            yield 0
            ch -= ch
        yield ch
            
def generate_rl2(source):
    """ This generator yields a sequence of run lengths of run
    lengths.  The expectation is that the input will be a series of
    positive numbers, with several sequences of 1.  The sequences
    of 1 are replaced by negative run-counts. """

    count = 0
    next = source.next()
    while True:
        if next == 1:
            while next == 1:
                count += 1
                next = source.next()
            if count == 1:
                yield 1
            else:
                yield -count
            count = 0
        else:
            yield next
            next = source.next()

def count_bits(num):
    count = 0
    while num >= (1 << count):
        count += 1
    return count

def chop1_rle(source):
    """ Separates the rle sequence into a sequence of one-bit sequences. """
    result = ''
    for v in source:
        numBits = count_bits(v)
        for z in range(numBits - 1):
            yield 0
        for x in range(numBits):
            b = (v >> (numBits - x - 1)) & 0x1
            assert b != 0 or x != 0
            yield b

def pack1_rle(source):
    """ Packs a sequence of one-bit sequences into a byte string. """
    seq = list(source)
    seq += [0, 0, 0, 0, 0, 0, 0]
    result = ''
    for i in range(0, len(seq) - 7, 8):
        v = (seq[i + 0] << 7) | (seq[i + 1] << 6) | (seq[i + 2] << 5) | (seq[i +3] << 4) | (seq[i + 4] << 3) | (seq[i + 5] << 2) | (seq[i + 6] << 1) | (seq[i + 7])
        result += (chr(v))
    return result

class Rl2Unpacker:
    """ This class reverses chop1_rle() and pack1_rle()--it reads a
    string and returns the original rle sequence of positive integers.
    It's written using a class and a call interface instead of as a
    generator, so it can serve as a prototype for the C code to do the
    same thing. """

    def __init__(self, str):
        self.str = str
        self.si = 0
        self.bi = 8

    def getList(self):
        result = []
        v = self.getNextValue()
        while v != 0:
            result.append(v)
            v = self.getNextValue()
        return result

    def getNextValue(self):
        """ Returns the next value in the sequence.  Returns 0 at the
        end of the sequence. """

        if self.si >= len(self.str):
            return 0
        
        # First, count the number of 0 bits until we come to a one bit.
        bitCount = 1
        b = ord(self.str[self.si])
        bv = b & (1 << (self.bi - 1))
        while bv == 0:
            bitCount += 1
            self.bi -= 1
            if self.bi <= 0:
                self.si += 1
                self.bi = 8
                if self.si >= len(self.str):
                    return 0
                
                b = ord(self.str[self.si])
            bv = b & (1 << (self.bi - 1))

        # OK, now we need to extract the next bitCount bits into a word.
        result = 0
        while bitCount >= self.bi:
            mask = (1 << self.bi) - 1
            value = (b & mask)
            result = (result << self.bi) | value
            bitCount -= self.bi

            self.si += 1
            self.bi = 8
            if self.si >= len(self.str):
                b = 0
                break

            b = ord(self.str[self.si])

        if bitCount > 0:
            # A partial word in the middle.
            bottomCount = self.bi - bitCount
            assert bottomCount > 0
            mask = ((1 << bitCount) - 1)
            value = ((b >> bottomCount) & mask)
            result = (result << bitCount) | value
            self.bi -= bitCount

        return result
            
            

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

    result = list(trim_rle(generate_rle(generate_pixels(image, stride))))
    assert max(result) <= 0xff

    basename = os.path.splitext(filename)[0]
    rleFilename = basename + '.rle'
    rle = open(rleFilename, 'wb')
    rle.write('%c%c%c' % (w, h, stride))
    rle.write(''.join(map(chr, result)))
    rle.close()
    
    print '%s: %s vs. %s' % (rleFilename, 3 + len(result), w * h / 8)

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

    result1 = list(trim_rle(generate_rle(generate_pixels(image, stride))))
    assert max(result1) <= 0xff

    result = pack1_rle(chop1_rle(generate_rle(generate_pixels(image, stride))))

    # Verify the result matches.
    unpacker = Rl2Unpacker(result)
    verify = unpacker.getList()
    result0 = list(generate_rle(generate_pixels(image, stride)))
    assert verify == result0

    basename = os.path.splitext(filename)[0]
    rl2Filename = basename + '.rl2'
    rl2 = open(rl2Filename, 'wb')
    rl2.write('%c%c%c' % (w, h, stride))
    rl2.write(result)
    rl2.close()
    
    print '%s: %s vs. %s vs. %s' % (rl2Filename, 3 + len(result), 3 + len(result1), w * h / 8)
    

# Main.
useRle2 = False
try:
    opts, args = getopt.getopt(sys.argv[1:], '2h')
except getopt.error, msg:
    usage(1, msg)

for opt, arg in opts:
    if opt == '-2':
        useRle2 = True
    elif opt == '-h':
        usage(0)

print args
for filename in args:
    if useRle2:
        make_rl2(filename)
    else:
        make_rle(filename)
