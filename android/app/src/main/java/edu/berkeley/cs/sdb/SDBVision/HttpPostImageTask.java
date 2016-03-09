package edu.berkeley.cs.sdb.SDBVision;

import android.graphics.ImageFormat;
import android.media.Image;
import android.os.AsyncTask;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;

import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class HttpPostImageTask extends AsyncTask<Void, Void, String> {
    private static final String LOG_TAG = "SDBVision";

    private static final MediaType MEDIA_TYPE_JPEG = MediaType.parse("application/octet-stream");

    private OkHttpClient mHttpClient;
    private String mUrl;
    private Image mImage;
    private Listener mListener;

    public interface Listener {
        void onResponse(String response);
    }

    public HttpPostImageTask(OkHttpClient httpClient, String url, Image image, Listener listener) {
        mHttpClient = httpClient;
        mUrl = url;
        mImage = image;
        mListener = listener;
    }

    /**
     * Takes an Android Image in the YUV_420_888 format and returns a byte array.
     * ref: http://stackoverflow.com/questions/30510928/convert-android-camera2-api-yuv-420-888-to-rgb
     *
     * @param image Image in the YUV_420_888 format
     * @return bytes that contains the image data in greyscale
     */
    private static byte[] imageToBytes(Image image) {
        if (image.getFormat() != ImageFormat.YUV_420_888) {
            return null;
        }

        int bytesPerPixel = ImageFormat.getBitsPerPixel(ImageFormat.YUV_420_888) / 8;
        Image.Plane yPlane = image.getPlanes()[0]; // we only need a gray picture
        int pixelStride = yPlane.getPixelStride();
        if (bytesPerPixel != 1 || pixelStride != 1) { // they are guaranteed to be both 1 in Y plane
            throw new RuntimeException("Wrong image format");
        }

        ByteBuffer buffer = yPlane.getBuffer();
        int width = image.getWidth();
        int height = image.getHeight();
        byte[] data = new byte[width * height];
        buffer.get(data);

        return data;
    }

    @Override
    protected String doInBackground(Void... voids) {
        byte[] bytes = imageToBytes(mImage);
        mImage.close();
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        RequestBody requestBody = new MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("file", timeStamp, RequestBody.create(MEDIA_TYPE_JPEG, bytes))
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

        if (result != null && !result.trim().equals("")) {
            return result.trim();
        }

        return null;
    }

    @Override
    protected void onPostExecute(String response) {
        mListener.onResponse(response);
    }
}