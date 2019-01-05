package com.tools.luaengine;

import android.content.Context;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.tools.utils.SystemUtils;

import org.keplerproject.luajava.LuaState;
import org.keplerproject.luajava.LuaStateFactory;

import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    private LuaState lua = null;
    private TextView textView;

    private void requestRootPermission(){
        String[] cmds = new String[3];
        cmds[0] = "setenforce 0" + "\n";
        cmds[1] = "chmod 777 /dev/input/*" + "\n";
        cmds[2] = "chmod 777 /dev/graphics/fb0" + "\n";
        SystemUtils.rootPermission(cmds);
    }

    private void testlua(){
        lua = LuaStateFactory.newLuaState();

        lua.openLibs();
        lua.LdoString(readAssetsTxt(this, "test.lua"));

        TestJavaFunction testJava = new TestJavaFunction(lua);
        testJava.init(this);
        testJava.register();

        lua.getGlobal("lua_main");
        lua.pushJavaObject(this.getApplicationContext());
        lua.pushString("\nHello Lua");

        lua.pcall(2, 0, 0);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        requestRootPermission();
        Button myText = findViewById(R.id.luatest);
        myText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(
                        new Runnable() {
                            @Override
                            public void run() {
                                testlua();
                            }
                        }
                ).start();
            }
        });
    }

    public static String readAssetsTxt(Context context, String fileName) {
        try {
            InputStream is = context.getAssets().open(fileName);
            int size = is.available();
            // Read the entire asset into a local byte buffer.
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            // Convert the buffer into a string.
            String text = new String(buffer, "utf-8");
            // Finally stick the string into the text view.
            return text;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "err";
    }
}
