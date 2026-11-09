# The X family's brand: icons and store images

Jon, 9 November 2026. He wanted the family to stand out from the generic look of store
listings, by using the HybridX brand (hybridx.club; the web repository `xhybrid-dev/hybridx-web-x`):
not yellow and black on all of them, but a varied, cohesive portfolio.

## The system

**Shared by every app:**
- HybridX's blade X (`assets/x-mark.png`, the brand's "Icon Logo.png"): big and cropped behind the
  watch on every card, and beside HYBRIDX.CLUB at the foot of every image;
- the brand's type:
  - **Anton** for the one big uppercase phrase (as HybridX's Instagram does);
  - **Space Grotesk** 500/700 for headlines and lines;
  - **Inter** 600 uppercase labels, at the brand's one tracking value, 0.12 em;
- one layout: text on the left, UNA's watch with a real simulator screen on the right, the X's
  chevron behind it, and a family card that closes each set;
- the brand's rules (`hybridx-web-x/handover/entry-funnel/docs/07-brand-and-design.md`): yellow
  is never text on white, and nothing is small.

**Each app's own colourway** (`brand.py`, `COLOURWAYS`):

| App | Colourway | Dark | Accent | Light | Watch |
|---|---|---|---|---|---|
| HybridX Race | Classic | black | HybridX yellow `#FADB5C` | white (its light cards are the yellow fill) | black |
| X Run | Engine | oxblood `#5C0F1A` | crimson `#C1121F`, heart `#E63946` | cream `#FBF5EF` | black |
| X Trail | Moss | forest `#1F3A2B` | olive `#6B8F3A`, route magenta `#E040A0` | bone `#EFE9DC` | white |
| X Ride | Velodrome | navy `#0E1A4F` | cobalt `#2346E8`, signal orange `#FF6A1A` | `#F2F4FB` | black |
| X Workout | Iron | graphite `#161618` | pink `#FF2E88` | `#EDEDED` | black |
| X Streak | Glacier | navy `#0B1F33` | ice teal `#2EC4C4` | snow `#F4FAFB` | teal |

Two of the colourways come from HybridX's own work: Engine is the "Build a Bigger Engine" VO2max
campaign's palette, and Iron's pink is the HybridX app's pink mode.

## The icons (`make_icons.py`)

- **The system:** a disc in the app's colour with its glyph, drawn once on a 60-unit grid:

  | App | Glyph | Disc |
  |---|---|---|
  | Race | the blade X | yellow |
  | Run | the engine gauge | oxblood |
  | Trail | the route | moss |
  | Ride | the bike | cobalt |
  | Workout | a kettlebell | pink |
  | Streak | the summit | navy |

- **Glances:** each glance has its app's glyph reversed onto the light colour.
- **What it writes:**
  - **For the watch:** each app's `Resources/icon_60x60.png` and `icon_30x30.png`, and each
    glance's store icon `Resources/Glance/icon_60x60.png`.
    - They use the watch's grid colours. The `.uapp` keeps two bits a channel and two of alpha
      (`app_merging.py`: `v >> 6`), so only 0, 85, 170 and 255 survive.
    - The store's `icon.png` is the 60 px file, so the phone shows what the watch shows.
  - **For the store page and socials:** `hybridx-link/docs/brand/icons/<app>-1024.png`, in full
    brand colour.
  - **A check sheet:** `hybridx-link/docs/brand/icons/watch-preview.png`, every icon as the watch
    will show it (the truncation simulated), at 60 and 30 px.

```
python3 hybridx-link/Tools/Brand/make_icons.py
```

## The store images (`store_images.py`)

For each listing, into its app's `docs/store/` (a glance's into `docs/store/glance/`):
- `01-banner.png`, 1920 × 1080: the first image of the listing;
- `02-…png` onwards, 1080 × 1080: four feature cards, then the family card (the glances have
  two feature cards);
- plus `hybridx-link/docs/brand/family-banner.png`, the whole family.

```
UNA_SDK=/path/to/una-sdk python3 hybridx-link/Tools/Brand/store_images.py
```

**What is real, and what is drawn:**
- **The watch screens** are the real simulator previews in each app's `Resources/previews/`
  (`docs/experiments/store_previews.sh`). The glance screens are the layout code's mock-ups
  (`glance_preview.py --store`).
- **The watch** is UNA's own render, from the SDK's Figma UI Resource Pack, read from `$UNA_SDK`
  at run time; none is copied here.
- **Every headline** is a claim from `hybridx-link/docs/STORE_RELEASE.md`'s checked facts.

**The check:** the run fails if any text has less than 4.5:1 contrast. It measures each text
block against the real pixels under it (the ground, the X, the watch), not against the card's
base colour.

## The asset pack (`pack_assets.py`)

One zip with a folder for each listing:
- `icon/`: 1024, 512 and 256 px, and the watch's 60 and 30 px;
- `store-images/`: the banner first, then the cards;
- `watch-screens/`: the 480 px previews.

It also holds the family banner and every icon together, and a README.txt. Rebuild it after
any change:

```
python3 hybridx-link/Tools/Brand/pack_assets.py X-Family-Store-Assets.zip
```

## Fonts and the logo

- `fonts/` holds Anton, Space Grotesk and Inter, all under the SIL Open Font License 1.1. Their
  licences are beside them.
- `assets/x-mark.png` is HybridX's own mark, Jon's.

## Before release

**UNA's logo:** the watch renders show the UNA logo on the bezel.
- The SDK's licence covers its code only.
- Its `TRADEMARK.md` allows "for the UNA Watch" as a nominative reference, without implying
  endorsement.
- The Figma pack ships these renders as mock-ups for developers, and Knitting's store art already
  uses them.

So check with UNA once that device renders in store art are welcome on their store.
