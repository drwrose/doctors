#! /usr/bin/env python

import PIL.Image
import PIL.ImageChops
import sys
import os
import getopt
from resources.make_rle import make_rle

help = """
setup.py

This script pre-populates the appinfo.json file and resources
directory for correctly building the 12 Doctors watchface.

setup.py [opts]

Options:

    -s slices
        Specifies the number of vertical slices of each face.

    -p platform[,platform...]
        Specifies the build platform (aplite, basalt, and/or chalk).

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

doctorsImage = """
      {
        "name": "%(resource_base)s",
        "file": "%(filename)s",
        "type": "%(ptype)s"
      },
"""

def enquoteStrings(strings):
    """ Accepts a list of strings, returns a list of strings with
    embedded quotation marks. """
    quoted = []
    for str in strings:
        quoted.append('"%s"' % (str))
    return quoted

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
            if mod == '~bw' and 'aplite' not in targetPlatforms:
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
                # Also make slices of the mod (color) variants.
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

            if numSlices != 1:
                # Make slices.
                filename = 'slices/%s_%s_of_%s.png' % (basename, slice + 1, numSlices)
            else:
                # Special case--no slicing needed; just use the
                # original full-sized image.
                filename = '%s.png' % (basename)
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

    resourceIn = open('%s/appinfo.json.in' % (rootDir), 'r').read()
    resource = open('%s/appinfo.json' % (rootDir), 'w')

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
    targetPlatforms = [ "aplite", "basalt", "chalk" ]
    
configWatch()
