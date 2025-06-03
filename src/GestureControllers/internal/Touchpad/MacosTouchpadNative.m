#import "MacosTouchpadNative.h"

@implementation MacosTouchpadNative

- (void)startListening {
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskGesture | NSEventMaskScrollWheel | NSEventMaskMagnify | NSEventMaskRotate | NSEventMaskSwipe handler:^(NSEvent *event) {
        [self handleTrackpadEvent:event];
        return event;
    }];
}

- (void)stopListening {
    [NSEvent removeMonitor:self];
}

- (void)handleTrackpadEvent:(NSEvent *)event {
    NSUInteger phase = [event phase];
    switch (phase) {
        case NSTouchPhaseBegan:
            NSLog(@"Touch began");
            break;
        case NSTouchPhaseMoved:
            NSLog(@"Touch moved");
            break;
        case NSTouchPhaseEnded:
            NSLog(@"Touch ended");
            break;
        case NSTouchPhaseCancelled:
            NSLog(@"Touch cancelled");
            break;
        default:
            break;
    }
}

@end
