import PIL.Image
import sys

# First, define an image with a 64-color palette that corresponds
# exactly to the Pebble Time's color space.
pal = PIL.Image.new('RGB', (8, 8))
for yi in range(8):
    for xi in range(8):
        pi = yi * 8 + xi
        r = pi & 3
        g = (pi >> 2) & 3
        b = (pi >> 4) & 3
        pal.putpixel((xi, yi), (r * 0x55, b * 0x55, g * 0x55))
pal = pal.convert("P", palette = PIL.Image.ADAPTIVE)
#pal.save('palette_64.png')

def make_palette(filename):
    im = PIL.Image.open(filename)
    if im.mode != 'RGB':
        print "skipping"
        return

    # First, convert it to the 64-color Pebble Time palette.
    im2 = im.quantize(palette = pal)

    # Then, find the 16 most common colors in the resulting image.
    palette = im2.getpalette()
    colors = sorted(im2.getcolors(), reverse = True)

    # And build a palette with just those 16 colors.
    pal2 = []
    for count, index in colors[:16]:
        index *= 3
        pal2 += palette[index : index + 3]
    while len(pal2) < 768:
        pal2 += palette[index : index + 3]
    im2.putpalette(pal2)

    # Now convert it again to that palette.
    im = im.quantize(palette = im2)
    im.save('palette/' + filename)

for filename in sys.argv[1:]:
    print filename
    make_palette(filename)
