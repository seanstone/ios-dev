#!/bin/bash

mkdir -p ../bin/
mkdir -p ../lib/

cp /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib ../lib/libssl.3.dylib
../../toolchain/patch_to_ios.py ../lib/libssl.3.dylib -o ../lib/libssl.3.dylib.1
rm -f ../lib/libssl.3.dylib
mv ../lib/libssl.3.dylib.1 ../lib/libssl.3.dylib

cp /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib ../lib/libcrypto.3.dylib
../../toolchain/patch_to_ios.py ../lib/libcrypto.3.dylib -o ../lib/libcrypto.3.dylib.1
rm -f ../lib/libcrypto.3.dylib
mv ../lib/libcrypto.3.dylib.1 ../lib/libcrypto.3.dylib

cp /opt/homebrew/opt/minizip/lib/libminizip.1.dylib ../lib/libminizip.1.dylib
../../toolchain/patch_to_ios.py ../lib/libminizip.1.dylib -o ../lib/libminizip.1.dylib.1
rm -f ../lib/libminizip.1.dylib
mv ../lib/libminizip.1.dylib.1 ../lib/libminizip.1.dylib

../../toolchain/patch_macho.py ./zsign/bin/zsign ../bin/zsign
../../toolchain/patch_to_ios.py ../bin/zsign -o ../bin/zsign
#../../toolchain/patch_add_syslog_shim.py ../lib/libcrypto.3.dylib ../lib/libcrypto.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib \
  @rpath/libssl.3.dylib \
  ../bin/zsign

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  ../bin/zsign

install_name_tool -change \
  /opt/homebrew/opt/minizip/lib/libminizip.1.dylib \
  @rpath/libminizip.1.dylib \
  ../bin/zsign

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  ../lib/libcrypto.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib \
  @rpath/libssl.3.dylib \
  ../lib/libssl.3.dylib

install_name_tool -change \
  /opt/homebrew/Cellar/openssl@3/3.5.4/lib/libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  ../lib/libssl.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/minizip/lib/libminizip.1.dylib \
  @rpath/libminizip.1.dylib \
  ../lib/libminizip.1.dylib

install_name_tool -change /usr/lib/libSystem.B.dylib \
  @rpath/libsyslog_extsn_shim.dylib \
  ../lib/libcrypto.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../bin/zsign

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../lib/libssl.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../lib/libcrypto.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../lib/libminizip.1.dylib