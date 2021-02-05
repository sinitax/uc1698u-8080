import sys, cv2, os

lcd_size = (160, 160)
level_width = 256 / 32

def adjustImageDepth(img):
    img = cv2.resize(img, lcd_size)
    for x in range(lcd_size[0]):
        for y in range(lcd_size[1]):
            img[y,x] = int(img[y,x] / level_width) * level_width
    return img

def convertImagetoCode(img):
    code = "const uint8_t img[] PROGMEM = { "
    for y in range(lcd_size[1]):
        for x in range(lcd_size[0]):
            # invert since LOW is black (LED ON) and HIGH is white (LED OFF)
            code += str(31 - int(img[y,x] / level_width)) + ", "
    code += "};\n"
    return code

if len(sys.argv) < 2:
    print("Usage: convert.py <image>")
    sys.exit(1)

if not os.path.exists("convert.py"):
    print("Run from same directory as script!")
    sys.exit(1)

img = cv2.imread(sys.argv[1], cv2.IMREAD_GRAYSCALE)
img = cv2.normalize(img, None, 0, 255, cv2.NORM_MINMAX)
img = adjustImageDepth(img)
reppath = "out/" + os.path.splitext(os.path.basename(sys.argv[1]))[0] + ".new.bmp"
saved = cv2.imwrite(reppath, img)
with open("out/img.h", "w+") as f:
    f.write(convertImagetoCode(img))

print("Done! The output image was saved to out/img.h.")
if saved:
    print("A representation of what the image should look like was saved to {}".format(reppath))
else:
    print("Failed to save a representation of what the converted image to {}".format(reppath))
