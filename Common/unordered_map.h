#ifndef __COMMON_UNORDERED_MAP_H__
#define __COMMON_UNORDERED_MAP_H__

#if 1
//c++0x
#include <unordered_map>
#define UNORDERED_MAP(...) std::unordered_map(__VA_ARGS__)
#else
// tr1
#include <tr1/unordered_map>
#define UNORDERED_MAP(...) std::tr1::unordered_map(__VA_ARGS__)
#endif

#endif
