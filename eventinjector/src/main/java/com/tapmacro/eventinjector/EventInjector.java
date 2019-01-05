package com.tapmacro.eventinjector;

import android.content.Context;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class EventInjector {


    private static final String TAG = "EventInjector";

    public static native void nativeTest();

    private native static int ScanFiles();
    private native static int OpenDev(int devid);
    private native static int RemoveDev(int devid);
    private native static String getDevPath(int devid);
    private native static String getDevName(int devid);
    private native static int PollDev(int devid);

    private native static int getType();
    private native static int getCode();
    private native static int getValue();
    private native static int intSendEvent(int devid, int type, int code, int value);

    private native static boolean isTouchDev(int index);

    static {
        System.loadLibrary("event-injector");
    }


    private static class InputDevice {
        int id;
        String path;
        String name;

        InputDevice(int id, String path) {
            this.id = id;
            this.path = path;
        }
    }



    private static class RawEvent{
        int devId;
        int type;
        int code;
        int value;

        public RawEvent(int devId, int type, int code, int value) {
            this.devId = devId;
            this.type = type;
            this.code = code;
            this.value = value;
        }
    }

    private Context context;
    private List<InputDevice> deviceList = new ArrayList<>();
    private Thread recordThread;
    private boolean recording;
    private int touchIndex;


    private List<RawEvent> testRawEvent = new ArrayList<>();

    public EventInjector(Context context) {
        this.context = context;
    }

    public int init(){
        deviceList.clear();
        int n = ScanFiles();
        for (int i = 0; i < n; i++){
            InputDevice device = new InputDevice(i, getDevPath(i));
            OpenDev(device.id);
            device.name = getDevName(i);
            if(isTouchDev(i)){
                touchIndex = i;
            }
            deviceList.add(device);
        }
        return n;
    }



    public void startTouchRecord(){
        if(recording) {
            return;
        }
        recording = true;

        testRawEvent.clear();
        recordThread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (recording){
                    int result = EventInjector.PollDev(touchIndex);
                    if(result == 0) {
                        int eCode = EventInjector.getCode();
                        int eType = EventInjector.getType();
                        int eValue = EventInjector.getValue();
                        testRawEvent.add(new RawEvent(touchIndex, eType, eCode, eValue));
                        Log.d(TAG, String.format("touch event: type: %d, code: %d, value: %d", eType, eCode, eValue));
                    }
                }
            }
        });
        recordThread.start();
    }

    public void stopTouchRecord(){
        recording = false;
        if(recordThread != null) {
            recordThread.interrupt();
            recordThread = null;
        }
    }

    public void sendRawTouchEvent(int type, int code, int value){
        if(touchIndex < 0){
            throw new IllegalStateException("not init");
        }
        intSendEvent(touchIndex, type, code, value);
    }


    public void testPlayRawEvent(){
        for(RawEvent e : testRawEvent) {
            sendRawTouchEvent(e.type, e.code, e.value);
        }
    }

    public void release(){
        for(InputDevice dev : deviceList) {
            RemoveDev(dev.id);
        }
        deviceList.clear();
    }
}
