#!/usr/bin/env python3
"""One zip of the X family's store assets, a folder per listing, for Jon to download and
upload to the store (or use anywhere else).

    python3 hybridx-link/Tools/Brand/pack_assets.py <out.zip>

Inside, X-Family-Store-Assets/:
  00-The-X-Family/       the family banner and all the icons together
  01-HybridX-Race/ ...   one folder per listing:
      icon/              1024, 512 and 256 px (full colour, for the store page and socials),
                         and the watch's own 60 and 30 px (as in the .uapp and the upload zip)
      store-images/      01-banner.png (1920 x 1080, upload first), then the 1080 x 1080 cards
      watch-screens/     the 480 px simulator screens in the upload zip's previews/
  README.txt

Everything is copied from the repository as it stands (make_icons.py and store_images.py
make them), except the 512 and 256 px icons, scaled here from the 1024 px masters.
"""
import io
import os
import sys
import zipfile

from PIL import Image

import brand

ROOT = brand.ROOT
ICONS = os.path.join(ROOT, "hybridx-link", "docs", "brand", "icons")
TOP = "X-Family-Store-Assets"

# (folder in the zip, icon name, app folder, glance?)
LISTINGS = [
    ("01-HybridX-Race", "race", "hybridx-race", False),
    ("02-X-Run", "run", "hybridx-run", False),
    ("03-VO2max-Glance", "run-glance", "hybridx-run", True),
    ("04-X-Trail", "trail", "hybridx-trail", False),
    ("05-X-Ride", "ride", "hybridx-ride", False),
    ("06-X-Workout", "workout", "hybridx-workout", False),
    ("07-X-Streak", "streak", "hybridx-streak", False),
    ("08-X-Streak-Glance", "streak-glance", "hybridx-streak", True),
]

README = """The X family: store assets
==========================

One folder per store listing. In each:

  icon/            icon-1024.png, icon-512.png, icon-256.png: the icon in full colour,
                   for the store page, socials and press.
                   icon-60-watch.png, icon-30-watch.png: the watch's own icon, in the
                   watch's 64 colours. These are already in the app and in its upload zip,
                   so they are for reference.
  store-images/    01-banner.png (1920 x 1080): upload it first, as the listing's first
                   image. Then the square cards (1080 x 1080), in the order of their numbers.
  watch-screens/   the real app screens (480 x 480), the same ones the upload zip carries
                   as its previews.

00-The-X-Family holds the family banner and every icon side by side.

Glances: a glance carries no icon on the watch. Its icon here is its store icon, its app's
symbol on a light disc.

The watch in the images is UNA's own render, from their developer design kit. Check once with
UNA that they are happy for it to appear in store images.

Made by hybridx-link/Tools/Brand (make_icons.py, store_images.py, pack_assets.py) in the
una-hyrox repository. Fonts: Anton, Space Grotesk and Inter (SIL Open Font License).
"""


def png_bytes(img):
    buf = io.BytesIO()
    img.save(buf, "PNG", optimize=True)
    return buf.getvalue()


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    count = 0
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        def add(path_in_zip, src=None, data=None):
            nonlocal count
            arc = "%s/%s" % (TOP, path_in_zip)
            if data is not None:
                z.writestr(arc, data)
            else:
                if not os.path.isfile(src):
                    sys.exit("missing: %s" % src)
                z.write(src, arc)
            count += 1

        add("README.txt", data=README)
        add("00-The-X-Family/family-banner.png", os.path.join(ROOT, "hybridx-link/docs/brand/family-banner.png"))
        for folder, icon, app, glance in LISTINGS:
            master = os.path.join(ICONS, icon + "-1024.png")
            add("00-The-X-Family/icons/%s-1024.png" % folder[3:], master)

            # The icon, three full-colour sizes and the watch's own.
            add("%s/icon/icon-1024.png" % folder, master)
            m = Image.open(master).convert("RGBA")
            for size in (512, 256):
                add("%s/icon/icon-%d.png" % (folder, size), data=png_bytes(m.resize((size, size), Image.LANCZOS)))
            res = os.path.join(ROOT, app, "Resources", "Glance" if glance else "")
            add("%s/icon/icon-60-watch.png" % folder, os.path.join(res, "icon_60x60.png"))
            if not glance:
                add("%s/icon/icon-30-watch.png" % folder, os.path.join(res, "icon_30x30.png"))

            # The store images, in upload order.
            store = os.path.join(ROOT, app, "docs", "store", "glance" if glance else "")
            for f in sorted(os.listdir(store)):
                if f.endswith(".png") and f[:2].isdigit():
                    add("%s/store-images/%s" % (folder, f), os.path.join(store, f))

            # The real screens, as the upload zip's previews.
            prev = os.path.join(res, "previews")
            for f in sorted(os.listdir(prev)):
                if f.endswith(".png"):
                    add("%s/watch-screens/%s" % (folder, f), os.path.join(prev, f))
    print("wrote %s: %d files, %.1f MB" % (out, count, os.path.getsize(out) / 1e6))


if __name__ == "__main__":
    main()
