ANDROID_JAR=../android-13.jar
AAPT=./../aapt
DX=./../dx
APKBUILDER=./../apkbuilder
NAME=gravgun
mkdir bin
mkdir bin/classes
mkdir assets/
$AAPT package -M AndroidManifest.xml -m -S res -I $ANDROID_JAR
$JAVA_HOME/bin/javac -d bin/classes -s bin/classes -cp $ANDROID_JAR src/in/celest/xash3d/*.java
$DX --dex --output=bin/classes.dex bin/classes/
$AAPT package -f -M AndroidManifest.xml -S res -I $ANDROID_JAR -F bin/$NAME.apk.unaligned
python2 makepak.py pak/ assets/extras.pak
zip -r bin/$NAME.apk.unaligned assets/
$APKBUILDER bin/$NAME.apk -u -nf libs/ -rj libs -f bin/classes.dex -z bin/$NAME.apk.unaligned
#java -jar /mnt/app/apktool/signapk.jar /mnt/app/apktool/testkey.x509.pem /mnt/app/apktool/testkey.pk8 bin/$NAME.apk bin/$NAME-signed.apk
