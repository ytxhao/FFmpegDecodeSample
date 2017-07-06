package ican.ytx.com.ffmpegdecodesample;

import android.content.res.AssetManager;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("faac");
        System.loadLibrary("x264");

        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("postproc");
        System.loadLibrary("avcodec");
        System.loadLibrary("avformat");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("native-lib");
    }


    private Button bt;
    private String FILE_NAME = "titanic.mkv";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        bt = (Button) findViewById(R.id.bt);
        bt.setOnClickListener(this);

        final String filePath = Environment.getExternalStorageDirectory()
                .getAbsolutePath() + "/";
        Utils.CopyAssets(this,"video",filePath);
        FILE_NAME = filePath+FILE_NAME;
    }


    public native void startDecodeVideo(AssetManager assetMgr, String filename);

    @Override
    public void onClick(View v) {
        int id = v.getId();

        switch (id){
            case R.id.bt:
                startDecodeVideo(getResources().getAssets(),FILE_NAME);
                break;
        }
    }
}
