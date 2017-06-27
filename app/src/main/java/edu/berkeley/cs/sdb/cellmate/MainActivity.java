package edu.berkeley.cs.sdb.cellmate;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.util.Size;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Surface;

import com.splunk.mint.Mint;

public class MainActivity extends AppCompatActivity implements PreviewFragment.StateCallback, ControlFragment.StateCallback {
    private static final String MINT_API_KEY = "76da1102";




    @Override
    public void onObjectIdentified(String name, double x, double y, double size) {
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Mint.initAndStartSession(this, MINT_API_KEY);

        setContentView(R.layout.main_activity);

        if (savedInstanceState == null) {
            Size size = new Size(640, 480);
            Camera.getInstance(getApplicationContext(),size);


            PreviewFragment previewFragment = PreviewFragment.newInstance(size);
            getFragmentManager().beginTransaction().replace(R.id.preview_fragment, previewFragment).commit();

            ControlFragment controlFragment = ControlFragment.newInstance();
            getFragmentManager().beginTransaction().replace(R.id.task_fragment, controlFragment).commit();
        }

        // hide action bar
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayShowHomeEnabled(false);
        actionBar.setDisplayShowTitleEnabled(false);
    }



    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.settings:
                Intent intent = new Intent(this, SettingsActivity.class);
                startActivity(intent);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }


}
