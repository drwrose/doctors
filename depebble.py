import sys, zipfile, json, cStringIO
sys.path.append('/Users/drose/pebble-dev/PebbleSDK-2.2/Pebble/tools')
import pbpack

#pbwFilename = '/Users/drose/Downloads/doctors (1).pbw'
#pbwFilename = '/Users/drose/Downloads/Tiny_Doctors.pbw'
#pbwFilename = '/Users/drose/src/pebble/stack/build/stack.pbw'
pbwFilename = sys.argv[1]
assert pbwFilename

platforms = [ 'aplite', 'basalt' ]
subdir_prefixes = { 'aplite' : '', 'basalt' : 'basalt/' }

pbw = zipfile.ZipFile(pbwFilename, 'r')

appinfo = pbw.open('appinfo.json', 'r')
aid = json.load(appinfo)
mlist = aid['resources']['media']

for platform in platforms:
    prefix = subdir_prefixes[platform]
    resourcesData = pbw.read(prefix + 'app_resources.pbpack', 'r')
    resources = cStringIO.StringIO(resourcesData)
    rpack = pbpack.ResourcePack()

    rpd = rpack.deserialize(resources)
    print rpd

    mi = 0
    ri = 0
    while mi < len(mlist) and ri < rpd.num_files:
        name = mlist[mi]['name']
        filename = mlist[mi]['file']
        type = mlist[mi]['type']
        numElements = 1
        if type == 'png-trans':
            numElements += 1
        mi += 1
        data = []
        for i in range(numElements):
            data.append(rpd.contents[ri])
            ri += 1

        print '%s, %s, %s: %s' % (platform, name, filename, map(len, data))

    print '%s, %s, %s' % (platform, mi, ri)


