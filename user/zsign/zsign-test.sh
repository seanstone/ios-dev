#!/bin/bash

mkdir -p ../bin/

cp /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib ../bin/libssl.3.dylib
../../toolchain/patch_to_ios.py ../bin/libssl.3.dylib -o ../bin/libssl.3.dylib.1
rm -f ../bin/libssl.3.dylib
mv ../bin/libssl.3.dylib.1 ../bin/libssl.3.dylib

cp /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib ../bin/libcrypto.3.dylib
../../toolchain/patch_to_ios.py ../bin/libcrypto.3.dylib -o ../bin/libcrypto.3.dylib.1
rm -f ../bin/libcrypto.3.dylib
mv ../bin/libcrypto.3.dylib.1 ../bin/libcrypto.3.dylib

cp /opt/homebrew/opt/minizip/lib/libminizip.1.dylib ../bin/libminizip.1.dylib
../../toolchain/patch_to_ios.py ../bin/libminizip.1.dylib -o ../bin/libminizip.1.dylib.1
rm -f ../bin/libminizip.1.dylib
mv ../bin/libminizip.1.dylib.1 ../bin/libminizip.1.dylib

../../toolchain/patch_macho.py ./zsign/bin/zsign ../bin/zsign
../../toolchain/patch_to_ios.py ../bin/zsign -o ../bin/zsign
#../../toolchain/patch_add_syslog_shim.py ../bin/libcrypto.3.dylib ../bin/libcrypto.3.dylib

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
  ../bin/libcrypto.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib \
  @rpath/libssl.3.dylib \
  ../bin/libssl.3.dylib

install_name_tool -change \
  /opt/homebrew/Cellar/openssl@3/3.5.4/lib/libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  ../bin/libssl.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/minizip/lib/libminizip.1.dylib \
  @rpath/libminizip.1.dylib \
  ../bin/libminizip.1.dylib

install_name_tool -change /usr/lib/libSystem.B.dylib \
  @rpath/libsyslog_extsn_shim.dylib \
  ../bin/libcrypto.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../bin/zsign

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../bin/libssl.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../bin/libcrypto.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../bin/libminizip.1.dylib