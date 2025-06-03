#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

@interface MacosTouchpadNative : NSObject

- (void)startListening;
- (void)stopListening;

@end