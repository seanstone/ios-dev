.PHONY: default
default: bin/libios-dev.a bin/hello

include id.mk

IOS_SDK = "$(shell xcrun --sdk iphoneos --show-sdk-path)"

bin/libios-dev.a: app/app-main.c
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

.PHONY: ipa
ipa: app/export/ios-dev.ipa

app/ios-dev.xcarchive: bin/libios-dev.a bin/hello
	cd app && xcodebuild archive -scheme ios-dev -sdk iphoneos -allowProvisioningUpdates -archivePath ios-dev.xcarchive

app/export/ios-dev.ipa: app/ios-dev.xcarchive
	cd app && xcodebuild -exportArchive -archivePath ios-dev.xcarchive -exportPath export -exportOptionsPlist ExportOptions.plist

.PHONY: clean
clean:
	rm -rf bin app/*.xcarchive app/export