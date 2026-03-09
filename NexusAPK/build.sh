#!/data/data/com.termux/files/usr/bin/bash
set -e
NDK=~/android-ndk-r25c
PROJ=~/NexusAPK
ENGINE=~/NovaEngine
OUT=$PROJ/build
ANDROID_JAR=~/android.jar
echo "=== Nexus Engine 2.0 APK Builder ==="
mkdir -p $OUT/lib/arm64-v8a
echo "[1/4] Compiling native glue..."
clang -target aarch64-linux-android21 -fPIC -c $NDK/sources/android/native_app_glue/android_native_app_glue.c -I$NDK/sources/android/native_app_glue -o $OUT/native_glue.o
echo "[1b] Compiling C++ engine..."
clang++ \
    --target=aarch64-linux-android21 \
    -std=c++17 -O2 -shared -fPIC \
    -DANDROID -DIMGUI_IMPL_OPENGL_ES3 \
    -I$ENGINE \
    -I$ENGINE/imgui \
    -I$ENGINE/imgui/backends \
    -I$NDK/sources/android/native_app_glue \
        $PROJ/main_android.cpp \
    $ENGINE/imgui/imgui.cpp \
    $ENGINE/imgui/imgui_draw.cpp \
    $ENGINE/imgui/imgui_widgets.cpp \
    $ENGINE/imgui/imgui_tables.cpp \
    $ENGINE/imgui/backends/imgui_impl_android.cpp \
    $ENGINE/imgui/backends/imgui_impl_opengl3.cpp \
    $OUT/native_glue.o -lGLESv3 -lEGL -landroid -llog -lm \
    -o $OUT/lib/arm64-v8a/libnexusengine.so
echo "[2/4] Packaging APK..."
cd $OUT
aapt package -f --min-sdk-version 21 --target-sdk-version 33 \
    -M $PROJ/AndroidManifest.xml \
    -I $ANDROID_JAR \
    -F $OUT/nexus_unsigned.apk
aapt add $OUT/nexus_unsigned.apk lib/arm64-v8a/libnexusengine.so
echo "[3/4] Signing APK..."
apksigner sign \
    --ks ~/nexus.keystore \
    --ks-alias nexus \
    --ks-pass pass:nexus123 \
    --key-pass pass:nexus123 \
    --out $OUT/NexusEngine2.apk \
    $OUT/nexus_unsigned.apk
echo "[4/4] Copying to Downloads..."
cp $OUT/NexusEngine2.apk /storage/emulated/0/Download/NexusEngine2.apk
echo "=== BUILD COMPLETE! APK is in your Downloads! ==="
