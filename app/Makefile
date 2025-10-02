.PHONY: default
default: bin/libios-dev.a ../user/bin/libsyslog_extsn_shim.dylib

SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)

bin/libios-dev.a: build/app-main.c.o
	mkdir -p $(@D)
	ar rcs $@ $^

build/%.c.o: %.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot $(SDK) -c $< -o $@

../user/bin/libsyslog_extsn_shim.dylib: syslog_extsn_shim.c
	mkdir -p $(@D)
	xcrun clang -target arm64-apple-ios -isysroot $(SDK) \
		-dynamiclib -fvisibility=default \
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
	rm -rf bin build *.xcarchive export