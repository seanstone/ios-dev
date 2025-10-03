#!/bin/bash

../../toolchain/patch_macho.py /opt/homebrew/bin/bash ../bin/bash
../../toolchain/patch_to_ios.py ../bin/bash -o ../bin/bash

# Patch CoreFoundation
install_name_tool -change \
  /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation \
  /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation \
  ../bin/bash

cp /opt/homebrew/opt/readline/lib/libreadline.8.dylib ../lib/libreadline.8.dylib
../../toolchain/patch_to_ios.py ../lib/libreadline.8.dylib -o ../lib/libreadline.8.dylib.1
rm -f ../lib/libreadline.8.dylib
mv ../lib/libreadline.8.dylib.1 ../lib/libreadline.8.dylib

install_name_tool -change \
  /opt/homebrew/opt/readline/lib/libreadline.8.dylib \
  @rpath/libreadline.8.dylib \
  ../bin/bash

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
  --options runtime \
  --timestamp=none \
  --preserve-metadata=identifier,flags \
  ../lib/libreadline.8.dylib


cp /opt/homebrew/opt/readline/lib/libhistory.8.dylib ../lib/libhistory.8.dylib
../../toolchain/patch_to_ios.py ../lib/libhistory.8.dylib -o ../lib/libhistory.8.dylib.1
rm -f ../lib/libhistory.8.dylib
mv ../lib/libhistory.8.dylib.1 ../lib/libhistory.8.dylib

install_name_tool -change \
  /opt/homebrew/opt/readline/lib/libhistory.8.dylib \
  @rpath/libhistory.8.dylib \
  ../bin/bash

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
  --options runtime \
  --timestamp=none \
  --preserve-metadata=identifier,flags \
  ../lib/libhistory.8.dylib


cp /opt/homebrew/opt/ncurses/lib/libncursesw.6.dylib ../lib/libncursesw.6.dylib
../../toolchain/patch_to_ios.py ../lib/libncursesw.6.dylib -o ../lib/libncursesw.6.dylib.1
rm -f ../lib/libncursesw.6.dylib
mv ../lib/libncursesw.6.dylib.1 ../lib/libncursesw.6.dylib

install_name_tool -change \
  /opt/homebrew/opt/ncurses/lib/libncursesw.6.dylib \
  @rpath/libncursesw.6.dylib \
  ../bin/bash

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
  --options runtime \
  --timestamp=none \
  --preserve-metadata=identifier,flags \
  ../lib/libncursesw.6.dylib


cp /opt/homebrew/opt/gettext/lib/libintl.8.dylib ../lib/libintl.8.dylib
../../toolchain/patch_to_ios.py ../lib/libintl.8.dylib -o ../lib/libintl.8.dylib.1
rm -f ../lib/libintl.8.dylib
mv ../lib/libintl.8.dylib.1 ../lib/libintl.8.dylib

install_name_tool -change \
  /opt/homebrew/opt/gettext/lib/libintl.8.dylib \
  @rpath/libintl.8.dylib \
  ../bin/bash

# Patch CoreFoundation
install_name_tool -change \
  /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation \
  /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation \
  ../lib/libintl.8.dylib

install_name_tool -change \
  /System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices \
  /System/Library/Frameworks/CoreServices.framework/CoreServices \
  ../lib/libintl.8.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
  --options runtime \
  --timestamp=none \
  --preserve-metadata=identifier,flags \
  ../lib/libintl.8.dylib

codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
  --options runtime \
  --timestamp=none \
  --preserve-metadata=identifier,flags \
  ../bin/bash