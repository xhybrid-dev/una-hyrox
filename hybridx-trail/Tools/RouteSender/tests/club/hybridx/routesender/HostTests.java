package club.hybridx.routesender;

import java.nio.charset.Charset;
import java.util.zip.CRC32;

/**
 * Host tests for the pure parts (Fts, RouteFile): plain Java, no Android, no
 * JUnit, so build.sh can run them anywhere a JDK is. Exits non-zero on failure.
 */
public final class HostTests {
    private static int sRun;
    private static int sFailed;

    private static void check(boolean ok, String what) {
        sRun++;
        if (!ok) {
            sFailed++;
            System.out.println("FAIL: " + what);
        }
    }

    private static void eq(Object got, Object want, String what) {
        check(want == null ? got == null : want.equals(got), what + ": got " + got + ", want " + want);
    }

    private static byte[] bytes(String hex) {
        String h = hex.replace("-", "").replace(" ", "");
        byte[] b = new byte[h.length() / 2];
        for (int i = 0; i < b.length; i++) {
            b[i] = (byte) Integer.parseInt(h.substring(2 * i, 2 * i + 2), 16);
        }
        return b;
    }

    public static void main(String[] args) {
        // -- Encoding: the exact bytes Jon typed into nRF Connect for "list /Apps".
        eq(Fts.hex(Fts.listDir("/Apps"), 64), "50-00-05-00-2F-41-70-70-73", "LISTDIR /Apps");

        // -- The watch's reply he got back (the listing's terminator): 31 apps.
        Fts.Entry end = Fts.listEntry(bytes("51-01-00-00-1F-00-00-00-1F-00-00-00-00-00-00-00-00-00-00-00"
                + "00-00-00-00-00-00-00-00"));
        check(end != null, "terminator decodes");
        eq(end.status, Fts.OK, "terminator status");
        eq(end.entryNumber, 31L, "terminator entryNumber");
        eq(end.totalEntries, 31L, "terminator totalEntries");
        check(end.isTerminator(), "terminator detected");
        eq(end.name, "", "terminator name");

        // -- An entry: directory "HybridXTrail", entry 3 of 31.
        byte[] name = "HybridXTrail".getBytes(Charset.forName("UTF-8"));
        byte[] entry = new byte[28 + name.length];
        byte[] head = bytes("51-01-0C-00-03-00-00-00-1F-00-00-00-01-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00");
        System.arraycopy(head, 0, entry, 0, 28);
        System.arraycopy(name, 0, entry, 28, name.length);
        Fts.Entry e = Fts.listEntry(entry);
        eq(e.name, "HybridXTrail", "entry name");
        check(e.isDir, "entry is a directory");
        check(!e.isTerminator(), "entry is not the terminator");
        // A pathLength longer than what arrived must not read past the end.
        byte[] cut = new byte[28 + 4];
        System.arraycopy(entry, 0, cut, 0, cut.length);
        eq(Fts.listEntry(cut).name, "Hybr", "truncated entry name");
        check(Fts.listEntry(bytes("51-01")) == null, "short entry rejected");
        check(Fts.listEntry(bytes("21-01-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00")) == null,
                "wrong command rejected");

        // -- WRITE: {0x20, 0, pathLen16, offset32, time64, total32} + path.
        byte[] w = Fts.write("/A/b.gpx", 0, 0x0102030405060708L, 1000);
        eq(w.length, 20 + 8, "WRITE length");
        eq(Fts.hex(w, 64), "20-00-08-00-00-00-00-00-08-07-06-05-04-03-02-01-E8-03-00-00-2F-41-2F-62-2E-67-70-78", "WRITE");

        // -- WRITE_DATA: {0x22, 0x01, 0, 0, offset32, size32} + data.
        byte[] payload = bytes("AA-BB-CC-DD");
        eq(Fts.hex(Fts.writeData(0x100, payload, 1, 2), 64), "22-01-00-00-00-01-00-00-02-00-00-00-BB-CC",
                "WRITE_DATA");

        // -- MKDIR: {0x40, 0, pathLen16, reserved32, time64} + path.
        eq(Fts.hex(Fts.mkdir("/R", 1L), 64), "40-00-02-00-00-00-00-00-01-00-00-00-00-00-00-00-2F-52", "MKDIR");

        // -- DIGEST and DELETE share LISTDIR's short header.
        eq(Fts.hex(Fts.digest("/R"), 64), "70-00-02-00-2F-52", "DIGEST");
        eq(Fts.hex(Fts.delete("/R"), 64), "30-00-02-00-2F-52", "DELETE");

        // -- WRITE_PACING: 20 bytes, freeSpace at 16.
        Fts.Pacing p = Fts.pacing(bytes("21-01-00-00-10-00-00-00-00-00-00-00-00-00-00-00-F0-00-00-00"));
        eq(p.status, Fts.OK, "pacing status");
        eq(p.offset, 16L, "pacing offset");
        eq(p.freeSpace, 240L, "pacing freeSpace");
        check(Fts.pacing(bytes("21-01-00")) == null, "short pacing rejected");

        // -- DIGEST_STATUS: CRC-32 of "123456789" is the standard check value CBF43926.
        CRC32 crc = new CRC32();
        crc.update("123456789".getBytes(Charset.forName("US-ASCII")));
        eq(crc.getValue(), 0xCBF43926L, "CRC-32 check value");
        Fts.Digest d = Fts.digestStatus(bytes("71-01-00-00-09-00-00-00-26-39-F4-CB"));
        eq(d.fileSize, 9L, "digest size");
        eq(d.crc32, 0xCBF43926L, "digest crc");
        eq(Fts.simpleStatus(bytes("41-01-00-00-00-00-00-00-00-00-00-00-00-00-00-00"), Fts.MKDIR_STATUS), Fts.OK,
                "mkdir status");
        eq(Fts.simpleStatus(bytes("31-03"), Fts.MKDIR_STATUS), -1, "status of the wrong reply");

        // -- Names for the watch.
        eq(RouteFile.watchName("Lakes 20k.gpx"), "Lakes 20k.gpx", "plain name kept");
        eq(RouteFile.watchName("Ridge.GPX"), "Ridge.gpx", "extension lower-cased");
        eq(RouteFile.watchName("Ridge"), "Ridge.gpx", "extension added");
        eq(RouteFile.watchName("/storage/emulated/0/Download/Dorset.gpx"), "Dorset.gpx", "path dropped");
        eq(RouteFile.watchName("..\\..\\._hidden.gpx"), "_hidden.gpx", "no leading dot");
        eq(RouteFile.watchName("Café  run: día 2.gpx"), "Cafe run dia 2.gpx", "accents and punctuation");
        eq(RouteFile.watchName("Komoot_Tour-123 (v2).gpx"), "Komoot_Tour-123 (v2).gpx", "safe punctuation kept");
        eq(RouteFile.watchName(""), "Route.gpx", "empty name");
        eq(RouteFile.watchName("???.gpx"), "Route.gpx", "nothing usable");
        eq(RouteFile.watchName(null), "Route.gpx", "no name");
        String longName = RouteFile.watchName("The Long Way Round From Dorchester To Weymouth Via The Ridgeway.gpx");
        check(longName.length() <= RouteFile.MAX_NAME_BYTES, "long name fits Trail: " + longName);
        check(longName.endsWith(".gpx") && !longName.contains(" .gpx"), "long name ends cleanly: " + longName);
        check(RouteFile.isRouteName(longName), "long name is a route name");

        // -- What Trail would list.
        check(RouteFile.isRouteName("Lakes 20k.gpx"), "route name");
        check(!RouteFile.isRouteName("._Lakes.gpx"), "macOS shadow file");
        check(!RouteFile.isRouteName("routes.idx"), "index file");
        check(!RouteFile.isRouteName(".gpx"), "bare extension");

        // -- GPX sniffing.
        byte[] gpx = "<?xml version=\"1.0\"?>\n<GPX version=\"1.1\">".getBytes(Charset.forName("UTF-8"));
        check(RouteFile.looksLikeGpx(gpx, gpx.length), "GPX recognised");
        byte[] html = "<html><body>not a route</body></html>".getBytes(Charset.forName("UTF-8"));
        check(!RouteFile.looksLikeGpx(html, html.length), "HTML rejected");

        System.out.println(sRun + " checks, " + sFailed + " failed");
        System.exit(sFailed == 0 ? 0 : 1);
    }
}
