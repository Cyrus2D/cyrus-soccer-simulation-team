# you should feed a png file with this size : 256 * 64

import matplotlib.pyplot as plt
import matplotlib.image as mpimg

img=mpimg.imread('/home/nader/Downloads/cyrus_logo.png')

for y in img:
    for x in y:
        x[0] = int(int(x[0] * 255) / 32) * 32
        x[1] = int(int(x[1] * 255) / 32) * 32
        x[2] = int(int(x[2] * 255) / 32) * 32
        x[0] = x[0] / 256
        x[1] = x[1] / 256
        x[2] = x[2] / 256
        x[3] = 1
plt.imshow(img)
plt.show()

for y in img:
    for x in y:
        x[0] = int(int(x[0] * 255) / 32 + 1) * 32
        x[1] = int(int(x[1] * 255) / 32 + 1) * 32
        x[2] = int(int(x[2] * 255) / 32 + 1) * 32
        x[3] = 1
        print(x)

colors = {}
import string
chars = string.printable
chars = chars.replace('\"', '')
chars = chars.replace('\'', '')
print(chars)
number = 0
new_image = []
for y in img:
    new_image.append('')
    for x in y:
        color = '#%02x%02x%02x' % (int(x[0]), int(x[1]), int(x[2]))
        if not color in colors.keys():
            colors[color] = chars[number]
            number += 1

        new_image[-1] += str(colors[color])

for y in new_image:
    print(y)

xmp = []
xmp.append('/* XPM */')
xmp.append('static char *team_logo_xpm[] = {')
xmp.append('/* columns rows colors chars-per-pixel */')
xmp.append(f'\"256 64 {len(colors)} 1 \",')
for c in colors.keys():
    xmp.append(f'\"{colors[c]} c {c}\",')
xmp.append('/* pixels */')
for y in new_image:
    xmp.append(f'\"{y}\",')
xmp.append("};")

file = open('new_xmp.xpm', 'w')
for l in xmp:
    file.write(l + '\n')
file.close()