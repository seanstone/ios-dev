#!/bin/bash

../../toolchain/patch_macho.py /opt/homebrew/bin/gcat ../bin/gcat
../../toolchain/patch_to_ios.py ./user/bin/gcat -o ../bin/gcat

# Patch CoreServices
install_name_tool -change \
  /System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices \
  /System/Library/Frameworks/CoreServices.framework/CoreServices \
  ../bin/gcat

# Patch CoreFoundation
install_name_tool -change \
  /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation \
  /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation \
  ../bin/gcat

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
    --options runtime \
    --timestamp=none \
    --preserve-metadata=identifier,flags \
    ../bin/gcat