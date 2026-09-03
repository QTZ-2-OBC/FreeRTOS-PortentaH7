#ifndef QTZ_LIB_DEBUG
#define QTZ_LIB_DEBUG

enum { QTZ_DEBUG_CAPACITY = 1024 };

// Platform specific implementation for initializing the resources for debug
// printing.
//
// Mainly used to setup QTZ_Debug_Print.
void QTZ_Debug_Init();

#ifdef QTZ_DEBUG
#define QTZ_Debug_Log(format, ...)                                             \
  QTZ_Debug_InnerLog("%s:%d [LOG] " format "\n", __FILE__, __LINE__,           \
                     ##__VA_ARGS__);
#define QTZ_Debug_Warning(format, ...)                                         \
  QTZ_Debug_InnerLog("%s:%d [WAR] " format "\n", __FILE__, __LINE__,           \
                     ##__VA_ARGS__);
#define QTZ_Debug_Error(format, ...)                                           \
  QTZ_Debug_InnerLog("%s:%d [ERR] " format "\n", __FILE__, __LINE__,           \
                     ##__VA_ARGS__);

void QTZ_Debug_InnerLog(const char *msg, ...);
#else
#define QTZ_Debug_Log(format, ...)
#define QTZ_Debug_Warning(format, ...)
#define QTZ_Debug_Error(format, ...)
#endif

// Internal function used to print the output!
//
// Implement according to the platform.
void QTZ_Debug_Print();

#endif
