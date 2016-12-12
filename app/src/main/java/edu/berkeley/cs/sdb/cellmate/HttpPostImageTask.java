package edu.berkeley.cs.sdb.cellmate;

import android.media.Image;
import android.os.AsyncTask;
import android.util.Log;

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class HttpPostImageTask extends AsyncTask<Void, Void, String> {
    private static final String LOG_TAG = "cellmate";

    private OkHttpClient mHttpClient;
    private String mUrl;
    private Image mImage;
    private double mFx;
    private double mFy;
    private double mCx;
    private double mCy;
    private Listener mListener;

    public interface Listener {
        void onResponse(String result); // null means network error
    }

    public HttpPostImageTask(OkHttpClient httpClient, String url, Image image, double fx, double fy, double cx, double cy, Listener listener) {
        mHttpClient = httpClient;
        mUrl = url;
        mImage = image;
        mFx = fx;
        mFy = fy;
        mCx = cx;
        mCy = cy;
        mListener = listener;
    }

    @Override
    protected String doInBackground(Void... voids) {
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        RequestBody requestBody = new MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("file", timeStamp, RequestBody.create(MediaType.parse("application/octet-stream"), Imgcodecs.compressJPEG(mImage)))
                .addFormDataPart("fx", Double.toString(mFx))
                .addFormDataPart("fy", Double.toString(mFy))
                .addFormDataPart("cx", Double.toString(mCx))
                .addFormDataPart("cy", Double.toString(mCy))
                .build();
        Request request = new Request.Builder()
                .url(mUrl)
                .post(requestBody)
                .build();

        String result = null;
        try {
            Response response = mHttpClient.newCall(request).execute();
            if (response.isSuccessful()) {
                result = response.body().string();
            } else {
                Log.e(LOG_TAG, "HTTP Error " + response.code() + ":" + response.message());
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return result; // null means network error
    }

    @Override
    protected void onPostExecute(String result) {
        mImage.close();
        mListener.onResponse(result);
    }
}
