#!/bin/bash

mkdir -p ./user/bin/

cp /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib ./user/bin/libssl.3.dylib
./toolchain/patch_to_ios.py ./user/bin/libssl.3.dylib -o ./user/bin/libssl.3.dylib.1
rm -f ./user/bin/libssl.3.dylib
mv ./user/bin/libssl.3.dylib.1 ./user/bin/libssl.3.dylib

cp /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib ./user/bin/libcrypto.3.dylib
./toolchain/patch_to_ios.py ./user/bin/libcrypto.3.dylib -o ./user/bin/libcrypto.3.dylib.1
rm -f ./user/bin/libcrypto.3.dylib
mv ./user/bin/libcrypto.3.dylib.1 ./user/bin/libcrypto.3.dylib

cp /opt/homebrew/opt/minizip/lib/libminizip.1.dylib ./user/bin/libminizip.1.dylib
./toolchain/patch_to_ios.py ./user/bin/libminizip.1.dylib -o ./user/bin/libminizip.1.dylib.1
rm -f ./user/bin/libminizip.1.dylib
mv ./user/bin/libminizip.1.dylib.1 ./user/bin/libminizip.1.dylib

./toolchain/patch_macho.py ./zsign/bin/zsign ./user/bin/zsign
./toolchain/patch_to_ios.py ./user/bin/zsign -o ./user/bin/zsign
#./toolchain/patch_add_syslog_shim.py ./user/bin/libcrypto.3.dylib ./user/bin/libcrypto.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib \
  @rpath/libssl.3.dylib \
  ./user/bin/zsign

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  ./user/bin/zsign

install_name_tool -change \
  /opt/homebrew/opt/minizip/lib/libminizip.1.dylib \
  @rpath/libminizip.1.dylib \
  ./user/bin/zsign

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  ./user/bin/libcrypto.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib \
  @rpath/libssl.3.dylib \
  ./user/bin/libssl.3.dylib

install_name_tool -change \
  /opt/homebrew/Cellar/openssl@3/3.5.4/lib/libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  ./user/bin/libssl.3.dylib

install_name_tool -change \
  /opt/homebrew/opt/minizip/lib/libminizip.1.dylib \
  @rpath/libminizip.1.dylib \
  ./user/bin/libminizip.1.dylib

install_name_tool -change /usr/lib/libSystem.B.dylib \
  @rpath/libsyslog_extsn_shim.dylib \
  ./user/bin/libcrypto.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ./user/bin/zsign

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ./user/bin/libssl.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ./user/bin/libcrypto.3.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ./user/bin/libminizip.1.dylib