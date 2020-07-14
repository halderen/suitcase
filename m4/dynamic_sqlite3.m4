AC_DEFUN([DYNAMIC_SQLITE3],[
	AC_ARG_ENABLE(
		[dynamic-sqlite3],
		[AS_HELP_STRING([--enable-dynamic-sqlite3],[enable dynamic dependency sqlite3 @<:@enabled@:>@])],
		,
		[enable_dynamic_sqlite3="yes"]
	)
	if test "${enable_dynamic_sqlite3}" = "yes"; then
		CPPFLAGS="$CPPFLAGS -DDYNAMIC_SQLITE3"
	fi
])
