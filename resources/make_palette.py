import PIL.Image
import sys

pal = PIL.Image.new('RGB', (8, 8))
for yi in range(8):
    for xi in range(8):
        pi = yi * 8 + xi
        r = pi & 3
        g = (pi >> 2) & 3
        b = (pi >> 4) & 3
        pal.putpixel((xi, yi), (r * 0x55, b * 0x55, g * 0x55))
pal = pal.convert("P", palette = PIL.Image.ADAPTIVE)
pal.save('pal.png')

def make_palette(filename):
    im = PIL.Image.open(filename)

    im2 = im.quantize(palette = pal)
    palette = im2.getpalette()
    colors = sorted(im2.getcolors(), reverse = True)
    pal2 = []
    for count, index in colors[:16]:
        index *= 3
        pal2 += palette[index : index + 3]
    while len(pal2) < 768:
        pal2 += palette[index : index + 3]
    im2.putpalette(pal2)

    im = im.convert("P", palette = im2)
    im.save('t.png')

for filename in sys.argv[1:]:
    print filename
    make_palette(filename)
