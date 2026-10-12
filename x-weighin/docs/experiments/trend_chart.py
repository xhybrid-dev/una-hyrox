#!/usr/bin/env python3
"""Chart a weigh-in log's daily weights against the trend, for the W1 gate.

    trend_chart.py <trend.csv> <out.png> [<compare.csv> <compare label>]

<trend.csv> is the output of Tests/Host's trend_csv tool (date,weight,trend,
mean7) at the app's share (20% a day). The chart shows the counted weigh-ins
as grey dots and the app's trend as a blue line. With <compare.csv> (trend_csv
at another share, on the same log) its trend is drawn as an orange line;
without it, the plain 7-day mean is. The CSVs are the chart's table view.

Light surface and chart chrome from the dataviz skill's reference palette;
the two series colours are its categorical slots 1 and 2, checked with its
validator (all checks pass). Needs matplotlib.
"""

import csv
import sys
from datetime import date

import matplotlib

matplotlib.use("Agg")
import matplotlib.dates as mdates  # noqa: E402
import matplotlib.pyplot as plt  # noqa: E402

SURFACE = "#fcfcfb"
INK = "#0b0b0b"
INK_2 = "#52514e"
MUTED = "#898781"
GRID = "#e1e0d9"
AXIS = "#c3c2b7"
TREND = "#2a78d6"   # categorical slot 1
OTHER = "#eb6834"   # categorical slot 2


def read(path):
    days, weights, trend, mean7 = [], [], [], []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            d = date.fromisoformat(row["date"])
            days.append(d)
            weights.append(float(row["weight"]) if row["weight"] else None)
            trend.append(float(row["trend"]))
            mean7.append(float(row["mean7"]) if row["mean7"] else None)
    return days, weights, trend, mean7


def main():
    if len(sys.argv) not in (3, 5):
        sys.exit(__doc__)
    days, weights, trend, mean7 = read(sys.argv[1])
    if len(sys.argv) == 5:
        cdays, _, other, _ = read(sys.argv[3])
        other_label, other_short = sys.argv[4], sys.argv[4]
        od, ov = cdays, other
    else:
        od = [d for d, m in zip(days, mean7) if m is not None]
        ov = [m for m in mean7 if m is not None]
        other_label, other_short = "Plain 7-day mean (comparison)", "7-day mean"

    plt.rcParams["font.family"] = "sans-serif"
    fig, ax = plt.subplots(figsize=(9, 4.6), dpi=110)
    fig.patch.set_facecolor(SURFACE)
    ax.set_facecolor(SURFACE)

    wd = [d for d, w in zip(days, weights) if w is not None]
    wv = [w for w in weights if w is not None]
    ax.scatter(wd, wv, s=30, color=MUTED, edgecolors=SURFACE, linewidths=1.5,
               zorder=3, label="Weigh-in (the latest of each day)")
    ax.plot(od, ov, color=OTHER, linewidth=2, solid_capstyle="round",
            solid_joinstyle="round", zorder=4, label=other_label)
    ax.plot(days, trend, color=TREND, linewidth=2, solid_capstyle="round",
            solid_joinstyle="round", zorder=5, label="Trend, 20% a day (the app)")

    # Direct labels at the line ends, in ink, with a short key line beside them.
    for value, colour, text in ((trend[-1], TREND, "20%"),
                                (ov[-1], OTHER, other_short.split(",")[-1].split(" a day")[0].strip())):
        ax.annotate(f"{text} {value:.2f}", xy=(days[-1], value),
                    xytext=(10, 0), textcoords="offset points", va="center",
                    fontsize=9, color=INK_2)
        ax.plot([days[-1]], [value], marker="o", markersize=5, color=colour,
                markeredgecolor=SURFACE, markeredgewidth=1.5, zorder=6)

    ax.set_title("Sample log: daily weigh-ins and the trend", loc="left",
                 fontsize=12, color=INK, pad=12)
    ax.set_ylabel("kg", color=INK_2, fontsize=9)
    ax.grid(axis="y", color=GRID, linewidth=1, linestyle="-")
    ax.set_axisbelow(True)
    for side in ("top", "right", "left"):
        ax.spines[side].set_visible(False)
    ax.spines["bottom"].set_color(AXIS)
    ax.tick_params(colors=MUTED, labelsize=8, length=0)
    ax.xaxis.set_major_locator(mdates.WeekdayLocator(byweekday=mdates.MO, interval=1))
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%d %b"))
    ax.margins(x=0.02)
    ax.set_xlim(right=days[-1] + (days[-1] - days[0]) * 0.13)
    # No date ticks in the room kept for the end labels.
    last = mdates.date2num(days[-1])
    ax.set_xticks([t for t in ax.get_xticks() if t <= last])

    leg = ax.legend(loc="lower left", frameon=False, fontsize=8.5)
    for text in leg.get_texts():
        text.set_color(INK_2)

    fig.tight_layout()
    fig.savefig(sys.argv[2], facecolor=SURFACE)
    print("wrote", sys.argv[2])


if __name__ == "__main__":
    main()
