AC_DEFUN([DYNAMIC_MYSQL],[
	AC_ARG_ENABLE(
		[dynamic-mysql],
		[AS_HELP_STRING([--enable-dynamic-mysql],[enable dynamic dependency mysql @<:@enabled@:>@])],
		,
		[enable_dynamic_mysql="yes"]
	)
	if test "${enable_dynamic_mysql}" = "yes"; then
		AC_DEFINE(DO_DYNAMIC_MYSQL, 1, [Define if dynamic MySQL/mariaDB loading])
		CPPFLAGS="$CPPFLAGS -DDYNAMIC_MYSQL"
	fi
])
