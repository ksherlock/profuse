#ifndef __AUTO_H__
#define __AUTO_H__

template <class T>
class auto_array
{
public:
  auto_array() : _t(NULL) {}
  auto_array(T *t) : _t(t) {}
  ~auto_array() { if (_t) delete []_t; }


  T* release()
  {  T *tmp = _t; _t = NULL; return tmp; }

  T* get() const { return _t; }
  operator T*() const { return _t; }
  T& operator[](int index) { return _t[index]; }

  void reset(T *t)
  {
    if (t == _t) return;
    if (_t) delete[] _t;
    _t = t;
  }

private:
  T *_t;
};

// ::close
#if defined(O_CREAT) 
class auto_fd
{
public:
  auto_fd(int fd = -1) : _fd(fd) { }

  ~auto_fd() { close(); }

  int release() 
  { int tmp = _fd; _fd = -1; return tmp; }

  int get() const { return _fd; }
  operator int() const { return _fd; }
  
  void reset(int fd)
  {
    if (fd != _fd)
    {
      close();
      _fd = fd;
    }
  }

private:
  auto_fd& operator=(const auto_fd&);
  
  void close()
  {
    if (_fd >= 0)
    {
      ::close(_fd);
      _fd = -1;
    }
  }
  
  int _fd;
};
#endif

// ::mmap, :munmap
#if defined(MAP_FAILED)
class auto_map
{
public:
  auto_map(int fd, size_t size, int prot, int flags) :
    _size(size), 
    _map(::mmap(NULL, size, prot, flags, fd, 0))
  { }

  ~auto_map() 
  { if (_map != MAP_FAILED) ::munmap(_map, _size); }

  void *release() 
  { void *tmp = _map; _map = MAP_FAILED; return tmp; }

  void *get() const { return _map; }
  operator void *() const { return _map; }

private:
  size_t _size;
  void *_map;
};
#endif


#endif

