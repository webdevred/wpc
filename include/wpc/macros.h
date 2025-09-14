#pragma once

#if defined(__clang__)
    #define WPR_DIAG_PUSH _Pragma("clang diagnostic push")
    #define WPR_DIAG_POP _Pragma("clang diagnostic pop")
    #define WPR_IGNORE_DOUBLE_PROMOTION                                        \
        _Pragma("clang diagnostic ignored \"-Wdouble-promotion\"")
    #define WPR_IGNORE_IMPLICIT_INT                                            \
        _Pragma("clang diagnostic ignored \"-Wimplicit-int\"")
    #define WPR_IGNORE_CONVERSION                                              \
        _Pragma("clang diagnostic ignored \"-Wconversion\"")
    #define WPR_IGNORE_STRICT_PROTOTYPES                                       \
        _Pragma("clang diagnostic ignored \"-Wstrict-prototypes\"")
    #define WPR_IGNORE_PEDANTIC                                                \
        _Pragma("clang diagnostic ignored \"-Wpedantic\"")
#elif defined(__GNUC__)
    #define WPR_DIAG_PUSH _Pragma("GCC diagnostic push")
    #define WPR_DIAG_POP _Pragma("GCC diagnostic pop")
    #define WPR_IGNORE_DOUBLE_PROMOTION                                        \
        _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
    #define WPR_IGNORE_IMPLICIT_INT                                            \
        _Pragma("GCC diagnostic ignored \"-Wimplicit-int\"")
    #define WPR_IGNORE_CONVERSION                                              \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"")
    #define WPR_IGNORE_STRICT_PROTOTYPES                                       \
        _Pragma("GCC diagnostic ignored \"-Wstrict-prototypes\"")
    #define WPR_IGNORE_PEDANTIC _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#else
    #define WPR_DIAG_PUSH
    #define WPR_DIAG_POP
    #define WPR_IGNORE_DOUBLE_PROMOTION
    #define WPR_IGNORE_IMPLICIT_INT
    #define WPR_IGNORE_CONVERSION
    #define WPR_IGNORE_STRICT_PROTOTYPES
    #define WPR_IGNORE_PEDANTIC
#endif

#define BEGIN_IGNORE_WARNINGS                                                  \
    WPR_DIAG_PUSH                                                              \
    WPR_IGNORE_DOUBLE_PROMOTION                                                \
    WPR_IGNORE_IMPLICIT_INT                                                    \
    WPR_IGNORE_CONVERSION                                                      \
    WPR_IGNORE_STRICT_PROTOTYPES                                               \
    WPR_IGNORE_PEDANTIC

#define END_IGNORE_WARNINGS WPR_DIAG_POP
