.PHONY: default
default: bin/libios-dev.a bin/hello

include id.mk

CC = $(CURDIR)/ios-cc

bin/libios-dev.a: app/app-main.c
	mkdir -p $(@D)
	$(CC) -c $< -o bin/app-main.o
	ar rcs $@ bin/app-main.o

bin/hello: hello/main.c
	mkdir -p $(@D)
	$(CC) $^ -o $@

.PHONY: ipa
ipa: app/export/ios-dev.ipa

app/ios-dev.xcarchive: bin/libios-dev.a bin/hello
	cd app && xcodebuild archive -scheme ios-dev -sdk iphoneos -allowProvisioningUpdates -archivePath ios-dev.xcarchive

app/export/ios-dev.ipa: app/ios-dev.xcarchive
	cd app && xcodebuild -exportArchive -archivePath ios-dev.xcarchive -exportPath export -exportOptionsPlist ExportOptions.plist

.PHONY: clean
clean:
	rm -rf bin app/*.xcarchive app/export