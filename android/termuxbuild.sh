#  enter command to build launcher:
#      bash build.sh

# IMPORTANT: LAUNCH THIS SCRIPT ONLY FROM
# SCRIPT LOCATION, OR YOU WILL FAIL BUILD

# tips:
#
# edit libname variable, for exammple LIBNAME=mylib
#
# open in nano app/src/main/java/su/xash/hlsdk/MainActivity.java
# and add .putExtra("argv", "-console -dll @mylib") line in code, so launcher
# will launch your library instead of default hl lib, also replace mylib with
# your LIBNAME variable, for example .putExtra("argv", "-console -dll @mymod")
#
# replace su.xash.hlsdk with your package name(for example su.xash.mymod)
# in this script and in AndroidManifest.xml file
# and do the same in app/src/main/java/su/xash/hlsdk/MainActivity.java
# (you should also replace su/xash/hlsdk folders name regarding your package name
# example: su/xash/mymod
#
# add android.jar file and build folder in .gitignore file before pushing changes
# if you forked this repo and you want to push it to github
#
# if you see cmake build failed: toolchain not found error,
# you should delete build directory in hlsdk
#
# if downloading/extrscting process of ndk was interrupted,
# delete bit ndk directory and ndk archive file and try again,
# for android.jar same rule
#

#envars
##########
WPATH=app/src/main
AJAR=./android.jar
RT=~/../usr/lib/rt.jar
AN=Launcher
KEYSTORE=keyst.keystore
KEYPASS=verystrongpassword
LIBNAME=mylib
NDKDIR=~/ndk
TOOLS="aapt dx javac cmake apksigner"
##########


#buildflags
##########
#java
JF="-Xlint:deprecation -source 1.8 -target 1.8\
	-bootclasspath ${RT}\
	-classpath ${AJAR} -d build/obj"
JTF=${WPATH}/java/su/xash/hlsdk/MainActivity.java
#java

#dx
DXF="--dex --output=build/apk/classes.dex build/obj/"

#aapt
AGRF="-I ${AJAR}  -f -m -J build/gen/ -M build/AndroidManifest.xml"
APF=" -I ${AJAR} -f -M build/AndroidManifest.xml \
	-F build/${AN}.apk build/apk/"
#aapt
##########



#COMMANDS
##########
set -e

for n in aapt dx javac cmake apksigner 7z
do
if command -v $n >/dev/null 2>&1; then
    echo "$n found"
else
    echo "$n not found, trying to install tools"
#    exit 1
yes | apt install aapt dx openjdk-21 cmake apksigner
fi
done

if [ -d ${NDKDIR}/android-ndk-r29 ]; then
echo "ndk found"
else
echo "ndk not found, installing"
mkdir -p ${NDKDIR}
wget https://github.com/lzhiyong/termux-ndk/releases/download/android-ndk/android-ndk-r29-aarch64.7z -O ${NDKDIR}/android-ndk-r29.7z
7z x ${NDKDIR}/android-ndk-r29.7z -o${NDKDIR}
fi

rm -rf build/*
mkdir -p build/gen build/obj build/apk build/lib/arm64-v8a

if [ -e .gitignore ]; then
echo ""
else
echo "creating .gitignore file to ignore build dir and android.jar file"
touch .gitignore
echo "build/" >> .gitignore
echo "android.jar" >> .gitignore
echo ${KEYSTORE} >> .gitignore
fi

if [ -e ${AJAR} ]; then
   echo "android.jar found"
else
   echo "downloading android.jar"
   wget https://raw.githubusercontent.com/Sable/android-platforms/master/android-30/android.jar -O \
   ${AJAR}
fi

if [ -e ${KEYSTORE} ]; then
   echo "keystore found"
else
   echo "keystore not found, generating..."
   keytool -genkeypair \
  -alias myalias \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000 \
  -keystore ${KEYSTORE} \
  -storepass ${KEYPASS} \
  -keypass ${KEYPASS} \
  -dname "CN=Unknown, OU=Unknown, O=Unknown, L=Unknown, S=Unknown, C=Unknown"
fi

if [ -d "../build/dlls" ]; then
    echo "cmake was configured(maybe)"
else
    echo "cmake was not configured, configuring..."
cd ..
cmake -DCMAKE_TOOLCHAIN_FILE=${NDKDIR}/android-ndk-r29/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-21 \
      -DCMAKE_BUILD_TYPE=Release \
      -B build
cd android
fi

cp ${WPATH}/AndroidManifest.xml build
sed -i '/<manifest/apackage="su.xash.hlsdk"' build/AndroidManifest.xml
sed -i '/<\/queries>/a<uses-sdk android:minSdkVersion="3" android:targetSdkVersion="34"\/>' build/AndroidManifest.xml
#default manifest file not containing this lines, so we just copying into it
#replace su.xash.hlsdk with your package name, for example su.xash.mymod

aapt package ${AGRF}
echo "aapt done"
javac ${JF} ${JTF}
echo "java done"
dx ${DXF}
echo "dx done"
aapt package ${APF}
echo "aapt done"

cd ..
cmake --build build
cp build/cl_dll/libclient_android_arm64.so android/build/lib/arm64-v8a
cp build/dlls/libhl_android_arm64.so android/build/lib/arm64-v8a/lib${LIBNAME}_android_arm64.so

cd android/build
aapt add ${AN}.apk lib/arm64-v8a/libclient_android_arm64.so
aapt add ${AN}.apk lib/arm64-v8a/lib${LIBNAME}_android_arm64.so
cd ..

echo "cmake build done"
echo ${KEYPASS} | apksigner sign --ks ${KEYSTORE} build/Launcher.apk
echo "build done, if not opening installation prompt, try termux-open build/${AN}.apk"

termux-open build/${AN}.apk
##########
