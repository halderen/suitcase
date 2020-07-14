all:	Makefile
	make -f Makefile all
all-am:	Makefile
	make -f Makefile all-am
clean:	Makefile
	make -f Makefile clean
clean-am:	Makefile
	make -f Makefile clean-am
check:	Makefile
	make -f Makefile check
check-am:	Makefile
	make -f Makefile check-am
install-am:	Makefile
	make -f Makefile install-am
install-exec-am:	Makefile
	make -f Makefile install-exec-am
install-data-am:	Makefile
	make -f Makefile install-data-am
install-data-hook:	Makefile
	make -f Makefile install-data-hook
uninstall-am:	Makefile
	make -f Makefile uninstall-am

uninstall:
	make -f Makefile uninstall
install:
	make -f Makefile install
dirtyclean:
	rm -rf $(root)/sbin $(root)/share/man
Makefile:
	sh autogen.sh
	if [ \! -d ROOT ]; then mkdir ROOT ; fi
	./configure --prefix=`pwd`/ROOT
