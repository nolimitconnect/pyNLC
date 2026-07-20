package org.nolimitconnect.nolimitconnect;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.Service;
import android.os.Build;
import android.os.Bundle;
import android.content.Context;
import android.content.Intent;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.media.Image;
import android.media.ImageReader;
import android.os.Binder;
import android.os.IBinder;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.Range;
import androidx.annotation.Nullable;
import java.nio.ByteBuffer;
import java.util.Arrays;

public class Camera2Service extends Service {
    private static final String TAG = "NLC Camera2Service";
    private static final String PERMISSION_FRAGMENT_TAG = "nlc.permission.fragment";
    private static boolean sNativeLoaded = false;

    private Handler m_MainThreadHandler = null;

    // native methods
    public static native void camPermissionResult(boolean granted);
    public static native void micPermissionResult(boolean granted);
    public native void camServiceStarted();
    public native void camServiceStopped();
    public native boolean canProcessCamCapture();
    public native void processCamCapture(   int imgWidth, int imgHeight, ByteBuffer y, ByteBuffer u, ByteBuffer v,
                                            int yPixelStride, int yRowStride,
                                            int uPixelStride, int uRowStride,
                                            int vPixelStride, int vRowStride );

    protected CameraManager m_CameraManager = null;
    protected CameraDevice m_CameraDevice = null;
    protected ImageReader m_ImageReader = null;
    protected String m_CameraId;
    private CameraCaptureSession m_CaptureSession = null;

    public Camera2Service()
    {
        Log.d(TAG, "Camera2Service() with NO context Called");
    }

    public Camera2Service(Context context)
    {
        Log.d(TAG, "Camera2Service() WITH context Called");
    }

    private static synchronized boolean ensureNativeLoaded() {
        if (sNativeLoaded) {
            return true;
        }

        final String[] candidateLibNames = {
            "nolimitconnect_arm64-v8a",
            "nolimitconnect"
        };

        for (String libName : candidateLibNames) {
            try {
                System.loadLibrary(libName);
                sNativeLoaded = true;
                Log.i(TAG, "Loaded native library: " + libName);
                return true;
            } catch (UnsatisfiedLinkError e) {
                Log.w(TAG, "Unable to load native library: " + libName, e);
            }
        }

        Log.e(TAG, "Failed to load native library for Camera2Service JNI.");
        return false;
    }

    public static void startCamServiceStatic(Context context) {
        Log.d(TAG, "startCamServiceStatic Called");
        // for unknown reason foreground service cause gradlelock and other issues on some devices
        // do not run in forground
        // context.startForegroundService(new Intent(context, Camera2Service.class));

        context.startService( new Intent( context, Camera2Service.class ) );
        Log.d(TAG, "startCamServiceStatic Done");
    }

    public static void stopCamServiceStatic(Context context) {
        Log.d(TAG, "stopCamServiceStatic Called");
        try {
            context.stopService(new Intent(context, Camera2Service.class));
        } catch (Exception e) {
            Log.e(TAG, "stopCamServiceStatic failed", e);
        }
        Log.d(TAG, "stopCamServiceStatic Done");
    }

    public static void requestPermissionStatic(Activity activity, String permission, int requestCode, String callbackType) {
        if (activity == null || permission == null || permission.isEmpty()) {
            Log.e(TAG, "requestPermissionStatic invalid activity or permission");
            return;
        }

        String callback = (callbackType == null || callbackType.isEmpty()) ? "camera" : callbackType;

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            dispatchPermissionResult(callback, true);
            return;
        }

        FragmentManager fragmentManager = activity.getFragmentManager();
        Fragment existing = fragmentManager.findFragmentByTag(PERMISSION_FRAGMENT_TAG);
        if (existing != null) {
            Log.d(TAG, "requestPermissionStatic request already pending");
            return;
        }

        PermissionRequestFragment fragment = PermissionRequestFragment.newInstance(permission, requestCode, callback);
        fragmentManager
                .beginTransaction()
                .add(fragment, PERMISSION_FRAGMENT_TAG)
                .commitAllowingStateLoss();
    }

    private static void dispatchPermissionResult(String callbackType, boolean granted) {
        if (!ensureNativeLoaded()) {
            return;
        }

        try {
            if ("microphone".equals(callbackType)) {
                micPermissionResult(granted);
            } else {
                camPermissionResult(granted);
            }
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Permission result JNI call failed", e);
        }
    }

    public static class PermissionRequestFragment extends Fragment {
        private static final String ARG_PERMISSION = "arg_permission";
        private static final String ARG_REQUEST_CODE = "arg_request_code";
        private static final String ARG_CALLBACK_TYPE = "arg_callback_type";

        static PermissionRequestFragment newInstance(String permission, int requestCode, String callbackType) {
            PermissionRequestFragment fragment = new PermissionRequestFragment();
            Bundle args = new Bundle();
            args.putString(ARG_PERMISSION, permission);
            args.putInt(ARG_REQUEST_CODE, requestCode);
            args.putString(ARG_CALLBACK_TYPE, callbackType);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            Bundle args = getArguments();
            if (args == null) {
                notifyPermissionResult("camera", false);
                removeSelf();
                return;
            }

            String permission = args.getString(ARG_PERMISSION, "");
            int requestCode = args.getInt(ARG_REQUEST_CODE, 0);
            String callbackType = args.getString(ARG_CALLBACK_TYPE, "camera");
            if (permission.isEmpty()) {
                notifyPermissionResult(callbackType, false);
                removeSelf();
                return;
            }

            requestPermissions(new String[]{ permission }, requestCode);
        }

        @Override
        public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);

            boolean granted = grantResults != null
                    && grantResults.length > 0
                    && grantResults[0] == android.content.pm.PackageManager.PERMISSION_GRANTED;

            Bundle args = getArguments();
            String callbackType = args != null ? args.getString(ARG_CALLBACK_TYPE, "camera") : "camera";
            notifyPermissionResult(callbackType, granted);
            removeSelf();
        }

        private void notifyPermissionResult(String callbackType, boolean granted) {
            dispatchPermissionResult(callbackType, granted);
        }

        private void removeSelf() {
            Activity activity = getActivity();
            if (activity == null || !isAdded()) {
                return;
            }

            activity.getFragmentManager()
                    .beginTransaction()
                    .remove(this)
                    .commitAllowingStateLoss();
        }
    }

    // Binder class to allow clients to bind to the service
    private final IBinder localBinder = new LocalBinder();

    public class LocalBinder extends Binder {
        Camera2Service getService() {
            return Camera2Service.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind Camera Service Called");
        return localBinder;
    }

    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate Camera Service Created");
        super.onCreate();
        // Initialize the Handler for running tasks on the main thread
        m_MainThreadHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);

        if (!ensureNativeLoaded()) {
            stopSelf(startId);
            return START_NOT_STICKY;
        }

        m_CameraManager = (CameraManager) getSystemService(CAMERA_SERVICE);
        // must start quickly or will shutdown. use runnable
        m_MainThreadHandler.post(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "onStartCommand Called");
                try {
                    camServiceStarted();
                } catch (UnsatisfiedLinkError e) {
                    Log.e(TAG, "camServiceStarted JNI call failed", e);
                    stopSelf();
                }
            }
        });

        // Keep service running until explicitly stopped
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "Camera Service Destroyed");
        internalStopCameraCapture();
        if (sNativeLoaded) {
            try {
                camServiceStopped();
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "camServiceStopped JNI call failed", e);
            }
        }

        if (m_MainThreadHandler != null) {
            m_MainThreadHandler.removeCallbacksAndMessages(null);
            m_MainThreadHandler = null;
        }

        super.onDestroy();
    }

    public void onConfigured(CameraCaptureSession session) {
        Log.d(TAG, "capture session onConfigured");
        m_CaptureSession = session;
    }

    public int getValue() {
        return 10;
    }

    public String[] getCameraIdList() {
        try {
            // Usually the first camera in list is rear-facing
            return m_CameraManager.getCameraIdList();
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }

        String[] emptyList = new String[0]; // Creates an empty array
        return emptyList;
    }

    public boolean isCameraBackFacing(String cameraId) {
        boolean isBackFacing = false;
        try {
            CameraCharacteristics characteristics = m_CameraManager.getCameraCharacteristics( cameraId );
            int facing = characteristics.get(CameraCharacteristics.LENS_FACING);
            if ( facing == CameraCharacteristics.LENS_FACING_BACK ) {
                isBackFacing = true;
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }

        return isBackFacing;
    }

    public boolean startCameraCapture(String cameraId)
    {
        m_CameraId = cameraId;
        Log.d(TAG, "**** begin startCameraCapture " + cameraId );
        // has to run on main thread
        m_MainThreadHandler.post(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "**** main thread openCamera " + cameraId );
                openCamera( m_CameraId );
            }
        });


        Log.d(TAG, "**** end startCameraCapture " + cameraId );
        return true;
    }

    public void stopCameraCapture()
    {
        Log.d(TAG, "**** begin stopCameraCapture " + m_CameraId );
        // has to run on main thread
        m_MainThreadHandler.post(new Runnable() {
            @Override
            public void run() {
                internalStopCameraCapture();
            }
       });

        Log.d(TAG, "**** end stopCameraCapture " + m_CameraId );
    }

    public void internalStopCameraCapture()
    {
        Log.d(TAG, "**** internalStopCameraCapture " + m_CameraId );

        if (m_CameraDevice != null) {
            m_CameraDevice.close();
            m_CameraDevice = null;
        }

        if (m_ImageReader != null) {
            m_ImageReader.close();
            m_ImageReader = null;
        }

        if (m_CaptureSession != null) {
            try {
                m_CaptureSession.stopRepeating();
                m_CaptureSession.abortCaptures();
                m_CaptureSession.close();
            } catch (CameraAccessException e) {
                Log.e(TAG, "Error stopping capture session", e);
            }
            m_CaptureSession = null;
        }
    }

    private final CameraDevice.StateCallback m_CameraStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice cameraDevice) {
            Log.d(TAG, "camera onOpened");
            m_CameraDevice = cameraDevice;
            createCameraCaptureSession( cameraDevice );
        }

        @Override
        public void onDisconnected(CameraDevice cameraDevice) {
            Log.d(TAG, "camera onDisconnected");
            cameraDevice.close();
            m_CameraDevice = null;
        }

        @Override
        public void onError(CameraDevice cameraDevice, int error) {
            Log.e(TAG, "camera onError " + error );
            cameraDevice.close();
            m_CameraDevice = null;
        }
    };

    private final CameraCaptureSession.StateCallback m_SessionStateCallback = new CameraCaptureSession.StateCallback() {
        @Override
        public void onReady(CameraCaptureSession session) {
            Log.d(TAG, "capture session onReady");
            try {
                CaptureRequest request = createCameraCaptureRequest();
                if (request == null) {
                    Log.w(TAG, "Skipping repeating request because capture request is null");
                    return;
                }

                session.setRepeatingRequest(request, null, null);
            } catch (CameraAccessException e) {
                Log.e(TAG, e.getMessage());
            }
        }

        @Override
        public void onConfigured(CameraCaptureSession session) {
            Log.d(TAG, "capture session onConfigured");
        }

        @Override
        public void onConfigureFailed( CameraCaptureSession session) {
            Log.d(TAG, "capture session onConfigureFailed");
        }
    };

    public void createCameraCaptureSession(CameraDevice camera) {
        try {
            camera.createCaptureSession(
                Arrays.asList(m_ImageReader.getSurface()),
                m_SessionStateCallback,
                null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }
    protected CaptureRequest createCameraCaptureRequest() {
        try {
            if (m_CameraDevice == null) {
                Log.w(TAG, "createCameraCaptureRequest called with null m_CameraDevice");
                return null;
            }

            CaptureRequest.Builder builder = m_CameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
            CameraCharacteristics characteristics = m_CameraManager.getCameraCharacteristics(m_CameraId);

            Range<Integer>[] fpsRanges = characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
            Range<Integer> selectedRange = null;

            if (fpsRanges != null) {
                for (Range<Integer> range : fpsRanges) {
                    if (range.getLower() <= 10 && range.getUpper() >= 10) {
                        selectedRange = Range.create(10, 10); // Fix to 10 FPS
                        break;
                    }
                }

                if (selectedRange == null) {
                    for (Range<Integer> range : fpsRanges) {
                        if (range.getLower() <= 12 && range.getUpper() >= 12) {
                            selectedRange = Range.create(12, 12); // Fix to 12 FPS
                            break;
                        }
                    }
                }

                if (selectedRange == null) {
                    // Fallback: pick the closest range below 30
                    for (Range<Integer> range : fpsRanges) {
                        if (range.getLower() >= 12 && range.getUpper() <= 30) {
                            selectedRange = range;
                            break;
                        }
                    }
                }

                if (selectedRange != null) {
                    builder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, selectedRange);
                    Log.d(TAG, "Selected FPS Range: " + selectedRange);
                } else {
                    Log.w(TAG, "No suitable FPS range found. Using default.");
                }
            }

            builder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO);
            builder.set(CaptureRequest.NOISE_REDUCTION_MODE, CaptureRequest.NOISE_REDUCTION_MODE_OFF);
            builder.set(CaptureRequest.EDGE_MODE, CaptureRequest.EDGE_MODE_OFF);
            builder.set(CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE, CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE_OFF);
            builder.set(CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE, CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF);

            builder.addTarget(m_ImageReader.getSurface());

            return builder.build();
        } catch (CameraAccessException e) {
            Log.e(TAG, "CameraAccessException in createCameraCaptureRequest: " + e.getMessage());
            return null;
        }
    }

    protected ImageReader.OnImageAvailableListener onImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            Image img = reader.acquireNextImage();
            if( img != null )
            {
                if (canProcessCamCapture())
                {
                    processImage(img);
                }

                img.close();
            }
        }
    };

    private boolean openCamera(String cameraId) {
        boolean result = false;
        try {
            internalStopCameraCapture(); // in case was already running

            if( m_ImageReader == null )
            {
                m_ImageReader = ImageReader.newInstance(320, 240, ImageFormat.YUV_420_888, 1 /* number of images buffered */);
                m_ImageReader.setOnImageAvailableListener(onImageAvailableListener, null);
            }

            m_CameraId = cameraId;
            m_CameraManager.openCamera( cameraId, m_CameraStateCallback, null );

            result = true;

        } catch (Exception e) {
            Log.e(TAG, "Error opening camera", e);
        }

        return result;
    }

    private void processImage(Image image) {
        int width = image.getWidth();
        int height = image.getHeight();

        final Image.Plane[] planes = image.getPlanes();
        Image.Plane yPlane = planes[0];
        Image.Plane uPlane = planes[1];
        Image.Plane vPlane = planes[2];

        // Use sliced buffers so JNI sees the plane data starting at the current plane position.
        ByteBuffer yBuffer = yPlane.getBuffer().slice();
        ByteBuffer uBuffer = uPlane.getBuffer().slice();
        ByteBuffer vBuffer = vPlane.getBuffer().slice();

        processCamCapture( width, height,
                        yBuffer,
                        uBuffer,
                        vBuffer,
                        yPlane.getPixelStride(),
                        yPlane.getRowStride(),
                        uPlane.getPixelStride(),
                        uPlane.getRowStride(),
                        vPlane.getPixelStride(),
                        vPlane.getRowStride() );
    }
}
