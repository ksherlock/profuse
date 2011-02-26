
#ifndef __PROFUSE_SMART_POINTERS_H__
#define __PROFUSE_SMART_POINTERS_H__

#ifdef CPP0X
//C++0x
#include <memory>


#define SHARED_PTR(T) std::shared_ptr<T>
#define WEAK_PTR(T) std::weak_ptr<T>

#define MAKE_SHARED(T, ...) std::make_shared<T>(__VA_ARGS__)
#define ENABLE_SHARED_FROM_THIS(T) std::enable_shared_from_this<T>

#define STATIC_POINTER_CAST(T, ARG) std::static_pointer_cast<T>(ARG)
#define DYNAMIC_POINTER_CAST(T, ARG) std::dynamic_pointer_cast<T>(ARG)

#else

// tr1
#include <tr1/memory>

#define SHARED_PTR(T) std::tr1::shared_ptr<T>
#define WEAK_PTR(T) std::tr1::weak_ptr<T>

#define MAKE_SHARED(T, ...) std::tr1::shared_ptr<T>(new T(__VA_ARGS__))
#define ENABLE_SHARED_FROM_THIS(T) std::tr1::enable_shared_from_this<T>

#define STATIC_POINTER_CAST(T, ARG) std::tr1::static_pointer_cast<T>(ARG)
#define DYNAMIC_POINTER_CAST(T, ARG) std::tr1::dynamic_pointer_cast<T>(ARG)

#endif

#endif
