#ifndef FTPWINDOW_H
#define FTPWINDOW_H

#include <QDialog>
#include <QHash>

class QFile;
class QFtp;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class QUrlInfo;

class FtpWindow : public QDialog
{
    Q_OBJECT

public:
    explicit FtpWindow(QWidget *parent = 0);
    QSize sizeHint() const;

private slots:
    void connectOrDisconnect();
    void downloadFile();
    void uploadFile();
    void cancelTransfer();
    void refreshList();
    void cdToParent();
    void browseSaveDir();
    void ftpCommandFinished(int id, bool error);
    void addToList(const QUrlInfo &urlInfo);
    void processItem(QTreeWidgetItem *item, int column);
    void updateDataTransferProgress(qint64 readBytes, qint64 totalBytes);
    void enableDownloadButton();
    void connectTimedOut();

private:
    void connectToFtp();
    void dropConnection();
    void resetTransfer(bool removePartial);
    void finishTransfer(bool error);
    void enterDirectory(const QString &name);
    QString selectedName() const;

    QLineEdit *hostEdit;
    QLineEdit *portEdit;
    QLineEdit *userEdit;
    QLineEdit *passwordEdit;
    QLineEdit *saveDirEdit;
    QLabel *pathLabel;
    QLabel *statusLabel;
    QTreeWidget *fileList;
    QPushButton *connectButton;
    QPushButton *upButton;
    QPushButton *refreshButton;
    QPushButton *downloadButton;
    QPushButton *uploadButton;
    QPushButton *cancelButton;
    QProgressBar *progressBar;

    QHash<QString, bool> isDirectory;
    QString currentPath;
    QFtp *ftp;
    QFile *transferFile;
    bool transferIsDownload;
    QTimer *connectTimer;
};

#endif
