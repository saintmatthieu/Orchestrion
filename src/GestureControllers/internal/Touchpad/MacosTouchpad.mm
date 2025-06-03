#import "MacosTouchpad.h"
#import "MacosTouchpadNative.h"

static MacosTouchpadNative *touchpad = nil;

void startListeningToTrackpadEvents() {
    if (!touchpad) {
        touchpad = [[MacosTouchpadNative alloc] init];
    }
    [touchpad startListening];
}

void stopListeningToTrackpadEvents() {
    if (touchpad) {
        [touchpad stopListening];
    }
}
