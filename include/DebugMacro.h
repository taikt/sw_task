// DebugMacro.h
#ifdef DEBUG
#define post(...) post_internal(__VA_ARGS__, __FILE__, __LINE__, __func__)
#else
#define post(...) post_internal(__VA_ARGS__, nullptr, 0, nullptr)
#endif