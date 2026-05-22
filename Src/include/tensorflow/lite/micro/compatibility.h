/*
  Local override for Arduino_TensorFlowLite compatibility header.
  This avoids a private operator delete that breaks placement-new on ESP32.
*/
#ifndef TENSORFLOW_LITE_MICRO_COMPATIBILITY_H_
#define TENSORFLOW_LITE_MICRO_COMPATIBILITY_H_

#ifdef ARDUINO
// Keep the macro empty to avoid a private operator delete.
#define TF_LITE_REMOVE_VIRTUAL_DELETE
#else
#define TF_LITE_REMOVE_VIRTUAL_DELETE
#endif

#endif  // TENSORFLOW_LITE_MICRO_COMPATIBILITY_H_
