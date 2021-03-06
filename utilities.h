#ifndef UTILITIES_H
#define UTILITIES_H

#ifdef NOTDEFINED
#error "never define NOTDEFINED"
#endif

#define QUOTE(ARG) #ARG

#ifdef __cplusplus
#include <cstdio>
#include <string>
#include <sstream>
#endif
#include <stdarg.h>

#if !defined(__GNUC__) || __GNUC__ < 2 || \
    (__GNUC__ == 2 && __GNUC_MINOR__ < 7) ||\
    defined(NEXT)
#ifndef __attribute__
#define __attribute__(__x)
#endif
#endif

#ifdef __cplusplus
class mkstring
{
private:
  std::ostringstream os;
public:
  template <class T> mkstring &operator<<(const T &t) {
    os << t;
    return *this;
  }
  mkstring& operator<<(const char* m) {
    os << m;
    return *this;
  }
  mkstring& operator<<(const std::string& s) {
    os << s;
    return *this;
  }
  static std::string format(const char *fmt, va_list ap);
  static std::string format(const char *fmt, ...)
     __attribute__ ((__format__ (__printf__, 1, 2)));
  operator std::string() const { return os.str(); }
  const std::string str() const { return os.str(); };
  const char* c_str() const { return os.str().c_str(); };
};
#endif

#ifdef DEBUG
# define BUG(ARG) ARG
#else
# define BUG(ARG)
#endif

#ifndef CHECK
#define CHECK(EX) do { if(EX) { int err = errno; fprintf(stderr, "operation" \
 " \"%s\" failed on line %d: %s (%d)\n", #EX, __LINE__, strerror(err), err); \
  abort(); }} while(0)
#endif

#ifndef CHECKALLOC
#define CHECKALLOC(PTR) if(!(PTR)) { fprintf(stderr,"Out of memory when executing %s at %s:%d\n", #PTR, __FILE__, __LINE__); }
#endif

extern char* argv0;

typedef void (*functioncast_t)(void);
extern functioncast_t functioncast(void*generic);

extern int clamp(int value, int lbnd, int ubnd);
unsigned long long int rnd(void);

#endif
