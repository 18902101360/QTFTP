# QtFtp

这是一个 FTP 客户端库，类名是 `QFtp`。协议是 FTP（TCP 21 端口），不是 TFTP。

自己写程序时，把下面 4 个文件复制进工程，放在同一个目录：

- `src/qftp/qftp.h`
- `src/qftp/qftp.cpp`
- `src/qftp/qurlinfo.h`
- `src/qftp/qurlinfo.cpp`

用 **Desktop Qt 5.14.1 MinGW 64-bit** 编译。`.pro` 里加上：

```qmake
QT += core network
HEADERS += qftp.h qurlinfo.h
SOURCES += qftp.cpp qurlinfo.cpp
```

代码里 `#include "qftp.h"`。

`connectToHost`、`login`、`list`、`get`、`put` 都是立刻返回的。真正的结果在 `commandFinished(int id, bool error)` 里。`error` 为 false 表示这条命令成功。不要在这个回调里弹出模态对话框，也不要在回调里直接 `delete` 掉 `QFtp`。

## 示例

连上 FTP 服务器，登录，列出当前目录，再把 `AT32F415C8T7_bldc.zip` 下载到程序当前目录。

`main.cpp`：

```cpp
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include "qftp.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QFtp ftp;
    QFile file("AT32F415C8T7_bldc.zip");
    QTextStream out(stdout);

    QObject::connect(&ftp, &QFtp::listInfo, [&](const QUrlInfo &info) {
        if (info.name() == "." || info.name() == "..")
            return;
        out << (info.isDir() ? "dir  " : "file ")
            << info.size() << "  " << info.name() << endl;
    });

    QObject::connect(&ftp, &QFtp::commandFinished, [&](int, bool error) {
        if (error) {
            out << "failed: " << ftp.errorString() << endl;
            app.exit(1);
            return;
        }

        switch (ftp.currentCommand()) {
        case QFtp::Login:
            ftp.list();
            break;
        case QFtp::List:
            if (!file.open(QIODevice::WriteOnly)) {
                out << "cannot create local file" << endl;
                app.exit(1);
                return;
            }
            ftp.get("AT32F415C8T7_bldc.zip", &file, QFtp::Binary);
            break;
        case QFtp::Get:
            file.close();
            out << "saved " << file.fileName() << " " << file.size() << " bytes" << endl;
            app.quit();
            break;
        default:
            break;
        }
    });

    ftp.connectToHost("169.254.4.61", 21);
    ftp.login("admin", "qt");
    return app.exec();
}
```

`.pro`：

```qmake
QT += core network
CONFIG += console
TARGET = ftpdemo
HEADERS += qftp.h qurlinfo.h
SOURCES += main.cpp qftp.cpp qurlinfo.cpp
```

服务器地址、账号、密码按实际修改。下载和上传用 `QFtp::Binary`。`QFile` 要一直活到对应的 `commandFinished` 到来之后再关闭。
