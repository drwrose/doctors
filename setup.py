#! /usr/bin/env python

import PIL.Image
import PIL.ImageChops
import sys
import os
import getopt
from resources.make_rle import make_rle
from resources.peb_platform import getPlatformShape, getPlatformColor, getPlatformFilename, screenSizes

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
        Perform RLE compression of images.

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

# [fill_rect, bar_rect, (font, vshift)] where rect is (x, y, w, h)
batteryGaugeSizes = {
    'rect' : [(6, 0, 18, 10), (10, 3, 10, 4), ('GOTHIC_14', -4)],
    'round' : [(6, 0, 18, 10), (10, 3, 10, 4), ('GOTHIC_14', -4)],
    'emery' : [(8, 0, 25, 14), (13, 3, 15, 8), ('GOTHIC_18', -5)],
    }

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

def makeDoctors(platform):
    """ Makes the resource string for the list of doctors images. """

    basenames = ['twelve', 'one', 'two', 'three', 'four', 'five', 'six',
                 'seven', 'eight', 'nine', 'ten', 'eleven', 'hurt']

    shape = getPlatformShape(platform)
    screenWidth, screenHeight = screenSizes[shape]

    slicePoints = [0]
    for slice in range(numSlices):
        next = (slice + 1) * screenWidth / numSlices
        slicePoints.append(next)

    buildDir = '%s/build' % (resourcesDir)
    if not os.path.isdir(buildDir):
        os.mkdir(buildDir)

    doctorsImages = ''
    doctorsIds = ''
    already_basenames = set()
    for basename in basenames:
        doctorsIds += '{\n'

        sourceFilename = getPlatformFilename('%s/%s.png' % (resourcesDir, basename), platform)
        ## sourceFilename = '%s/%s.png' % (resourcesDir, basename)
        source = PIL.Image.open(sourceFilename)
        assert source.size == (screenWidth, screenHeight)

        for slice in range(numSlices):
            # Make a vertical slice of the image.
            resource_base = '%s_%s' % (basename.upper(), slice + 1)

            if numSlices != 1:
                # Make slices.
                xf = slicePoints[slice]
                xt = slicePoints[slice + 1]
                box = (xf, 0, xt, screenHeight)

                image = source.crop(box)
                filename = 'build/%s_%s_of_%s_%s.png' % (basename, slice + 1, numSlices, platform)
                image.save('%s/%s' % (resourcesDir, filename))

            else:
                # Special case--no slicing needed; just use the
                # original full-sized image.  However, we still copy
                # it into the build folder, because the ~color~round
                # version (for Chalk) will need to get circularized.
                if shape == 'round':
                    image = circularizeImage(source)
                else:
                    image = source

                filename = 'build/%s_%s.png' % (basename, platform)
                image.save('%s/%s' % (resourcesDir, filename))

            if basename not in already_basenames:
                already_basenames.add(basename)
                doctorsImages += make_rle(filename, name = resource_base, useRle = supportRle, platforms = [platform])

            doctorsIds += 'RESOURCE_ID_%s, ' % (resource_base)
        doctorsIds += '},\n'

    return slicePoints, doctorsImages, doctorsIds

def configWatch():
    configH = open('%s/generated_config.h' % (resourcesDir), 'w')
    configIn = open('%s/generated_config.h.in' % (resourcesDir), 'r').read()
    print >> configH, configIn % {
        'numSlices' : numSlices,
        'supportRle' : int(supportRle),
        'compileDebugging' : int(compileDebugging),
        }

    configC = open('%s/generated_config.c' % (resourcesDir), 'w')
    configIn = open('%s/generated_config.c.in' % (resourcesDir), 'r').read()
    print >> configC, configIn % {
        }

    resourceStr = ''
    for platform in targetPlatforms:
        shape = getPlatformShape(platform)
        color = getPlatformColor(platform)
        screenWidth, screenHeight = screenSizes[shape]

        slicePoints, doctorsImages, doctorsIds = makeDoctors(platform)
        resourceStr += doctorsImages

        for ti in [1, 2, 3, 4]:
            filename = 'tardis_%02d.png' % (ti)
            name = 'TARDIS_%02d' % (ti)
            resourceStr += make_rle(filename, name = name, useRle = False, platforms = [platform], compress = False, requirePalette = False)

        for basename in ['dalek', 'k9']:
            filename = '%s.png' % (basename)
            name = basename.upper()
            resourceStr += make_rle(filename, name = name, useRle = supportRle, platforms = [platform], requirePalette = True)

        if color == 'bw':
            for basename in ['tardis', 'dalek', 'k9']:
                filename = '%s_mask.png' % (basename)
                name = '%s_MASK' % (basename.upper())
                resourceStr += make_rle(filename, name = name, useRle = supportRle, platforms = [platform])

        bluetoothFilename = getPlatformFilename('resources/bluetooth_connected.png', platform)
        im = PIL.Image.open(bluetoothFilename)
        bluetoothSizes[platform] = im.size

        resourceStr += make_rle('bluetooth_connected.png', name = 'BLUETOOTH_CONNECTED', useRle = supportRle, platforms = [platform])
        resourceStr += make_rle('bluetooth_disconnected.png', name = 'BLUETOOTH_DISCONNECTED', useRle = supportRle, platforms = [platform])
        if color == 'bw':
            resourceStr += make_rle('bluetooth_mask.png', name = 'BLUETOOTH_MASK', useRle = supportRle, platforms = [platform])

        resourceStr += make_rle('battery_gauge_empty.png', name = 'BATTERY_GAUGE_EMPTY', useRle = supportRle, platforms = [platform])
        resourceStr += make_rle('battery_gauge_charged.png', name = 'BATTERY_GAUGE_CHARGED', useRle = supportRle, platforms = [platform])
        resourceStr += make_rle('charging.png', name = 'CHARGING', useRle = supportRle, platforms = [platform])
        if color == 'bw':
            resourceStr += make_rle('battery_gauge_mask.png', name = 'BATTERY_GAUGE_MASK', useRle = supportRle, platforms = [platform])
            resourceStr += make_rle('charging_mask.png', name = 'CHARGING_MASK', useRle = supportRle, platforms = [platform])

        configIn = open('%s/generated_config.h.per_platform_in' % (resourcesDir), 'r').read()
        print >> configH, configIn % {
            'platformUpper' : platform.upper(),
            'screenWidth' : screenWidth,
            'screenHeight' : screenHeight,
            'batteryGaugeFillX' : batteryGaugeSizes[shape][0][0],
            'batteryGaugeFillY' : batteryGaugeSizes[shape][0][1],
            'batteryGaugeFillW' : batteryGaugeSizes[shape][0][2],
            'batteryGaugeFillH' : batteryGaugeSizes[shape][0][3],
            'batteryGaugeBarX' : batteryGaugeSizes[shape][1][0],
            'batteryGaugeBarY' : batteryGaugeSizes[shape][1][1],
            'batteryGaugeBarW' : batteryGaugeSizes[shape][1][2],
            'batteryGaugeBarH' : batteryGaugeSizes[shape][1][3],
            'batteryGaugeFont' : batteryGaugeSizes[shape][2][0],
            'batteryGaugeVshift' : batteryGaugeSizes[shape][2][1],
            'bluetoothSizeX' : bluetoothSizes[platform][0],
            'bluetoothSizeY' : bluetoothSizes[platform][1],
            }

        configIn = open('%s/generated_config.c.per_platform_in' % (resourcesDir), 'r').read()
        print >> configC, configIn % {
            'platformUpper' : platform.upper(),
            'doctorsIds' : doctorsIds,
            'slicePoints' : ', '.join(map(str, slicePoints)),
            }


    resourceStr += make_rle('mins_background.png', name = 'MINS_BACKGROUND', useRle = supportRle, platforms = targetPlatforms, color = 'bw')
    resourceStr += make_rle('hours_background.png', name = 'HOURS_BACKGROUND', useRle = supportRle, platforms = targetPlatforms, color = 'bw')
    resourceStr += make_rle('date_background.png', name = 'DATE_BACKGROUND', useRle = supportRle, platforms = targetPlatforms, color = 'bw')

    resourceIn = open('%s/package.json.in' % (rootDir), 'r').read()
    resource = open('%s/package.json' % (rootDir), 'w')

    print >> resource, resourceIn % {
        'targetPlatforms' : ', '.join(enquoteStrings(targetPlatforms)),
        'resourceStr' : resourceStr,
        }


# Main.
try:
    opts, args = getopt.getopt(sys.argv[1:], 's:p:dxh')
except getopt.error, msg:
    usage(1, msg)

numSlices = 1
compileDebugging = False
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
        supportRle = True
    elif opt == '-h':
        usage(0)

if not targetPlatforms:
    targetPlatforms = [ 'aplite', 'basalt', 'chalk', 'diorite', 'emery' ]

bluetoothSizes = {} # filled in by configWatch()

configWatch()
