#ifndef QTZ_LIB_DEBUG
#define QTZ_LIB_DEBUG

enum { QTZ_DEBUG_CAPACITY = 1024 };

// Platform specific implementation for initializing the resources for debug
// printing.
//
// Mainly used to setup QTZ_Debug_Print.
void QTZ_Debug_Init();

void QTZ_Debug_Log(const char *msg, ...);
void QTZ_Debug_Warning(const char *msg, ...);
void QTZ_Debug_Error(const char *msg, ...);

// Internal function used to print the output!
//
// Implement according to the platform.
void QTZ_Debug_Print();

#endif
