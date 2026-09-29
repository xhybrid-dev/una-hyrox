package club.hybridx.routesender;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Parcelable;
import android.provider.OpenableColumns;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * HybridX Route Sender (test build): pick or share a GPX, send it to HybridX
 * Trail's Routes/ folder on a paired UNA Watch. One screen, a log for Jon to
 * copy back, nothing else.
 */
public class MainActivity extends Activity {

    private static final int PICK_FILE = 1;
    private static final int ASK_BLUETOOTH = 2;
    private static final String PERM_CONNECT = "android.permission.BLUETOOTH_CONNECT";

    private TextView mFileText;
    private Button mChoose;
    private Button mSend;
    private Button mCheck;
    private ProgressBar mProgress;
    private TextView mStatus;
    private TextView mLog;
    private ScrollView mScroll;

    private byte[] mData;
    private String mWatchName;
    private boolean mBusy;
    /** What to do once Bluetooth permission is granted: 0 none, 1 check, 2 send. */
    private int mPending;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        buildUi();
        log("HybridX Route Sender " + versionName() + ", Android " + Build.VERSION.RELEASE
                + " (API " + Build.VERSION.SDK_INT + "), " + Build.MANUFACTURER + " " + Build.MODEL);
        handleIntent(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        handleIntent(intent);
    }

    // -- Screen ----------------------------------------------------------------------

    private int dp(int v) {
        return Math.round(v * getResources().getDisplayMetrics().density);
    }

    private void buildUi() {
        LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        col.setPadding(dp(16), dp(16), dp(16), dp(16));

        TextView intro = new TextView(this);
        intro.setText("Send a GPX route to HybridX Trail on your UNA Watch.\n"
                + "Your watch must be paired through the UNA app, nearby, with HybridX Trail installed.");
        intro.setTextSize(15);
        col.addView(intro);

        mChoose = button("Choose GPX file");
        mChoose.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { chooseFile(); }
        });
        col.addView(mChoose);

        mFileText = new TextView(this);
        mFileText.setText("No route chosen. You can also share a GPX to this app from another app.");
        mFileText.setTextSize(15);
        mFileText.setPadding(0, dp(8), 0, dp(8));
        col.addView(mFileText);

        mSend = button("Send to watch");
        mSend.setEnabled(false);
        mSend.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { start(2); }
        });
        col.addView(mSend);

        mProgress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        mProgress.setMax(1000);
        col.addView(mProgress, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(24)));

        mStatus = new TextView(this);
        mStatus.setTextSize(17);
        mStatus.setTypeface(Typeface.DEFAULT_BOLD);
        mStatus.setPadding(0, dp(4), 0, dp(12));
        col.addView(mStatus);

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        mCheck = button("Check watch");
        mCheck.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { start(1); }
        });
        Button copy = button("Copy log");
        copy.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { copyLog(); }
        });
        row.addView(mCheck, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        row.addView(copy, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        col.addView(row);

        mLog = new TextView(this);
        mLog.setTypeface(Typeface.MONOSPACE);
        mLog.setTextSize(11);
        mLog.setTextIsSelectable(true);
        mLog.setPadding(0, dp(8), 0, 0);
        col.addView(mLog);

        mScroll = new ScrollView(this);
        mScroll.addView(col);
        setContentView(mScroll);
    }

    private Button button(String text) {
        Button b = new Button(this);
        b.setText(text);
        b.setAllCaps(false);
        b.setTextSize(16);
        return b;
    }

    private void setBusy(boolean busy) {
        mBusy = busy;
        mChoose.setEnabled(!busy);
        mCheck.setEnabled(!busy);
        mSend.setEnabled(!busy && mData != null);
    }

    private void status(final String text) {
        runOnUiThread(new Runnable() {
            @Override public void run() { mStatus.setText(text); }
        });
    }

    private void log(final String line) {
        final String stamped = new SimpleDateFormat("HH:mm:ss.SSS", Locale.UK).format(new Date()) + " " + line;
        runOnUiThread(new Runnable() {
            @Override public void run() {
                mLog.append(stamped + "\n");
                mScroll.post(new Runnable() {
                    @Override public void run() { mScroll.fullScroll(View.FOCUS_DOWN); }
                });
            }
        });
    }

    private void copyLog() {
        ClipboardManager cm = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
        cm.setPrimaryClip(ClipData.newPlainText("Route Sender log", mLog.getText()));
        status("Log copied. Paste it into your message to Claude.");
    }

    private String versionName() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return "?";
        }
    }

    // -- Choosing a file ---------------------------------------------------------------

    private void chooseFile() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");   // .gpx has no reliable MIME type across phones
        startActivityForResult(i, PICK_FILE);
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request == PICK_FILE && result == RESULT_OK && data != null && data.getData() != null) {
            load(data.getData());
        }
    }

    private void handleIntent(Intent intent) {
        if (intent == null) {
            return;
        }
        Uri uri = null;
        if (Intent.ACTION_SEND.equals(intent.getAction())) {
            Parcelable p = intent.getParcelableExtra(Intent.EXTRA_STREAM);
            if (p instanceof Uri) {
                uri = (Uri) p;
            }
        } else if (Intent.ACTION_VIEW.equals(intent.getAction())) {
            uri = intent.getData();
        }
        if (uri != null) {
            load(uri);
        }
    }

    private void load(final Uri uri) {
        String display = queryName(uri);
        byte[] data;
        try {
            data = readAll(uri);
        } catch (IOException e) {
            showFileProblem("Couldn't read that file: " + e.getMessage());
            return;
        }
        if (data == null) {
            showFileProblem("That file is too big for a route (over "
                    + (RouteFile.MAX_BYTES / (1024 * 1024)) + " MB).");
            return;
        }
        if (!RouteFile.looksLikeGpx(data, data.length)) {
            showFileProblem("\"" + display + "\" doesn't look like a GPX route.");
            return;
        }
        mData = data;
        mWatchName = RouteFile.watchName(display);
        mFileText.setText("Route: " + display + "\n" + kb(data.length) + ", saved on the watch as \""
                + mWatchName + "\"");
        status("");
        mProgress.setProgress(0);
        log("Chose " + display + " (" + data.length + " bytes) -> " + mWatchName);
        setBusy(mBusy);
    }

    private void showFileProblem(String text) {
        mData = null;
        mFileText.setText(text);
        log(text);
        setBusy(mBusy);
    }

    private String queryName(Uri uri) {
        String name = null;
        Cursor c = null;
        try {
            c = getContentResolver().query(uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null);
            if (c != null && c.moveToFirst()) {
                name = c.getString(0);
            }
        } catch (RuntimeException ignored) {
            // Some providers don't answer; fall back to the path.
        } finally {
            if (c != null) {
                c.close();
            }
        }
        if (name == null) {
            name = uri.getLastPathSegment();
        }
        return name == null ? "Route.gpx" : name;
    }

    /** The whole file, or null if it is over RouteFile.MAX_BYTES. */
    private byte[] readAll(Uri uri) throws IOException {
        InputStream in = getContentResolver().openInputStream(uri);
        if (in == null) {
            throw new IOException("no data");
        }
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[16384];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
                if (out.size() > RouteFile.MAX_BYTES) {
                    return null;
                }
            }
            return out.toByteArray();
        } finally {
            in.close();
        }
    }

    private static String kb(long bytes) {
        return bytes < 1024 ? bytes + " bytes" : ((bytes + 512) / 1024) + " KB";
    }

    // -- Talking to the watch -----------------------------------------------------------

    private void start(int action) {
        if (mBusy) {
            return;
        }
        if (Build.VERSION.SDK_INT >= 31 && checkSelfPermission(PERM_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            mPending = action;
            requestPermissions(new String[] { PERM_CONNECT }, ASK_BLUETOOTH);
            return;
        }
        BluetoothManager bm = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = bm == null ? null : bm.getAdapter();
        if (adapter == null || !adapter.isEnabled()) {
            status("Turn Bluetooth on, then try again.");
            return;
        }
        final List<BluetoothDevice> watches;
        try {
            watches = WatchLink.pairedWatches(adapter);
        } catch (SecurityException e) {
            status("This app needs the Nearby devices permission.");
            return;
        }
        if (watches.isEmpty()) {
            status("No paired UNA Watch found. Pair your watch with the UNA app first.");
            return;
        }
        final int act = action;
        if (watches.size() == 1) {
            run(act, watches.get(0));
            return;
        }
        String[] names = new String[watches.size()];
        for (int i = 0; i < names.length; i++) {
            names[i] = watches.get(i).getName() + "  " + watches.get(i).getAddress();
        }
        new AlertDialog.Builder(this).setTitle("Which watch?")
                .setItems(names, new DialogInterface.OnClickListener() {
                    @Override public void onClick(DialogInterface d, int which) { run(act, watches.get(which)); }
                }).show();
    }

    @Override
    public void onRequestPermissionsResult(int request, String[] permissions, int[] results) {
        if (request == ASK_BLUETOOTH) {
            int action = mPending;
            mPending = 0;
            if (results.length > 0 && results[0] == PackageManager.PERMISSION_GRANTED) {
                start(action);
            } else {
                status("Without the Nearby devices permission this app can't reach the watch.");
            }
        }
    }

    private void run(final int action, final BluetoothDevice device) {
        setBusy(true);
        mProgress.setProgress(0);
        status(action == 2 ? "Sending…" : "Checking the watch…");
        final byte[] data = mData;
        final String name = mWatchName;
        new Thread(new Runnable() {
            @Override public void run() {
                WatchLink link = new WatchLink(MainActivity.this, new WatchLink.Listener() {
                    @Override public void log(String line) { MainActivity.this.log(line); }
                    @Override public void progress(final long done, final long total) {
                        runOnUiThread(new Runnable() {
                            @Override public void run() {
                                mProgress.setProgress(total == 0 ? 1000 : (int) (done * 1000 / total));
                                mStatus.setText("Sending… " + kb(done) + " of " + kb(total));
                            }
                        });
                    }
                });
                try {
                    link.open(device);
                    if (action == 2) {
                        send(link, name, data);
                    } else {
                        check(link);
                    }
                } catch (IOException e) {
                    log("FAILED: " + e.getMessage());
                    status(e.getMessage());
                } catch (RuntimeException e) {
                    log("FAILED: " + e);
                    status("Something went wrong: " + e.getMessage());
                } finally {
                    link.close();
                    runOnUiThread(new Runnable() {
                        @Override public void run() { setBusy(false); }
                    });
                }
            }
        }, "watch-link").start();
    }

    /** Finds Trail's folder; its exact name as the watch spells it. */
    private String findTrail(WatchLink link) throws IOException {
        List<Fts.Entry> apps = link.listDir(RouteFile.APPS_DIR);
        if (apps == null) {
            throw new WatchLink.WatchException("Couldn't list the watch's apps.");
        }
        log("Apps on the watch: " + apps.size());
        for (Fts.Entry e : apps) {
            if (e.isDir && e.name.equalsIgnoreCase(RouteFile.APP_DIR_NAME)) {
                log("Found " + RouteFile.APPS_DIR + "/" + e.name);
                return RouteFile.APPS_DIR + "/" + e.name;
            }
        }
        throw new WatchLink.WatchException("HybridX Trail isn't on this watch. Install it first.");
    }

    /** Lists Routes/ into the log; null if the folder isn't there. */
    private List<Fts.Entry> listRoutes(WatchLink link, String routesDir) throws IOException {
        List<Fts.Entry> routes = link.listDir(routesDir);
        if (routes == null) {
            log("No Routes folder yet");
            return null;
        }
        int count = 0;
        for (Fts.Entry e : routes) {
            if (!e.isDir && RouteFile.isRouteName(e.name)) {
                count++;
                log("  route: " + e.name + " (" + e.size + " bytes)");
            }
        }
        log("Routes on the watch: " + count);
        return routes;
    }

    private void check(WatchLink link) throws IOException {
        String trail = findTrail(link);
        listRoutes(link, trail + "/Routes");
        status("Watch OK: HybridX Trail found, file transfer version " + link.version() + ".");
    }

    private void send(WatchLink link, String name, byte[] data) throws IOException {
        String trail = findTrail(link);
        String routesDir = trail + "/Routes";

        List<Fts.Entry> routes = listRoutes(link, routesDir);
        if (routes == null) {
            int st = link.mkdir(routesDir);
            log("MKDIR " + routesDir + ": " + (st < 0 ? "no reply" : Fts.statusName(st)));
            routes = listRoutes(link, routesDir);
            if (routes == null) {
                throw new WatchLink.WatchException("Couldn't make the Routes folder on the watch.");
            }
        }
        int count = 0;
        boolean replacing = false;
        for (Fts.Entry e : routes) {
            if (!e.isDir && RouteFile.isRouteName(e.name)) {
                count++;
                if (e.name.equalsIgnoreCase(name)) {
                    replacing = true;
                    name = e.name;   // overwrite in place, keeping the watch's spelling
                }
            }
        }
        if (!replacing && count >= RouteFile.MAX_ROUTES) {
            log("Warning: Trail shows at most " + RouteFile.MAX_ROUTES + " routes; this one may not appear");
        }

        String path = routesDir + "/" + name;
        log((replacing ? "Replacing " : "Writing ") + path + " (" + data.length + " bytes)");
        long t0 = System.currentTimeMillis();
        link.writeFile(path, data);
        long ms = Math.max(1, System.currentTimeMillis() - t0);
        // Bytes per second: a small route rounds to "0 KB/s" otherwise.
        log("Written in " + (ms / 1000.0) + " s (" + (data.length * 1000L / ms) + " bytes/s)");

        Fts.Digest d = link.digest(path);
        long crc = WatchLink.crc32(data);
        if (d == null) {
            log("No digest check (watch version " + link.version() + ")");
        } else if (d.status != Fts.OK) {
            throw new WatchLink.WatchException("The watch couldn't check the file (" + Fts.statusName(d.status) + ").");
        } else if (d.fileSize != data.length || d.crc32 != crc) {
            throw new WatchLink.WatchException("The file on the watch doesn't match (size " + d.fileSize
                    + ", CRC " + Long.toHexString(d.crc32) + " vs " + Long.toHexString(crc) + "). Try again.");
        } else {
            log("Verified: size and CRC-32 " + Long.toHexString(crc) + " match");
        }
        listRoutes(link, routesDir);
        status("Done. Open HybridX Trail on your watch to find \"" + name.substring(0, name.length() - 4) + "\".");
    }
}
