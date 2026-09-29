package club.hybridx.routesender;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Build;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.zip.CRC32;

/**
 * One session with the watch over the BLE File Transfer Service.
 *
 * Blocking by design: every call runs on the caller's worker thread and waits
 * for the matching GATT callback or notification, one operation at a time
 * (Android allows only one outstanding GATT operation per client).
 *
 * The watch does not advertise while the UNA app is connected to it, so this
 * never scans: it connects to the watch the phone is already bonded with
 * (hybridx-trail/docs/NOTES.md, "Phone delivery over BLE").
 */
public final class WatchLink extends BluetoothGattCallback {

    public interface Listener {
        void log(String line);
        void progress(long done, long total);
    }

    /** A failure with a message fit to show the user. */
    public static final class WatchException extends IOException {
        public WatchException(String message) { super(message); }
    }

    private static final UUID SERVICE = UUID.fromString(Fts.SERVICE_UUID);
    private static final UUID VERSION = UUID.fromString(Fts.VERSION_UUID);
    private static final UUID RAW = UUID.fromString(Fts.RAW_UUID);
    private static final UUID CCCD = UUID.fromString(Fts.CCCD_UUID);

    private static final int REQUEST_MTU = 247;
    private static final long CONNECT_TIMEOUT_MS = 20000;
    private static final long OP_TIMEOUT_MS = 8000;
    private static final long REPLY_TIMEOUT_MS = 8000;
    /** v5 write window (Docs/BLE-File-Transfer-Service.md: recommended 2048). */
    private static final int WRITE_WINDOW = 2048;
    private static final int MAX_STALLS = 10;

    private static Method sWrite33;

    private final Context mContext;
    private final Listener mListener;

    private final Semaphore mConnected = new Semaphore(0);
    private final Semaphore mOpDone = new Semaphore(0);
    private final LinkedBlockingQueue<byte[]> mNotes = new LinkedBlockingQueue<byte[]>();
    private volatile int mOpStatus;
    private volatile byte[] mReadValue;
    private volatile int mMtu = 23;
    private volatile boolean mIsConnected;

    private BluetoothGatt mGatt;
    private BluetoothGattCharacteristic mRaw;
    private int mVersion;

    public WatchLink(Context context, Listener listener) {
        mContext = context.getApplicationContext();
        mListener = listener;
    }

    /** Bonded devices that look like a UNA Watch ("UNA WATCH 042648"). */
    public static List<BluetoothDevice> pairedWatches(BluetoothAdapter adapter) {
        List<BluetoothDevice> out = new ArrayList<BluetoothDevice>();
        if (adapter == null) {
            return out;
        }
        for (BluetoothDevice d : adapter.getBondedDevices()) {
            String name = d.getName();
            if (name != null && name.toUpperCase().contains("UNA")) {
                out.add(d);
            }
        }
        return out;
    }

    public int version() {
        return mVersion;
    }

    // -- Session -------------------------------------------------------------------

    public void open(BluetoothDevice device) throws IOException {
        log("Connecting to " + device.getName() + " (" + device.getAddress() + ")");
        mGatt = device.connectGatt(mContext, false, this, BluetoothDevice.TRANSPORT_LE);
        if (mGatt == null) {
            throw new WatchException("Android refused to open a Bluetooth connection.");
        }
        if (!acquire(mConnected, CONNECT_TIMEOUT_MS) || !mIsConnected) {
            throw new WatchException("Couldn't connect to the watch. Is it nearby, with Bluetooth on?");
        }
        log("Connected");

        // The UNA app has usually set the link's MTU already; Android then
        // reports the current one. If no answer comes, carry on at the
        // minimum (23): slow, but safe, and the log shows it.
        mOpDone.drainPermits();
        if (!mGatt.requestMtu(REQUEST_MTU) || !acquire(mOpDone, 5000)) {
            log("No MTU answer; using the minimum");
        }
        log("MTU " + mMtu + " (up to " + chunkSize() + " bytes of file per packet)");

        mOpDone.drainPermits();
        if (!mGatt.discoverServices()) {
            throw new WatchException("Couldn't read the watch's services.");
        }
        waitOp("service discovery");
        BluetoothGattService fts = mGatt.getService(SERVICE);
        if (fts == null) {
            throw new WatchException("The watch has no file transfer service (0xFEBB).");
        }
        BluetoothGattCharacteristic version = fts.getCharacteristic(VERSION);
        mRaw = fts.getCharacteristic(RAW);
        if (version == null || mRaw == null) {
            throw new WatchException("The watch's file transfer service is missing a characteristic.");
        }

        mOpDone.drainPermits();
        mReadValue = null;
        if (!mGatt.readCharacteristic(version)) {
            throw new WatchException("Couldn't read the file transfer version.");
        }
        waitOp("version read");
        byte[] v = mReadValue;
        if (v == null || v.length < 4) {
            throw new WatchException("The watch sent an unreadable file transfer version.");
        }
        mVersion = ByteBuffer.wrap(v).order(ByteOrder.LITTLE_ENDIAN).getInt(0);
        log("File transfer version " + mVersion + (mVersion >= 5 ? " (fast transfer)" : " (classic)"));

        mGatt.setCharacteristicNotification(mRaw, true);
        BluetoothGattDescriptor cccd = mRaw.getDescriptor(CCCD);
        if (cccd == null) {
            throw new WatchException("The watch's file transfer channel can't send replies.");
        }
        cccd.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        mOpDone.drainPermits();
        if (!mGatt.writeDescriptor(cccd)) {
            throw new WatchException("Couldn't turn on replies from the watch.");
        }
        waitOp("enable notifications");
        log("Replies on");
    }

    /**
     * Ends this app's use of the connection. The UNA app keeps its own: Android
     * only drops the link when no app is using it.
     */
    public void close() {
        BluetoothGatt g = mGatt;
        mGatt = null;
        if (g != null) {
            try {
                g.disconnect();
                g.close();
            } catch (RuntimeException ignored) {
                // Already gone.
            }
        }
    }

    // -- File operations -------------------------------------------------------------

    /** Lists a folder, or returns null if the watch says it isn't there. */
    public List<Fts.Entry> listDir(String dir) throws IOException {
        mNotes.clear();
        send(Fts.listDir(dir));
        List<Fts.Entry> out = new ArrayList<Fts.Entry>();
        for (int guard = 0; guard < 1000; guard++) {
            byte[] n = next(Fts.LISTDIR_ENTRY, REPLY_TIMEOUT_MS);
            if (n == null) {
                throw new WatchException("The watch stopped answering while listing " + dir + ".");
            }
            Fts.Entry e = Fts.listEntry(n);
            if (e == null) {
                log("  unreadable entry: " + Fts.hex(n, 40));
                continue;
            }
            if (e.status != Fts.OK) {
                log("  LISTDIR " + dir + ": " + Fts.statusName(e.status));
                return null;
            }
            if (e.isTerminator()) {
                return out;
            }
            out.add(e);
        }
        throw new WatchException("Listing " + dir + " never ended.");
    }

    public int mkdir(String dir) throws IOException {
        mNotes.clear();
        send(Fts.mkdir(dir, nowNs()));
        return Fts.simpleStatus(next(Fts.MKDIR_STATUS, REPLY_TIMEOUT_MS), Fts.MKDIR_STATUS);
    }

    /**
     * Writes a whole file from offset 0. Stop-and-wait on a version-4 watch;
     * on version 5, a 2 KB credit window with go-back-N on a stall, as
     * Docs/BLE-File-Transfer-Service.md describes.
     */
    public void writeFile(String path, byte[] data) throws IOException {
        final long total = data.length;
        final int chunk = chunkSize();
        final int window = mVersion >= 5 ? Math.max(WRITE_WINDOW, chunk) : chunk;
        final long replyWait = mVersion >= 5 ? 1500 : 4000;

        mNotes.clear();
        send(Fts.write(path, 0, nowNs(), (int) total));
        byte[] first = next(Fts.WRITE_PACING, REPLY_TIMEOUT_MS);
        long acked = 0;
        if (first == null) {
            log("  no reply to WRITE; sending anyway");
        } else {
            Fts.Pacing p = Fts.pacing(first);
            if (p == null || p.status != Fts.OK) {
                throw new WatchException("The watch refused the file ("
                        + (p == null ? Fts.hex(first, 24) : Fts.statusName(p.status)) + ").");
            }
            acked = total - p.freeSpace;
        }

        long sent = acked;
        int stalls = 0;
        mListener.progress(acked, total);
        while (acked < total) {
            while (sent < total && sent - acked < window) {
                int len = (int) Math.min(chunk, total - sent);
                send(Fts.writeData((int) sent, data, (int) sent, len));
                sent += len;
            }
            byte[] n = next(Fts.WRITE_PACING, replyWait);
            if (n == null) {
                if (++stalls > MAX_STALLS) {
                    throw new WatchException("The watch stopped acknowledging at " + acked + " of " + total + " bytes.");
                }
                log("  no progress; resending from byte " + acked);
                sent = acked;
                continue;
            }
            Fts.Pacing p = Fts.pacing(n);
            if (p == null || p.status != Fts.OK) {
                throw new WatchException("The watch stopped the transfer ("
                        + (p == null ? Fts.hex(n, 24) : Fts.statusName(p.status)) + ").");
            }
            long a = total - p.freeSpace;
            if (a > acked) {
                acked = a;
                stalls = 0;
                mListener.progress(acked, total);
            }
        }
        // A late duplicate ACK must not be read as the next command's reply.
        drain(300);
    }

    /** CRC-32 and size the watch reports for a file, or null (version < 5, or no reply). */
    public Fts.Digest digest(String path) throws IOException {
        if (mVersion < 5) {
            return null;
        }
        mNotes.clear();
        send(Fts.digest(path));
        return Fts.digestStatus(next(Fts.DIGEST_STATUS, REPLY_TIMEOUT_MS));
    }

    public static long crc32(byte[] data) {
        CRC32 c = new CRC32();
        c.update(data, 0, data.length);
        return c.getValue();
    }

    // -- GATT plumbing ------------------------------------------------------------------

    private int chunkSize() {
        return Math.max(8, Math.min(244, mMtu - 3) - Fts.WRITE_DATA_HEADER);
    }

    private static long nowNs() {
        return System.currentTimeMillis() * 1000000L;
    }

    private void send(byte[] packet) throws IOException {
        BluetoothGatt g = mGatt;
        if (g == null || !mIsConnected) {
            throw new WatchException("The watch disconnected.");
        }
        mOpDone.drainPermits();
        boolean queued = false;
        for (int attempt = 0; attempt < 50 && !queued; attempt++) {
            queued = queueWrite(g, packet);
            if (!queued) {
                sleep(20);   // Android's GATT queue is busy; try again shortly
            }
        }
        if (!queued) {
            throw new WatchException("Android wouldn't send to the watch (Bluetooth busy).");
        }
        waitOp("write");
    }

    /**
     * Android 13 (API 33) added writeCharacteristic(characteristic, value, type),
     * which carries the bytes itself. The older form sends whatever the shared
     * characteristic object holds, and an incoming reply overwrites that, so
     * on API 33+ the new form is used. It is called by reflection because this
     * app compiles against API 23 so that it builds with Ubuntu's Android SDK.
     */
    private boolean queueWrite(BluetoothGatt g, byte[] packet) throws IOException {
        if (Build.VERSION.SDK_INT >= 33) {
            try {
                if (sWrite33 == null) {
                    sWrite33 = BluetoothGatt.class.getMethod("writeCharacteristic",
                            BluetoothGattCharacteristic.class, byte[].class, int.class);
                }
                Object rc = sWrite33.invoke(g, mRaw, packet,
                        BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
                return rc instanceof Integer && ((Integer) rc).intValue() == 0;   // BluetoothStatusCodes.SUCCESS
            } catch (InvocationTargetException e) {
                Throwable cause = e.getCause();
                if (cause instanceof RuntimeException) {
                    throw (RuntimeException) cause;   // e.g. SecurityException: no permission
                }
                throw new WatchException("Bluetooth write failed: " + cause);
            } catch (ReflectiveOperationException e) {
                // Fall through to the old form.
            }
        }
        mRaw.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
        mRaw.setValue(packet);
        return g.writeCharacteristic(mRaw);
    }

    /** The next notification with this command byte; others are logged and skipped. */
    private byte[] next(int command, long timeoutMs) throws IOException {
        long end = System.currentTimeMillis() + timeoutMs;
        while (true) {
            long left = end - System.currentTimeMillis();
            if (left <= 0) {
                return null;
            }
            byte[] n;
            try {
                n = mNotes.poll(left, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                throw new WatchException("Interrupted.");
            }
            if (n == null) {
                if (!mIsConnected) {
                    throw new WatchException("The watch disconnected.");
                }
                return null;
            }
            if (Fts.command(n) == command) {
                return n;
            }
            log("  skipped reply " + Fts.hex(n, 12));
        }
    }

    private void drain(long ms) {
        try {
            while (mNotes.poll(ms, TimeUnit.MILLISECONDS) != null) {
                // discard
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    private void waitOp(String what) throws IOException {
        if (!acquire(mOpDone, OP_TIMEOUT_MS)) {
            throw new WatchException("Timed out waiting for the watch (" + what + ").");
        }
        if (!mIsConnected) {
            throw new WatchException("The watch disconnected.");
        }
        if (mOpStatus != BluetoothGatt.GATT_SUCCESS) {
            throw new WatchException("Bluetooth error " + mOpStatus + " (" + what + ").");
        }
    }

    private static boolean acquire(Semaphore s, long ms) {
        try {
            return s.tryAcquire(ms, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return false;
        }
    }

    private static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    private void log(String line) {
        mListener.log(line);
    }

    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
        if (newState == BluetoothProfile.STATE_CONNECTED) {
            mIsConnected = true;
            mConnected.release();
        } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
            boolean was = mIsConnected;
            mIsConnected = false;
            if (was) {
                log("Disconnected (status " + status + ")");
            }
            // Wake anything waiting, so it sees the disconnect at once.
            mConnected.release();
            mOpDone.release();
        }
    }

    @Override
    public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
        if (status == BluetoothGatt.GATT_SUCCESS) {
            mMtu = mtu;
        }
        mOpStatus = BluetoothGatt.GATT_SUCCESS;   // a refused MTU is not fatal
        mOpDone.release();
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
        mOpStatus = status;
        mOpDone.release();
    }

    @Override
    public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic c, int status) {
        byte[] v = c.getValue();
        mReadValue = v == null ? null : v.clone();
        mOpStatus = status;
        mOpDone.release();
    }

    // Android 13+ form, with the value passed in (see onCharacteristicChanged below).
    public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic c, byte[] value, int status) {
        mReadValue = value == null ? null : value.clone();
        mOpStatus = status;
        mOpDone.release();
    }

    @Override
    public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic c, int status) {
        mOpStatus = status;
        mOpDone.release();
    }

    @Override
    public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor d, int status) {
        mOpStatus = status;
        mOpDone.release();
    }

    // Android 13+ calls the three-argument form, whose default implementation
    // calls the old one; overriding it (without super) sees each reply once,
    // with its own bytes. Older Android calls only the old form. The new form
    // can't carry @Override here because the app compiles against API 23.
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic c, byte[] value) {
        if (RAW.equals(c.getUuid()) && value != null) {
            mNotes.offer(value.clone());
        }
    }

    @Override
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic c) {
        if (RAW.equals(c.getUuid())) {
            byte[] v = c.getValue();
            if (v != null) {
                mNotes.offer(v.clone());
            }
        }
    }
}
