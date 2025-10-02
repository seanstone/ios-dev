.PHONY: default
default: bin/libios-dev.a

SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)

bin/libios-dev.a: build/app-main.c.o
	mkdir -p $(@D)
	ar rcs $@ $^

build/%.c.o: %.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot $(SDK) -c $< -o $@

.PHONY: ipa
ipa: app/export/ios-dev.ipa

app/ios-dev.xcarchive: bin/libios-dev.a
	xcodebuild archive -scheme ios-dev -sdk iphoneos -allowProvisioningUpdates -archivePath ios-dev.xcarchive

app/export/ios-dev.ipa: app/ios-dev.xcarchive
	xcodebuild -exportArchive -archivePath ios-dev.xcarchive -exportPath export -exportOptionsPlist ExportOptions.plist

.PHONY: clean
clean:
	rm -rf bin build *.xcarchive export