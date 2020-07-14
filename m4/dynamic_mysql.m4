AC_DEFUN([DYNAMIC_MYSQL],[
	AC_ARG_ENABLE(
		[dynamic-MYSQL],
		[AS_HELP_STRING([--enable-dynamic-mysql],[enable dynamic dependency mysql @<:@enabled@:>@])],
		,
		[enable_dynamic_mysql="yes"]
	)
	if test "${enable_dynamic_mysql}" = "yes"; then
		CPPFLAGS="$CPPFLAGS -DDYNAMIC_MYSQL"
	fi
])
