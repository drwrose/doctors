#! /usr/bin/env python

import PIL.Image
import PIL.ImageChops
import sys
import os
import getopt
from resources.make_rle import make_rle

help = """
setup.py

This script pre-populates the package.json file and resources
directory for correctly building the 12 Doctors watchface.

setup.py [opts]

Options:

    -s slices
        Specifies the number of vertical slices of each face.

    -p platform[,platform...]
        Specifies the build platform (aplite, basalt, chalk, diorite).

    -x
        Perform no RLE compression of images.

    -d
        Compile for debugging.  Specifically this enables "fast time",
        so the faces change quickly.  It also enables logging.

"""

def usage(code, msg = ''):
    print >> sys.stderr, help
    print >> sys.stderr, msg
    sys.exit(code)

# Attempt to determine the directory in which we're operating.
rootDir = os.path.dirname(__file__) or '.'
resourcesDir = os.path.join(rootDir, 'resources')

# These are the min_x and min_y pairs for each of the 180 rows of
# GBitmapFormat8BitCircular-format images in Chalk.
circularBufferSize = [
    (76, 103),
    (71, 108),
    (66, 113),
    (63, 116),
    (60, 119),
    (57, 122),
    (55, 124),
    (52, 127),
    (50, 129),
    (48, 131),
    (46, 133),
    (45, 134),
    (43, 136),
    (41, 138),
    (40, 139),
    (38, 141),
    (37, 142),
    (36, 143),
    (34, 145),
    (33, 146),
    (32, 147),
    (31, 148),
    (29, 150),
    (28, 151),
    (27, 152),
    (26, 153),
    (25, 154),
    (24, 155),
    (23, 156),
    (22, 157),
    (22, 157),
    (21, 158),
    (20, 159),
    (19, 160),
    (18, 161),
    (18, 161),
    (17, 162),
    (16, 163),
    (15, 164),
    (15, 164),
    (14, 165),
    (13, 166),
    (13, 166),
    (12, 167),
    (12, 167),
    (11, 168),
    (10, 169),
    (10, 169),
    (9, 170),
    (9, 170),
    (8, 171),
    (8, 171),
    (7, 172),
    (7, 172),
    (7, 172),
    (6, 173),
    (6, 173),
    (5, 174),
    (5, 174),
    (5, 174),
    (4, 175),
    (4, 175),
    (4, 175),
    (3, 176),
    (3, 176),
    (3, 176),
    (2, 177),
    (2, 177),
    (2, 177),
    (2, 177),
    (2, 177),
    (1, 178),
    (1, 178),
    (1, 178),
    (1, 178),
    (1, 178),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (0, 179),
    (1, 178),
    (1, 178),
    (1, 178),
    (1, 178),
    (1, 178),
    (2, 177),
    (2, 177),
    (2, 177),
    (2, 177),
    (2, 177),
    (3, 176),
    (3, 176),
    (3, 176),
    (4, 175),
    (4, 175),
    (4, 175),
    (5, 174),
    (5, 174),
    (5, 174),
    (6, 173),
    (6, 173),
    (7, 172),
    (7, 172),
    (7, 172),
    (8, 171),
    (8, 171),
    (9, 170),
    (9, 170),
    (10, 169),
    (10, 169),
    (11, 168),
    (12, 167),
    (12, 167),
    (13, 166),
    (13, 166),
    (14, 165),
    (15, 164),
    (15, 164),
    (16, 163),
    (17, 162),
    (18, 161),
    (18, 161),
    (19, 160),
    (20, 159),
    (21, 158),
    (22, 157),
    (22, 157),
    (23, 156),
    (24, 155),
    (25, 154),
    (26, 153),
    (27, 152),
    (28, 151),
    (29, 150),
    (31, 148),
    (32, 147),
    (33, 146),
    (34, 145),
    (36, 143),
    (37, 142),
    (38, 141),
    (40, 139),
    (41, 138),
    (43, 136),
    (45, 134),
    (46, 133),
    (48, 131),
    (50, 129),
    (52, 127),
    (55, 124),
    (57, 122),
    (60, 119),
    (63, 116),
    (66, 113),
    (71, 108),
    (76, 103),
    ]

doctorsImage = """
        {
          "targetPlatforms": [ "aplite", "diorite" ],
          "type": "%(ptype)s",
          "memoryFormat" : "1Bit",
          "spaceOptimization" : "memory",
          "name": "%(resource_base)s",
          "file": "%(filename)s"
        },
        {
          "targetPlatforms": [ "basalt", "chalk" ],
          "type": "%(ptype)s",
          "name": "%(resource_base)s",
          "file": "%(filename)s"
        },"""

def enquoteStrings(strings):
    """ Accepts a list of strings, returns a list of strings with
    embedded quotation marks. """
    quoted = []
    for str in strings:
        quoted.append('"%s"' % (str))
    return quoted

def circularizeImage(source):
    # Apply a circular crop to a 180x180 image, constructing a
    # GBitmapFormat4BitPaletteCircular image.  This will leave us
    # 25792 pixels, which we store consecutively into a 208x124 pixel
    # image.
    assert source.size == (180, 180)
    dest_size = (208, 124)

    dest = PIL.Image.new(source.mode, dest_size)
    if source.mode in ['P', 'L']:
        dest.putpalette(source.getpalette())

    dy = 0
    dx = 0
    for sy in range(180):
        min_x, max_x = circularBufferSize[sy]
        for sx in range(min_x, max_x + 1):
            dest.putpixel((dx, dy), source.getpixel((sx, sy)))
            dx += 1
            if dx >= dest_size[0]:
                dx = 0
                dy += 1
    assert dy == dest_size[1] and dx == 0
    return dest

def makeDoctors():
    """ Makes the resource string for the list of doctors images. """

    basenames = ['twelve', 'one', 'two', 'three', 'four', 'five', 'six',
                 'seven', 'eight', 'nine', 'ten', 'eleven', 'hurt']

    slicePoints = {}
    for screenWidth in [144, 180]:
        slicePoints[screenWidth] = [0]
        for slice in range(numSlices):
            next = (slice + 1) * screenWidth / numSlices
            slicePoints[screenWidth].append(next)

    slicesDir = '%s/slices' % (resourcesDir)
    if not os.path.isdir(slicesDir):
        os.mkdir(slicesDir)

    doctorsImages = ''
    doctorsIds = ''
    for basename in basenames:
        doctorsIds += '{\n'
        ## sourceFilename = '%s/%s.png' % (resourcesDir, basename)
        ## source = PIL.Image.open(sourceFilename)
        ## assert source.size == (screenWidth, screenHeight)

        mods = {}
        for mod in [ '~bw', '~color~rect', '~color~round' ]:
            if mod == '~bw' and 'aplite' not in targetPlatforms and 'diorite' not in targetPlatforms:
                continue
            if mod == '~color~rect' and 'basalt' not in targetPlatforms:
                continue
            if mod == '~color~round' and 'chalk' not in targetPlatforms:
                continue
            modFilename = '%s/%s%s.png' % (resourcesDir, basename, mod)
            if os.path.exists(modFilename):
                modImage = PIL.Image.open(modFilename)
                mods[mod] = modImage

        for slice in range(numSlices):
            # Make a vertical slice of the image.
            resource_base = '%s_%s' % (basename.upper(), slice + 1)

            if numSlices != 1:
                # Make slices.
                for mod, modImage in mods.items():
                    screenWidth, screenHeight = 144, 172
                    if mod == '~color~round':
                        screenWidth, screenHeight = 180, 180

                    xf = slicePoints[screenWidth][slice]
                    xt = slicePoints[screenWidth][slice + 1]
                    box = (xf, 0, xt, screenHeight)

                    image = modImage.crop(box)
                    filename = 'slices/%s_%s_of_%s%s.png' % (basename, slice + 1, numSlices, mod)
                    image.save('%s/%s' % (resourcesDir, filename))

                filename = 'slices/%s_%s_of_%s.png' % (basename, slice + 1, numSlices)
            else:
                # Special case--no slicing needed; just use the
                # original full-sized image.  However, we still copy
                # it into the slices folder, because the ~color~round
                # version (for Chalk) will need to get circularized.
                for mod, modImage in mods.items():
                    image = modImage
                    if mod == '~color~round':
                        image = circularizeImage(modImage)

                    filename = 'slices/%s%s.png' % (basename, mod)
                    image.save('%s/%s' % (resourcesDir, filename))

                filename = 'slices/%s.png' % (basename)

            rleFilename, ptype = make_rle(filename, useRle = supportRle, modes = mods.keys())

            doctorsImages += doctorsImage % {
            'resource_base' : resource_base,
            'filename' : rleFilename,
            'ptype' : ptype,
            }

            doctorsIds += 'RESOURCE_ID_%s, ' % (resource_base)
        doctorsIds += '},\n'

    return slicePoints, doctorsImages, doctorsIds

def configWatch():
    slicePoints, doctorsImages, doctorsIds = makeDoctors()

    configIn = open('%s/generated_config.h.in' % (resourcesDir), 'r').read()
    config = open('%s/generated_config.h' % (resourcesDir), 'w')
    print >> config, configIn % {
        'doctorsIds' : doctorsIds,
        'numSlices' : numSlices,
        'supportRle' : int(supportRle),
        'compileDebugging' : int(compileDebugging),
        }

    configIn = open('%s/generated_config.c.in' % (resourcesDir), 'r').read()
    config = open('%s/generated_config.c' % (resourcesDir), 'w')
    print >> config, configIn % {
        'doctorsIds' : doctorsIds,
        'slicePointsRound' : ', '.join(map(str, slicePoints[180])),
        'slicePointsRect' : ', '.join(map(str, slicePoints[144])),
        }

    resourceIn = open('%s/package.json.in' % (rootDir), 'r').read()
    resource = open('%s/package.json' % (rootDir), 'w')

    print >> resource, resourceIn % {
        'targetPlatforms' : ', '.join(enquoteStrings(targetPlatforms)),
        'doctorsImages' : doctorsImages,
        }


# Main.
try:
    opts, args = getopt.getopt(sys.argv[1:], 's:p:dxh')
except getopt.error, msg:
    usage(1, msg)

numSlices = 1
compileDebugging = False
#supportRle = True
supportRle = False
targetPlatforms = [ ]
for opt, arg in opts:
    if opt == '-s':
        numSlices = int(arg)
    elif opt == '-p':
        targetPlatforms += arg.split(',')
    elif opt == '-d':
        compileDebugging = True
    elif opt == '-x':
        supportRle = False
    elif opt == '-h':
        usage(0)

if not targetPlatforms:
    targetPlatforms = [ "aplite", "basalt", "chalk", "diorite" ]

configWatch()
