CONFIG += compile_examples

HEADERS       = ftpwindow.h
SOURCES       = ftpwindow.cpp \
                main.cpp
RESOURCES    += ftp.qrc
QT           += network ftp widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qftp
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qftp
INSTALLS += target sources
