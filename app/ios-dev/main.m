#import <UIKit/UIKit.h>
#include <pthread.h>

void ios_main(const char* cache_dir, const char* bundle_dir, const char* frameworks_dir);

#import "../fishhook/fishhook.h"
 
static int (*orig_exit)(int);
 
int my_exit(int rc) {
  NSLog(@"Calling exit: %d", rc);
  orig_exit(rc);
  return 0;
}

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

        rebind_symbols((struct rebinding[1]){{"exit", my_exit, (void *)&orig_exit}}, 1);

        ios_main(cachesPath.UTF8String, bundlePath.UTF8String, frameworksPath.UTF8String);
    }
    
    return 0;
}
