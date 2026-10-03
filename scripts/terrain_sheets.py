"""Renders numbered contact sheets of the terrain pieces (one PNG per 40 pieces of each
MCD set) plus pieces.csv with their properties, for labelling src/Access/TerrainNames.inc.
Usage: python scripts/terrain_sheets.py <output folder>"""
import os, struct, sys, csv
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "bin", "UFO")
OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)

pal_raw = open(os.path.join(ROOT, "GEODATA", "PALETTES.DAT"), "rb").read()
off = 4 * (768 + 6)
pal = [min(255, b * 4) for b in pal_raw[off:off + 768]]

TYPES = "FWNO"  # floor, west wall, north wall, object

def load_pck(name):
    tab = open(os.path.join(ROOT, "TERRAIN", name + ".TAB"), "rb").read()
    pck = open(os.path.join(ROOT, "TERRAIN", name + ".PCK"), "rb").read()
    if struct.unpack("<I", tab[:4])[0] != 0 and len(tab) >= 4:
        offs = list(struct.unpack("<%dH" % (len(tab) // 2), tab))
    else:
        offs = list(struct.unpack("<%dI" % (len(tab) // 4), tab))
    frames = []
    for o in offs:
        px = bytearray(32 * 40)
        i = o
        pos = pck[i] * 32
        i += 1
        while i < len(pck) and pck[i] != 255:
            v = pck[i]
            if v == 254:
                pos += pck[i + 1]
                i += 2
            else:
                if pos < len(px):
                    px[pos] = v
                pos += 1
                i += 1
        frames.append(px)
    return frames

def sprite(px, scale=3):
    img = Image.new("P", (32, 40))
    img.putpalette(pal)
    img.putdata(px)
    rgba = img.convert("RGBA")
    data = [(40, 40, 40, 255) if px[k] == 0 else rgba.getpixel((k % 32, k // 32)) for k in range(32 * 40)]
    rgba.putdata(data)
    return rgba.resize((32 * scale, 40 * scale), Image.NEAREST)

font = ImageFont.load_default(size=14)
rows = []
for fn in sorted(os.listdir(os.path.join(ROOT, "TERRAIN"))):
    if not fn.endswith(".MCD"):
        continue
    name = fn[:-4]
    data = open(os.path.join(ROOT, "TERRAIN", fn), "rb").read()
    recs = [data[i:i + 62] for i in range(0, len(data), 62)]
    frames = load_pck(name)
    info = []
    for r in recs:
        f = r[0:8]
        info.append(dict(frame=f[0], ufodoor=r[30], stoplos=r[31], nofloor=r[32], bigwall=r[33], lift=r[34], door=r[35],
                         tuwalk=r[39], armor=r[42], die=r[44], alt=r[46], tlevel=struct.unpack("b", r[48:49])[0],
                         foot=r[52], type=r[53], loft=list(r[8:20])))
    dies = {}
    alts = {}
    for i, d in enumerate(info):
        if d["die"] and d["die"] != i:
            dies.setdefault(d["die"], []).append(i)
        if d["alt"] and d["alt"] != i:
            alts.setdefault(d["alt"], []).append(i)
    for i, d in enumerate(info):
        rows.append([name, i, TYPES[d["type"]] if d["type"] < 4 else "?", d["tuwalk"], d["stoplos"], d["bigwall"], d["armor"],
                     d["door"], d["ufodoor"], d["foot"], d["tlevel"], d["die"], d["alt"],
                     " ".join(map(str, dies.get(i, []))), " ".join(map(str, alts.get(i, [])))])
    per = 40
    cols = 8
    cw, ch = 32 * 3 + 8, 40 * 3 + 36
    for start in range(0, len(info), per):
        chunk = list(range(start, min(len(info), start + per)))
        nrows = (len(chunk) + cols - 1) // cols
        sheet = Image.new("RGBA", (cols * cw, nrows * ch), (0, 0, 0, 255))
        dr = ImageDraw.Draw(sheet)
        for k, i in enumerate(chunk):
            d = info[i]
            x, y = (k % cols) * cw, (k // cols) * ch
            fr = d["frame"]
            if fr < len(frames):
                sheet.paste(sprite(frames[fr]), (x + 4, y + 2))
            tag = "%d %s" % (i, TYPES[d["type"]] if d["type"] < 4 else "?")
            if i in dies:
                tag += " wreck"
            if i in alts:
                tag += " alt"
            dr.text((x + 4, y + 40 * 3 + 4), tag, fill=(255, 255, 0, 255), font=font)
        sheet.save(os.path.join(OUT, "%s_%03d.png" % (name, start)))

with open(os.path.join(OUT, "pieces.csv"), "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["set", "idx", "type", "tuwalk", "stoplos", "bigwall", "armor", "door", "ufodoor", "foot", "tlevel", "die", "alt", "wreck_of", "alt_of"])
    w.writerows(rows)
print(len(rows), "pieces")
