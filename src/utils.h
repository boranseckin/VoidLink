#ifndef UTILS_H
#define UTILS_H

// #define DEBUG

// only print if DEBUG is enabled
#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#define error(...) printf(__VA_ARGS__)

#endif // UTILS_H