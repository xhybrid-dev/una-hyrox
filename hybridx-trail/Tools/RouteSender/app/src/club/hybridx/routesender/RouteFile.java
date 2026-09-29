package club.hybridx.routesender;

import java.nio.charset.Charset;
import java.text.Normalizer;

/**
 * What goes onto the watch, and under what name: pure Java, host-tested.
 *
 * The rules follow what HybridX Trail's Navigator accepts
 * (hybridx-trail/Software/Libs/Core/Sources/Navigator.cpp): a name ending
 * ".gpx", not starting with ".", and shorter than RouteInfo::file[48].
 */
public final class RouteFile {
    private RouteFile() {}

    /** The watch app's folder, as the phone sees it over FTS. */
    public static final String APP_DIR_NAME = "HybridXTrail";
    public static final String APPS_DIR = "/Apps";
    public static final String APP_DIR = APPS_DIR + "/" + APP_DIR_NAME;
    public static final String ROUTES_DIR = APP_DIR + "/Routes";

    /** Trail's RouteInfo::file is char[48], so at most 47 bytes. */
    public static final int MAX_NAME_BYTES = 47;
    /** Trail lists at most this many routes (Navigator::kMaxRoutes). */
    public static final int MAX_ROUTES = 16;
    /** Trail reads up to 16 MB; a planner's route is ~100 KB. Keep sends sane. */
    public static final int MAX_BYTES = 8 * 1024 * 1024;

    private static final Charset UTF8 = Charset.forName("UTF-8");

    /**
     * A safe file name for the watch from whatever name the phone gave the file:
     * letters, digits, spaces and - _ ( ) only, no leading dot, ".gpx" on the
     * end, and short enough for Trail.
     */
    public static String watchName(String displayName) {
        String base = displayName == null ? "" : displayName;
        int slash = Math.max(base.lastIndexOf('/'), base.lastIndexOf('\\'));
        if (slash >= 0) {
            base = base.substring(slash + 1);
        }
        if (base.toLowerCase().endsWith(".gpx")) {
            base = base.substring(0, base.length() - 4);
        }
        // "Café" -> "Cafe": split accents off their letters, then drop them.
        base = Normalizer.normalize(base, Normalizer.Form.NFD).replaceAll("\\p{M}+", "");
        StringBuilder sb = new StringBuilder();
        boolean lastWasSpace = true;   // drops leading spaces
        for (int i = 0; i < base.length(); i++) {
            char c = base.charAt(i);
            boolean keep = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
                    || c == '-' || c == '_' || c == '(' || c == ')';
            if (keep) {
                sb.append(c);
                lastWasSpace = false;
            } else if (!lastWasSpace) {
                // Spaces, dots and anything else become one space.
                sb.append(' ');
                lastWasSpace = true;
            }
        }
        String clean = sb.toString().trim();
        int maxBase = MAX_NAME_BYTES - 4;
        if (clean.length() > maxBase) {
            clean = clean.substring(0, maxBase).trim();
        }
        if (clean.isEmpty()) {
            clean = "Route";
        }
        return clean + ".gpx";
    }

    /** True if the bytes look like a GPX: a "<gpx" tag near the start. */
    public static boolean looksLikeGpx(byte[] data, int length) {
        int n = Math.min(length, 4096);
        String head = new String(data, 0, n, UTF8).toLowerCase();
        return head.contains("<gpx");
    }

    /** True if this listing name is one Trail would show as a route. */
    public static boolean isRouteName(String name) {
        return name != null && name.length() >= 5 && name.charAt(0) != '.'
                && name.toLowerCase().endsWith(".gpx")
                && name.getBytes(UTF8).length <= MAX_NAME_BYTES;
    }
}
