#include "config.h"
#include "utilities.h"
#include "modules.h"

static char* info(void)
{
    return "MySQL database module";
}

const voidfunc infotable[1] = { (voidfunc)info };

static int getfunctions(void** table)
{
    *table = 0;
    return 0;
}

const voidfunc databasetable[1] = { (voidfunc)getfunctions };

struct module_mystruct {
  char* type;
  char* name;
  struct {
      const char* interface;
      const void* function;
  } interfaces[3];
};

struct module_mystruct module = {
  "basic", "dbsimple_mysql",
  { { "info", infotable },
    { "database", databasetable },
    { 0, 0 }
  }
};

#include "module.c"
