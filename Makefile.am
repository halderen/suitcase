.PHONY: all clean install uninstall analyze

MAINTAINERCLEANFILE=$(srcdir)/Makefile.in
# config.log config.status \
# $(srcdir)/configure \
# $(srcdir)/install-sh $(srcdir)/ltmain.sh $(srcdir)/missing \
# $(srcdir)/depcomp $(srcdir)/aclocal.m4 $(srcdir)/compile \
# $(srcdir)/config.guess $(srcdir)/config.sub

CLEANFILES=$(srcdir)/*~ core* vgcore*
CLEANFILES+=schema_mysql.h schema_sqlite3.h sqlstmts_mysql.h sqlstmts_sqlite3.h

ACLOCAL_AMFLAGS = -Im4
AM_CFLAGS = -DSYSCONFDIR=@sysconfdir@

# libsuitcase_CFLAGS = -D_REENTRANT -O2
suitcase_CFLAGS = @MYSQL_CFLAGS@ @SQLITE3_CFLAGS@ $(AM_CFLAGS)
suitcase_LDFLAGS = @MYSQL_LDFLAGS@ @SQLITE3_LDFLAGS@
suitcase_LDADD = schema_mysql.o schema_sqlite3.o libsuitcase.la $(LIBCURL) -ldl @PTHREAD_LIBS@

check_PROGRAMS = test
lib_LTLIBRARIES = libsuitcase.la libmoduleexectcl.la
if HAVE_MYSQL
lib_LTLIBRARIES += libmoduledbmysql.la 
endif

bin_PROGRAMS = suitcase

install-exec-hook:
	ln $(DESTDIR)$(bindir)/suitcase$(EXEEXT) \
	   $(DESTDIR)$(bindir)/suitcased$(EXEEXT)

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/suitcased$(EXEEXT)

BUILT_SOURCES = sqlstmts_sqlite3.inc sqlstmts_mysql.inc
BUILT_SOURCES += schema_mysql.h schema_sqlite3.h
EXTRA_DIST = $(srcdir)/LICENSE $(srcdir)/README.md
EXTRA_DIST += schema_mysql.txt schema_sqlite3.txt

libsuitcase_la_SOURCES  = modules.c modules.h
libsuitcase_la_SOURCES += janitor.c janitor.h
libsuitcase_la_SOURCES += logging.c logging.h
libsuitcase_la_SOURCES += utilities.c utilities.h
libsuitcase_la_SOURCES += tree.c tree.h
libsuitcase_la_SOURCES += settings.c settings.h settings_yaml.c settings_xml.c
libsuitcase_la_SOURCES += dbsimple.h dbsimple.c dbsimplebase.h dbsimplebase.c
libsuitcase_la_CFLAGS   = $(XML_CPPFLAGS) $(AM_CFLAGS)
libsuitcase_la_LIBADD   = $(XML_LIBS)
suitcase_SOURCES  = main.c exampledb.c exampledb.h
if HAVE_LUA
suitcase_SOURCES += executelua.c executelua.h
endif
suitcase_SOURCES += commandline.c commandline.h
suitcase_SOURCES += daemonize.c daemonize.h
suitcase_SOURCES += execute.c execute.h
suitcase_SOURCES += sqlstmts_sqlite3.inc
suitcase_SOURCES += longgetopt.c longgetopt.h

if INCLUDE_SQLITE3
suitcase_SOURCES += dbsimple_sqlite3.c
endif
if HAVE_SQLITE3
suitcase_SOURCES += schema_sqlite3.h
endif
suitcase_SOURCES += dbsimple_dummy.c
suitcase_LDFLAGS += @LUA_LIB@

if HAVE_MYSQL
libmoduledbmysql_la_SOURCES = moduledbmysql.c dbsimple_mysql.c schema_mysql.h
libmoduledbmysql_la_LIBADD  = libsuitcase.la @MYSQL_LDFLAGS@
endif

if HAVE_TCL
libmoduleexectcl_la_SOURCES = executetcl.c executetcl.h
libmoduleexectcl_la_LIBADD  = libsuitcase.la -ltcl
endif

test_SOURCES = tests/suitcasetest.c schema_mysql.h
test_CFLAGS = @CUNIT_INCLUDES@ @MYSQL_CFLAGS@
test_AM_LDLIBS = @CUNIT_LIBS@
test_LDFLAGS = @MYSQL_LDFLAGS@ $(XML_LIBS)
test_LDADD = @CUNIT_LIBS@ schema_mysql.o -lmysqlclient

TESTS = $(check_PROGRAMS)

%.h %.o : %.txt
	-if [ "`tail -c 1 $(<) | tr '\000' X`" != X -o \
	     "`tail -c 1 $(<) | tr '\000' Y`" != Y ]; then \
	  echo -n 0 | tr 0 '\000' >> $(<) ; \
	fi
	-elfO=`objdump -a /usr/bin/env | \
	       sed -e 's/.*file format \(.*\)/\1/p' -e d`
	-objcopy --readonly-text -I binary -O elf64-x86-64 -B i386:x86-64 \
        --rename-section .data=.progmem.data,contents,alloc,load,readonly,data \
        --redefine-sym _binary_$*_sql_start=$* \
        --redefine-sym _binary_$*_sql_end=$*_end \
        --redefine-sym _binary_$*_sql_size=$*_size_sym \
	$(<) $(*).o
	echo "extern const char " $(*)"[];" > $(*).h
	echo "extern const char " $(*)_end"[];" >> $(*).h
	echo "extern const char " $(*)_size_sym"[];" >> $(*).h
	echo "#define $(*)_size ((int)$(*)_size_sym)" >> $(*).h

%.inc: %.sql
	$(AWK) > $(*).inc -f sql2inc.awk \
	       -v prefix=$(*)_ -v postfix=_ -v queryarray=$(*)_array \
	       $(*).sql

sqlstmts_sqlite3.inc: Makefile

doxygen:
	rm -rf $(top_builddir)/doxygen
	env $(DX_ENV) SRCDIR=$(top_srcdir) OUTPUTDIR=$(top_builddir)/doxygen \
	    $(DX_DOXYGEN) $(top_builddir)/doxygen

analyze:
	cppcheck --quiet --enable=unusedFunction $(suitcase_SOURCES)
