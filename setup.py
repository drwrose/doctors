#! /usr/bin/env python

import PIL.Image
import PIL.ImageChops
import sys
import os
import getopt

help = """
setup.py

This script pre-populates the appinfo.json file and resources
directory for correctly building the 12 Doctors watchface.

setup.py [opts]

Options:

    -s slices
        Specifies the number of vertical slices of each face.

    -d
        Compile for debugging.  Specifically this enables "fast time",
        so the hands move quickly about the face of the watch.  It
        also enables logging.
        
"""

def usage(code, msg = ''):
    print >> sys.stderr, help
    print >> sys.stderr, msg
    sys.exit(code)

# Attempt to determine the directory in which we're operating.
rootDir = os.path.dirname(__file__) or '.'
resourcesDir = os.path.join(rootDir, 'resources')

screenWidth = 144
screenHeight = 168

doctorsImage = """
      {
        "name": "%(resource_base)s",
        "file": "%(filename)s",
        "type": "png"
      },
"""
        

def makeDoctors():
    """ Makes the resource string for the list of doctors images. """

    basenames = ['twelve', 'one', 'two', 'three', 'four', 'five', 'six',
                 'seven', 'eight', 'nine', 'ten', 'eleven', 'hurt']

    slicePoints = [0]
    for slice in range(numSlices):
        next = (slice + 1) * screenWidth / numSlices
        slicePoints.append(next)

    slicesDir = '%s/slices' % (resourcesDir)
    if not os.path.isdir(slicesDir):
        os.mkdir(slicesDir)
        
    doctorsImages = ''
    doctorsIds = ''
    for basename in basenames:
        doctorsIds += '{\n'
        source = PIL.Image.open('%s/%s.png' % (resourcesDir, basename))
        assert source.size == (screenWidth, screenHeight)
        
        for slice in range(numSlices):
            # Make a vertical slice of the image.
            xf = slicePoints[slice]
            xt = slicePoints[slice + 1]
            box = (xf, 0, xt, screenHeight)

            image = source.crop(box)
            filename = 'slices/%s_%s_of_%s.png' % (basename, slice + 1, numSlices)
            resource_base = '%s_%s' % (basename.upper(), slice + 1)
            image.save('%s/%s' % (resourcesDir, filename))
            
            doctorsImages += doctorsImage % {
            'resource_base' : resource_base,
            'filename' : filename,
            }

            doctorsIds += 'RESOURCE_ID_%s, ' % (resource_base)
        doctorsIds += '},\n'

    return slicePoints, doctorsImages, doctorsIds

def configWatch():
    slicePoints, doctorsImages, doctorsIds = makeDoctors()

    slicePoints = ', '.join(map(str, slicePoints))

    configIn = open('%s/generated_config.h.in' % (resourcesDir), 'r').read()
    config = open('%s/generated_config.h' % (resourcesDir), 'w')
    print >> config, configIn % {
        'doctorsIds' : doctorsIds,
        'numSlices' : numSlices,
        'compileDebugging' : int(compileDebugging),
        }

    configIn = open('%s/generated_config.c.in' % (resourcesDir), 'r').read()
    config = open('%s/generated_config.c' % (resourcesDir), 'w')
    print >> config, configIn % {
        'doctorsIds' : doctorsIds,
        'slicePoints' : slicePoints,
        }

    resourceIn = open('%s/appinfo.json.in' % (rootDir), 'r').read()
    resource = open('%s/appinfo.json' % (rootDir), 'w')

    print >> resource, resourceIn % {
        'doctorsImages' : doctorsImages,
        }


# Main.
try:
    opts, args = getopt.getopt(sys.argv[1:], 's:dh')
except getopt.error, msg:
    usage(1, msg)

numSlices = 3
compileDebugging = False
for opt, arg in opts:
    if opt == '-s':
        numSlices = int(arg)
    elif opt == '-d':
        compileDebugging = True
    elif opt == '-h':
        usage(0)

configWatch()
