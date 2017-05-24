package edu.berkeley.cs.sdb.cellmate;

import android.media.Image;
import android.os.AsyncTask;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
import java.util.concurrent.Semaphore;

import edu.berkeley.cs.sdb.bosswave.BosswaveClient;
import edu.berkeley.cs.sdb.bosswave.BosswaveResponse;
import edu.berkeley.cs.sdb.bosswave.BosswaveResult;
import edu.berkeley.cs.sdb.bosswave.ChainElaborationLevel;
import edu.berkeley.cs.sdb.bosswave.PayloadObject;
import edu.berkeley.cs.sdb.bosswave.PublishRequest;
import edu.berkeley.cs.sdb.bosswave.ResponseHandler;
import edu.berkeley.cs.sdb.bosswave.ResultHandler;
import edu.berkeley.cs.sdb.bosswave.SubscribeRequest;


public class BwPubImgTask extends AsyncTask<Void, Void, String> {
    private BosswaveClient mBosswaveClient;
    private String mTopic;
    private Image mImage;
    private PayloadObject.Type mType;
    private Listener mTaskListener;
    private Semaphore mSem;
    private String mResult;
    private double mFx;
    private double mFy;
    private double mCx;
    private double mCy;
    private double mPitch;
    private double mRoll;

    public interface Listener {
        void onResponse(String response);
    }

    public BwPubImgTask(BosswaveClient bosswaveClient, String topic, Image image, double fx, double fy, double cx, double cy,double pitch, double roll, Listener listener) {
        mBosswaveClient = bosswaveClient;
        mTopic = topic;
        mImage = image;
        mFx = fx;
        mFy = fy;
        mCx = cx;
        mCy = cy;
        mPitch = pitch;
        mRoll = roll;
        mTaskListener = listener;
        mSem = new Semaphore(0);
    }

    private ResponseHandler mResponseHandler = new ResponseHandler() {
        @Override
        public void onResponseReceived(BosswaveResponse response) {
            mResult = response.getStatus();
        }
    };


    private class ResponseErrorHandler implements ResponseHandler {
        @Override
        public void onResponseReceived(BosswaveResponse resp) {
            if (!resp.getStatus().equals("okay")) {
                throw new RuntimeException(resp.getReason());
            }
        }
    }

    private class TextResultHandler implements ResultHandler {
        @Override
        public void onResultReceived(BosswaveResult rslt) {
            byte[] messageContent = rslt.getPayloadObjects().get(0).getContent();
            String msg = new String(messageContent, StandardCharsets.UTF_8);
            mResult = msg;
            mSem.release();
        }
    }


    @Override
    protected String doInBackground(Void... voids) {
        try {
            String header = "Cellmate Image";
            String identity = UUID.randomUUID().toString().substring(0, 10);
            // Subscribe to a Bosswave URI
            SubscribeRequest.Builder subsbuilder = new SubscribeRequest.Builder(mTopic + "/" + identity);
            SubscribeRequest subsRequest = subsbuilder.build();
            mBosswaveClient.subscribe(subsRequest, new ResponseErrorHandler(), new TextResultHandler());
            PublishRequest.Builder builder = new PublishRequest.Builder(mTopic);
            builder.setAutoChain(true);
            builder.setChainElaborationLevel(ChainElaborationLevel.PARTIAL);
            builder.clearPayloadObjects();
            PayloadObject poHeader = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), header.getBytes());
            PayloadObject poIdentity = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), identity.getBytes());
            PayloadObject poData = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), ImgCodec.compressJPEG(mImage));
            PayloadObject poWidth = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Integer.toString(mImage.getWidth()).getBytes());
            PayloadObject poHeight = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Integer.toString(mImage.getHeight()).getBytes());
            PayloadObject poFx = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Double.toString(mFx).getBytes());
            PayloadObject poFy = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Double.toString(mFy).getBytes());
            PayloadObject poCx = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Double.toString(mCx).getBytes());
            PayloadObject poCy = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Double.toString(mCy).getBytes());
            PayloadObject poPitch = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Double.toString(mPitch).getBytes());
            PayloadObject poRoll = new PayloadObject(new PayloadObject.Type(new byte[]{64, 0, 0, 0}), Double.toString(mRoll).getBytes());
            builder.addPayloadObject(poHeader);
            builder.addPayloadObject(poIdentity);
            builder.addPayloadObject(poData);
            builder.addPayloadObject(poWidth);
            builder.addPayloadObject(poHeight);
            builder.addPayloadObject(poFx);
            builder.addPayloadObject(poFy);
            builder.addPayloadObject(poCx);
            builder.addPayloadObject(poCy);
            builder.addPayloadObject(poPitch);
            builder.addPayloadObject(poRoll);
            PublishRequest request = builder.build();
            mBosswaveClient.publish(request, mResponseHandler);
        } catch (IOException e) {
            e.printStackTrace();
        }

        try {
            mSem.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        return mResult;
    }

    @Override
    protected void onPostExecute(String response) {
        mImage.close();
        mTaskListener.onResponse(response);
    }
}
