.PHONY: default
default: ios-lib lib/libfishhook.a ../user/bin/libsyslog_extsn_shim.dylib

.PHONY: ios-lib
ios-lib:
	cargo build --lib --profile release --target=aarch64-apple-ios

SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)

lib/libfishhook.a: fishhook/fishhook.c
	mkdir -p build
	clang -target arm64-apple-ios -isysroot $(SDK) -Ifishhook -DFISHHOOK_EXPORT -c $< -o build/fishhook.o
	mkdir -p $(@D)
	ar rcs $@ build/fishhook.o

../user/bin/libsyslog_extsn_shim.dylib: syslog_extsn_shim.c
	mkdir -p $(@D)
	xcrun clang -target arm64-apple-ios -isysroot $(SDK) \
		-dynamiclib -fvisibility=default \
		-Wl,-reexport_library,/usr/lib/libSystem.B.dylib \
		-install_name @rpath/libsyslog_extsn_shim.dylib \
		-o $@ $<
	codesign --force --sign "Apple Development: En Shih (8JAX23HR7H)" \
		--options runtime \
		--timestamp=none \
		--preserve-metadata=identifier,flags \
		$@

.PHONY: ipa
ipa: app/export/ios-dev.ipa

app/ios-dev.xcarchive: bin/libios-dev.a
	xcodebuild archive -scheme ios-dev -sdk iphoneos -allowProvisioningUpdates -archivePath ios-dev.xcarchive

app/export/ios-dev.ipa: app/ios-dev.xcarchive
	xcodebuild -exportArchive -archivePath ios-dev.xcarchive -exportPath export -exportOptionsPlist ExportOptions.plist

.PHONY: clean
clean:
	rm -rf bin build target *.xcarchive export