TEMPLATE = subdirs

SUBDIRS += headersclean
SUBDIRS += qftp
SUBDIRS += cmake

# headersclean is the run target. The other tests need a live server or ctest.
qftp.CONFIG += no_default_target no_default_install
cmake.CONFIG += no_default_target no_default_install
