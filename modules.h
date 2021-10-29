#ifndef MODULES_H
#define MODULES_H

struct module_struct {
  const char* type;
  const char* name;
  struct module_interface_struct {
      const char* interface;
      const void* function;
  } interfaces[];
};

extern int modules(void*);

extern char** modules_names();

extern int modules_lookup(char* interface, char** name, const void** ptr);

struct library_struct {
    struct library_struct* next;
    char* type;
    char* name;
    struct library_interface {
        char* interface;
        void* function;
    } interfaces[];
};

extern struct library_struct* libraries;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void library_register(struct library_struct *library) {
    library->next = libraries;
    libraries = library;
}
#pragma GCC diagnostic pop

#endif
