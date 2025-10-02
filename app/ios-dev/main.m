#import <UIKit/UIKit.h>

void ios_main(const char* cache_dir, const char* bundle_dir, const char* frameworks_dir);

int main(int argc, char * argv[]) {
    @autoreleasepool {
        // Bundle path
        NSString *bundlePath = [[NSBundle mainBundle] resourcePath];

        // Documents path
        NSURL *docsURL = [[[NSFileManager defaultManager]
                            URLsForDirectory:NSDocumentDirectory
                            inDomains:NSUserDomainMask] lastObject];
        NSString *docsPath = docsURL.path;

        // Caches path
        NSURL *cachesURL = [[[NSFileManager defaultManager]
                              URLsForDirectory:NSCachesDirectory
                              inDomains:NSUserDomainMask] lastObject];
        NSString *cachesPath = cachesURL.path;

        NSString *frameworksPath = [[[NSBundle mainBundle] privateFrameworksPath] stringByStandardizingPath];

        // (Optional) debug prints in Xcode console
        NSLog(@"Bundle path: %@", bundlePath);
        NSLog(@"Documents path: %@", docsPath);
        NSLog(@"Caches path: %@", cachesPath);
        NSLog(@"Frameworks path: %@", frameworksPath);

        ios_main(cachesPath.UTF8String, bundlePath.UTF8String, frameworksPath.UTF8String);
    }
    
    return 0;
}