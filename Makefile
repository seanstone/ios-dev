.PHONY: default
default: app/export/ios-env.ipa

IOS_SDK = "$(shell xcrun --sdk iphoneos --show-sdk-path)"
ID = "Apple Development: En Shih (8JAX23HR7H)"

bin/libios-env.a: app/app-main.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot $(IOS_SDK) -c $< -o bin/app-main.o
	ar rcs $@ bin/app-main.o

bin/hello: hello/main.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot $(IOS_SDK) -dynamiclib $^ -o $@
	codesign --force --sign $(ID) \
		--options runtime \
		--timestamp=none \
		--preserve-metadata=identifier,flags \
		$@

app/ios-env.xcarchive: bin/libios-env.a bin/hello
	cd app && xcodebuild archive -scheme ios-env -sdk iphoneos -allowProvisioningUpdates -archivePath ios-env.xcarchive

app/export/ios-env.ipa: app/ios-env.xcarchive
	cd app && xcodebuild -exportArchive -archivePath ios-env.xcarchive -exportPath export -exportOptionsPlist ExportOptions.plist

.PHONY: clean
clean:
	rm -rf bin app/*.xcarchive app/export