mkdir -p downloads
cd downloads

wget -c https://github.com/SoftwareDefinedBuildings/bw2android/releases/download/v0.1/bw2android-0.1-all.jar -P ../app/libs --no-check-certificate

wget -c https://github.com/opencv/opencv/releases/download/3.1.0/OpenCV-3.1.0-android-sdk.zip --no-check-certificate
if [ ! -d OpenCV-android-sdk ]; then
    unzip OpenCV-3.1.0-android-sdk.zip
fi

mkdir -p ../app/src/main/3rdparty
rsync -avz ./OpenCV-android-sdk/sdk/native/3rdparty/ ../app/src/main/3rdparty

mkdir -p ../app/src/main/jni/include
rsync -avz ./OpenCV-android-sdk/sdk/native/jni/include/ ../app/src/main/jni/include

mkdir -p ../app/src/main/jniLibs
rsync -avz ./OpenCV-android-sdk/sdk/native/libs/ ../app/src/main/jniLibs

mkdir -p ../openCVLibrary310
cp ./OpenCV-android-sdk/sdk/java/lint.xml ../openCVLibrary310

mkdir -p ../openCVLibrary310/src/main/java
rsync -avz --exclude 'org/opencv/engine/' ./OpenCV-android-sdk/sdk/java/src/ ../openCVLibrary310/src/main/java

mkdir -p ../openCVLibrary310/src/main/aidl/org/opencv/engine
rsync -avz ./OpenCV-android-sdk/sdk/java/src/org/opencv/engine/ ../openCVLibrary310/src/main/aidl/org/opencv/engine

mkdir -p ../openCVLibrary310/src/main/res
rsync -avz ./OpenCV-android-sdk/sdk/java/res/ ../openCVLibrary310/src/main/res

cd ..
