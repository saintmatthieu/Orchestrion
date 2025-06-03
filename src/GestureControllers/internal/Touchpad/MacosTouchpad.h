#pragma once

// #include "../GestureControllerInternalTypes.h"
// #include "IOperatingSystemTouchpad.h"

// #include <functional>

#ifdef __OBJC__
@class MacosTouchpadNative;
#else
typedef struct objc_object MacosTouchpadNative;
#endif

void startListeningToTrackpadEvents();
void stopListeningToTrackpadEvents();

// class MacosTouchpad : public IOperatingSystemTouchpad
// {
// public:
//   MacosTouchpad(std::function<void(const TouchpadScan &)> cb);
//   ~MacosTouchpad() override;

//   bool isAvailable() const override;
// };
