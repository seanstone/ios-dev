#import <UIKit/UIKit.h>

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

        // Prints to Xcode console
        NSLog(@"Bundle path: %@", bundlePath);
        NSLog(@"Documents path: %@", docsPath);
        NSLog(@"Caches path: %@", cachesPath);
        NSLog(@"Frameworks path: %@", frameworksPath);
    }
    
    return 0;
}