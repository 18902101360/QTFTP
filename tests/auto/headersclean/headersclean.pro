QT = core testlib ftp
CONFIG += console
TARGET = headersclean
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

# This Qt install has no tests/auto/other/headersclean/headersclean.pri.
# Generate one translation unit per public header, plus a main(), so the
# test executable exists and each header compiles on its own.

headerPri = $$shadowed($$dirname(_QMAKE_CONF_))/include/QtFtp/headers.pri
exists($$headerPri): include($$headerPri)
isEmpty(SYNCQT.HEADER_FILES): SYNCQT.HEADER_FILES = qftp.h qurlinfo.h

checkDir = $$OUT_PWD/headercheck
!exists($$checkDir): system($$sprintf($$QMAKE_MKDIR_CMD, $$shell_path($$checkDir)))

for (hdr, SYNCQT.HEADER_FILES) {
    base = $$replace(hdr, \\.[^.]+$, )
    cpp = $$checkDir/$${base}.cpp
    lines = \
        "$${LITERAL_HASH}include <QtFtp/$$hdr>" \
        "void headersclean_$${base}() {}"
    write_file($$cpp, lines)|error("Cannot write $$cpp")
    SOURCES += $$cpp
}

mainCpp = $$checkDir/main.cpp
mainLines = "int main(int, char **) { return 0; }"
write_file($$mainCpp, mainLines)|error("Cannot write $$mainCpp")
SOURCES += $$mainCpp
