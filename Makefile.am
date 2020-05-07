.PHONY: analyze

MAINTAINERCLEANFILES=$(srcdir)/Makefile.in
# config.log config.status \
# $(srcdir)/configure \
# $(srcdir)/install-sh $(srcdir)/ltmain.sh $(srcdir)/missing \
# $(srcdir)/depcomp $(srcdir)/aclocal.m4 $(srcdir)/compile \
# $(srcdir)/config.guess $(srcdir)/config.sub

CLEANFILES=$(srcdir)/*~ core* vgcore*
CLEANFILES+=schema_mysql.h schema_sqlite.h sqlstmts_mysql.h sqlstmts_sqlite.h

ACLOCAL_AMFLAGS = -Im4
AM_CFLAGS = -DSYSCONFDIR=@sysconfdir@

libsuitcase_CFLAGS = -D_REENTRANT -O2
suitcase_CFLAGS = @MYSQL_CFLAGS@ @SQLITE3_CFLAGS@ -D_REENTRANT -O2
suitcase_LDFLAGS = @MYSQL_LDFLAGS@ @SQLITE3_LDFLAGS@
suitcase_LDADD = schema_mysql.o schema_sqlite.o $(LIBCURL) -ldl @PTHREAD_LIBS@

check_PROGRAMS = test
lib_LTLIBRARIES = libsuitcase.la
bin_PROGRAMS = suitcase

install-exec-hook:
	ln $(DESTDIR)$(bindir)/suitcase$(EXEEXT) \
	   $(DESTDIR)$(bindir)/suitcased$(EXEEXT)

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/suitcased$(EXEEXT)

BUILT_SOURCES = schema_mysql.h schema_sqlite.h
EXTRA_DIST = $(srcdir)/LICENSE $(srcdir)/README.md
EXTRA_DIST += schema_mysql.txt schema_sqlite.txt

libsuitcase_la_SOURCES = tree.c tree.h
suitcase_SOURCES  = main.c
suitcase_SOURCES += daemonize.c daemonize.h janitor.c janitor.h
suitcase_SOURCES += settings.c settings.h logging.c logging.h
suitcase_SOURCES += utilities.c utilities.h
suitcase_SOURCES += sqlstmts.c


if USE_SQLITE3
suitcase_SOURCES += schema_sqlite.h
endif
if USE_MYSQL
suitcase_SOURCES += schema_mysql.h
endif

test_SOURCES = tests/suitcasetest.c schema_mysql.h
test_CFLAGS = @CUNIT_INCLUDES@ @MYSQL_CFLAGS@
test_AM_LDLIBS = @CUNIT_LIBS@
test_LDFLAGS = @MYSQL_LDFLAGS@
test_LDADD = @CUNIT_LIBS@ schema_mysql.o -lmysqlclient

TESTS = $(check_PROGRAMS)

%.h %.o : %.txt
	-if [ "`tail -c 1 schema_mysql.sql | tr '\000' X`" != X -o \
	     "`tail -c 1 schema_mysql.sql | tr '\000' Y`" != Y ]; then \
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
	awk -f sql2inc.awk $(*).sql > $(*).inc a=$(*)

sqlstmts.c: sqlstmts_sqlite.inc

doxygen:
	rm -rf $(top_builddir)/doxygen
	env $(DX_ENV) SRCDIR=$(top_srcdir) OUTPUTDIR=$(top_builddir)/doxygen \
	    $(DX_DOXYGEN) $(top_builddir)/doxygen

analyze:
	cppcheck --quiet --enable=unusedFunction $(suitcase_SOURCES)
