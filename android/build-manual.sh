ANDROID_JAR=../android-13.jar
AAPT=./../aapt
DX=./../dx
APKBUILDER=./../apkbuilder
NAME=mod
mkdir bin
mkdir bin/classes
$AAPT package -M AndroidManifest.xml -m -S res -I $ANDROID_JAR
$JAVA_HOME/bin/javac -d bin/classes -s bin/classes -cp $ANDROID_JAR src/in/celest/xash3d/LauncherActivity.java
$DX --dex --output=bin/classes.dex bin/classes/
/mnt/app/apktool/aapt package -f -M AndroidManifest.xml -S res -I $ANDROID_JAR -F bin/$NAME.apk.unaligned
$APKBUILDER bin/$NAME.apk -u -nf libs/ -rj libs -f bin/classes.dex -z bin/$NAME.apk.unaligned
java -jar /mnt/app/apktool/signapk.jar /mnt/app/apktool/testkey.x509.pem /mnt/app/apktool/testkey.pk8 bin/$NAME.apk bin/$NAME-signed.apk
