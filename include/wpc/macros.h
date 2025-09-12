#pragma once

#if defined(__clang__)
  #define WPR_DIAG_PUSH    _Pragma("clang diagnostic push")
  #define WPR_DIAG_POP     _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
  #define WPR_DIAG_PUSH    _Pragma("GCC diagnostic push")
  #define WPR_DIAG_POP     _Pragma("GCC diagnostic pop")
#else
  #define WPR_DIAG_PUSH
  #define WPR_DIAG_POP
#endif

#if defined(__clang__)
  #if defined(__has_warning)
    #if __has_warning("-Wdouble-promotion")
      #define WPR_IGNORE_DOUBLE_PROMOTION _Pragma("clang diagnostic ignored \"-Wdouble-promotion\"")
    #else
      #define WPR_IGNORE_DOUBLE_PROMOTION
    #endif
  #else
    #define WPR_IGNORE_DOUBLE_PROMOTION _Pragma("clang diagnostic ignored \"-Wdouble-promotion\"")
  #endif

#elif defined(__GNUC__)
  #if defined(__has_warning)
    #if __has_warning("-Wdouble-promotion")
      #define WPR_IGNORE_DOUBLE_PROMOTION _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
    #else
      #define WPR_IGNORE_DOUBLE_PROMOTION
    #endif
  #else
    #if (__GNUC__ > 9) || (__GNUC__ == 9 && __GNUC_MINOR__ >= 0)
      #define WPR_IGNORE_DOUBLE_PROMOTION _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
    #else
      #define WPR_IGNORE_DOUBLE_PROMOTION
    #endif
  #endif

#else
  #define WPR_IGNORE_DOUBLE_PROMOTION
#endif

#if defined(__clang__)
  #if defined(__has_warning)
    #if __has_warning("-Wimplicit-int-float-conversion")
      #define WPR_IGNORE_IMPLICIT_INT_FLOAT_CONV _Pragma("clang diagnostic ignored \"-Wimplicit-int-float-conversion\"")
    #else
      #define WPR_IGNORE_IMPLICIT_INT_FLOAT_CONV
    #endif
  #else
    #define WPR_IGNORE_IMPLICIT_INT_FLOAT_CONV _Pragma("clang diagnostic ignored \"-Wimplicit-int-float-conversion\"")
  #endif
#else
  #define WPR_IGNORE_IMPLICIT_INT_FLOAT_CONV
#endif

#define BEGIN_IGNORE_WARNINGS \
  WPR_DIAG_PUSH              \
  WPR_IGNORE_DOUBLE_PROMOTION\
  WPR_IGNORE_IMPLICIT_INT_FLOAT_CONV

#define END_IGNORE_WARNINGS \
  WPR_DIAG_POP
