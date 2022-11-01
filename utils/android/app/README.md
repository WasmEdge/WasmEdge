# Android Application Sample for WasmEdge

## Android SDK Components
* platforms;android-31
* build-tools;31.0.0
* cmake;3.22.1
* ndk;23.1.7779620

You could deploy these components via [Android Studio](https://developer.android.com/studio/intro/update#sdk-manager) or [sdkmanager](https://developer.android.com/studio/command-line/sdkmanager) command line tool.

## Building Project with Gradle
1. Setup environment variable `ANDROID_HOME=path/to/your/android/sdk`
2. Run Command `./gradlew assembleRelease`
3. Sign your APK file with `apksigner`
    > apk file location `./app/build/outputs/apk/release`
    > `apksigner` location `$ANDROID_HOME/build-tools/$VERSION/apksigner` 

## Building Project with Android Studio
Open this folder with [Android Studio](https://developer.android.com/studio) 2020.3.1 or later 

For Release APK, click `Menu -> Build -> Generate Signed Bundle/APK`, select APK, setup keystore configuration and wait for build finished.
