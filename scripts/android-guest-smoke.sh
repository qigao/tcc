#!/system/bin/sh
set -eu
cd /data/local/tmp/tcc_poc
export LD_LIBRARY_PATH=/data/local/tmp/tcc_poc
chmod 700 ./tcc ./android_smoke
echo '=== Android Bionic runtime ==='
./android_smoke
echo '=== Android native TinyCC compiler ==='
./tcc -v
echo '=== Android TinyCC ELF object generation ==='
./tcc -c android_tcc_object.c -o android_tcc_object.o
test -s android_tcc_object.o
echo 'Android: native TinyCC compiler generated a nonempty ELF object'
