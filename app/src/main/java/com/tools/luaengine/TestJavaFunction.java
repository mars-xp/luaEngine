package com.tools.luaengine;

import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;

import org.keplerproject.luajava.JavaFunction;
import org.keplerproject.luajava.LuaException;
import org.keplerproject.luajava.LuaState;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.LogRecord;


/**
 * 这是 Lua 调用 java 函数的 demo. 首先把要执行的函数包装成一个类，继承 {@link JavaFunction}，
 * 在 {@link #execute()} 中执行操作，可获取 Lua 传入的参数或压入返回值。
 * <p>
 * This is a demo that Lua calls the java function.
 * First wrap the function to be a class which extends {@link JavaFunction}.
 * Then do something in {@link #execute()}.
 * You can get the parameters passed in by Lua or press the return value.
 */
public class TestJavaFunction extends JavaFunction {

    private Context mContext;
    private Handler mHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch(msg.what){
                case 1:
                    startSettins();
                    break;
                default:
                    break;
            }
        }
    };

    public void init(Context context){
        mContext = context;
    }

    public TestJavaFunction(LuaState luaState) {
        super(luaState);
    }

    @Override
    public int execute() {
        // 获取Lua传入的参数，注意第一个参数固定为上下文环境。
        // Getting the parameters passed in by Lua
        // Notice that the first argument is lua context.
        int x = L.toInteger(2);
        int y = L.toInteger(3);
        Log.v("lua-test", "x is "+x+" y is "+y);
        mHandler.sendEmptyMessage(1);
        return 1; // 返回值的个数 // Number of return values
    }

    public void register() {
        try {
            // 注册为 Lua 全局函数
            // Register as a Lua global function
            register("touchDown");
            register("touchUp");
        } catch (LuaException e) {
            e.printStackTrace();
        }
    }

    private void startSettins(){
        Intent vIntent = new Intent();
        vIntent.setAction(Settings.ACTION_SETTINGS);
        vIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(vIntent);
    }

}

