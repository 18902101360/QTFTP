#include "ftpwindow.h"

#include <QtFtp>
#include <QtWidgets>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

namespace {
QString defaultSaveDir()
{
    const QString ziliao = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
            + QLatin1String("/ziliao");
    if (QDir(ziliao).exists())
        return ziliao;
    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!downloads.isEmpty())
        return downloads;
    return QDir::homePath();
}
}

FtpWindow::FtpWindow(QWidget *parent)
    : QDialog(parent), ftp(0), transferFile(0), transferIsDownload(true), connectTimer(new QTimer(this))
{
    hostEdit = new QLineEdit(QLatin1String("169.254.4.61"));
    portEdit = new QLineEdit(QLatin1String("21"));
    portEdit->setValidator(new QIntValidator(1, 65535, portEdit));
    portEdit->setMaximumWidth(70);
    userEdit = new QLineEdit(QLatin1String("admin"));
    userEdit->setMaximumWidth(120);
    passwordEdit = new QLineEdit(QLatin1String("qt"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMaximumWidth(120);
    connectButton = new QPushButton(tr("Connect"));
    connectButton->setDefault(true);

    upButton = new QPushButton(tr("Up"));
    upButton->setEnabled(false);
    refreshButton = new QPushButton(tr("Refresh"));
    refreshButton->setEnabled(false);
    pathLabel = new QLabel(QLatin1String("/"));

    fileList = new QTreeWidget;
    fileList->setEnabled(false);
    fileList->setRootIsDecorated(false);
    fileList->setAlternatingRowColors(true);
    fileList->setSelectionMode(QAbstractItemView::SingleSelection);
    fileList->setHeaderLabels(QStringList() << tr("Name") << tr("Size") << tr("Owner") << tr("Group") << tr("Time"));
    fileList->header()->setStretchLastSection(false);
    fileList->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    saveDirEdit = new QLineEdit(defaultSaveDir());
    QPushButton *browseButton = new QPushButton(tr("Browse"));
    downloadButton = new QPushButton(tr("Download"));
    uploadButton = new QPushButton(tr("Upload"));
    cancelButton = new QPushButton(tr("Cancel"));
    downloadButton->setEnabled(false);
    uploadButton->setEnabled(false);
    cancelButton->setEnabled(false);

    progressBar = new QProgressBar;
    progressBar->setRange(0, 1000);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    statusLabel = new QLabel(tr("Test server 169.254.4.61, user admin."));
    statusLabel->setWordWrap(true);

    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectOrDisconnect()));
    connect(upButton, SIGNAL(clicked()), this, SLOT(cdToParent()));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(refreshList()));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(browseSaveDir()));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadFile()));
    connect(uploadButton, SIGNAL(clicked()), this, SLOT(uploadFile()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelTransfer()));
    connect(fileList, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(processItem(QTreeWidgetItem*,int)));
    connect(fileList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(enableDownloadButton()));
    connectTimer->setSingleShot(true);
    connect(connectTimer, SIGNAL(timeout()), this, SLOT(connectTimedOut()));

    QHBoxLayout *loginLayout = new QHBoxLayout;
    loginLayout->addWidget(new QLabel(tr("Server")));
    loginLayout->addWidget(hostEdit, 1);
    loginLayout->addWidget(new QLabel(tr("Port")));
    loginLayout->addWidget(portEdit);
    loginLayout->addWidget(new QLabel(tr("User")));
    loginLayout->addWidget(userEdit);
    loginLayout->addWidget(new QLabel(tr("Password")));
    loginLayout->addWidget(passwordEdit);
    loginLayout->addWidget(connectButton);

    QHBoxLayout *navLayout = new QHBoxLayout;
    navLayout->addWidget(upButton);
    navLayout->addWidget(refreshButton);
    navLayout->addWidget(pathLabel, 1);

    QHBoxLayout *saveLayout = new QHBoxLayout;
    saveLayout->addWidget(new QLabel(tr("Save to")));
    saveLayout->addWidget(saveDirEdit, 1);
    saveLayout->addWidget(browseButton);
    saveLayout->addWidget(downloadButton);
    saveLayout->addWidget(uploadButton);
    saveLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(loginLayout);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(fileList, 1);
    mainLayout->addLayout(saveLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(statusLabel);
    setLayout(mainLayout);
    setWindowTitle(tr("FTP"));
}

QSize FtpWindow::sizeHint() const
{
    return QSize(760, 520);
}

void FtpWindow::connectOrDisconnect()
{
    if (ftp) {
        dropConnection();
        statusLabel->setText(tr("Disconnected."));
        return;
    }
    connectToFtp();
}

void FtpWindow::connectToFtp()
{
    const QString host = hostEdit->text().trimmed();
    bool portOk = false;
    const quint16 port = portEdit->text().toUShort(&portOk);
    if (host.isEmpty() || !portOk) {
        statusLabel->setText(tr("Enter a server and a port."));
        return;
    }

    ftp = new QFtp(this);
    connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)));
    connect(ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(addToList(QUrlInfo)));
    connect(ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
            this, SLOT(updateDataTransferProgress(qint64,qint64)));

    fileList->clear();
    isDirectory.clear();
    currentPath = QLatin1String("/");
    pathLabel->setText(currentPath);
    progressBar->setValue(0);

    ftp->connectToHost(host, port);
    const QString user = userEdit->text();
    if (user.isEmpty())
        ftp->login();
    else
        ftp->login(user, passwordEdit->text());

    connectButton->setText(tr("Disconnect"));
    fileList->setEnabled(true);
    statusLabel->setText(tr("Connecting to %1:%2...").arg(host).arg(port));
    connectTimer->start(12000);
}

void FtpWindow::dropConnection()
{
    connectTimer->stop();
    resetTransfer(transferIsDownload);
    if (ftp) {
        QFtp *old = ftp;
        ftp = 0;
        old->abort();
        old->deleteLater();
    }
    fileList->setEnabled(false);
    upButton->setEnabled(false);
    refreshButton->setEnabled(false);
    uploadButton->setEnabled(false);
    downloadButton->setEnabled(false);
    cancelButton->setEnabled(false);
    connectButton->setText(tr("Connect"));
    connectButton->setEnabled(true);
    progressBar->setRange(0, 1000);
    progressBar->setValue(0);
}

void FtpWindow::connectTimedOut()
{
    if (!ftp)
        return;
    const QFtp::State state = ftp->state();
    if (state == QFtp::HostLookup || state == QFtp::Connecting || state == QFtp::Unconnected) {
        statusLabel->setText(tr("Connection to %1 timed out.").arg(hostEdit->text().trimmed()));
        dropConnection();
    }
}

void FtpWindow::resetTransfer(bool removePartial)
{
    if (!transferFile)
        return;
    const QString path = transferFile->fileName();
    transferFile->close();
    if (removePartial && transferIsDownload)
        transferFile->remove();
    delete transferFile;
    transferFile = 0;
    cancelButton->setEnabled(false);
}

void FtpWindow::downloadFile()
{
    if (!ftp || transferFile)
        return;
    const QString name = selectedName();
    if (name.isEmpty() || isDirectory.value(name))
        return;

    QDir dir(saveDirEdit->text().trimmed());
    if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) {
        statusLabel->setText(tr("Cannot create %1").arg(dir.absolutePath()));
        return;
    }

    QFile *out = new QFile(dir.filePath(name), this);
    if (!out->open(QIODevice::WriteOnly)) {
        statusLabel->setText(tr("Cannot write %1: %2").arg(out->fileName(), out->errorString()));
        delete out;
        return;
    }

    transferFile = out;
    transferIsDownload = true;
    cancelButton->setEnabled(true);
    downloadButton->setEnabled(false);
    progressBar->setRange(0, 0);
    progressBar->setValue(0);
    statusLabel->setText(tr("Downloading %1...").arg(name));
    ftp->get(name, transferFile, QFtp::Binary);
}

void FtpWindow::uploadFile()
{
    if (!ftp || transferFile || ftp->state() != QFtp::LoggedIn)
        return;
    const QString local = QFileDialog::getOpenFileName(this, tr("Upload file"));
    if (local.isEmpty())
        return;
    QFile *in = new QFile(local, this);
    if (!in->open(QIODevice::ReadOnly)) {
        statusLabel->setText(tr("Cannot read %1: %2").arg(local, in->errorString()));
        delete in;
        return;
    }
    transferFile = in;
    transferIsDownload = false;
    cancelButton->setEnabled(true);
    progressBar->setRange(0, 1000);
    progressBar->setValue(0);
    const QString remote = QFileInfo(local).fileName();
    statusLabel->setText(tr("Uploading %1...").arg(remote));
    ftp->put(transferFile, remote, QFtp::Binary);
}

void FtpWindow::cancelTransfer()
{
    if (!ftp || !transferFile)
        return;
    statusLabel->setText(tr("Canceling transfer..."));
    ftp->abort();
}

void FtpWindow::refreshList()
{
    if (!ftp || transferFile || ftp->state() != QFtp::LoggedIn)
        return;
    fileList->clear();
    isDirectory.clear();
    statusLabel->setText(tr("Listing %1...").arg(currentPath));
    ftp->list();
}

void FtpWindow::browseSaveDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Save downloads to"), saveDirEdit->text());
    if (!dir.isEmpty())
        saveDirEdit->setText(dir);
}

void FtpWindow::enterDirectory(const QString &name)
{
    if (!ftp || transferFile)
        return;
    fileList->clear();
    isDirectory.clear();
    if (currentPath.endsWith(QLatin1Char('/')))
        currentPath += name;
    else
        currentPath += QLatin1Char('/') + name;
    pathLabel->setText(currentPath);
    upButton->setEnabled(true);
    statusLabel->setText(tr("Opening %1...").arg(currentPath));
    ftp->cd(name);
    ftp->list();
}

void FtpWindow::cdToParent()
{
    if (!ftp || transferFile || ftp->state() != QFtp::LoggedIn)
        return;
    fileList->clear();
    isDirectory.clear();
    const int slash = currentPath.lastIndexOf(QLatin1Char('/'));
    currentPath = slash <= 0 ? QString(QLatin1String("/")) : currentPath.left(slash);
    pathLabel->setText(currentPath);
    upButton->setEnabled(currentPath != QLatin1String("/"));
    statusLabel->setText(tr("Opening %1...").arg(currentPath));
    ftp->cd(currentPath == QLatin1String("/") ? QLatin1String("/") : currentPath);
    ftp->list();
}

void FtpWindow::ftpCommandFinished(int, bool error)
{
    if (!ftp)
        return;

    const QFtp::Command cmd = ftp->currentCommand();
    if (cmd == QFtp::ConnectToHost) {
        connectTimer->stop();
        if (error) {
            const QString message = tr("Connect failed: %1").arg(ftp->errorString());
            QTimer::singleShot(0, this, [this, message]() {
                statusLabel->setText(message);
                dropConnection();
            });
        }
        return;
    }

    if (cmd == QFtp::Login) {
        if (error) {
            const QString message = tr("Login failed: %1").arg(ftp->errorString());
            QTimer::singleShot(0, this, [this, message]() {
                statusLabel->setText(message);
                dropConnection();
            });
            return;
        }
        statusLabel->setText(tr("Logged in. Listing / ..."));
        uploadButton->setEnabled(true);
        refreshButton->setEnabled(true);
        ftp->list();
        return;
    }

    if (cmd == QFtp::Get || cmd == QFtp::Put) {
        finishTransfer(error);
        if (cmd == QFtp::Put && !error)
            refreshList();
        return;
    }

    if (cmd == QFtp::List) {
        if (fileList->topLevelItemCount() == 0)
            statusLabel->setText(tr("Empty directory %1").arg(currentPath));
        else
            statusLabel->setText(tr("%1 item(s) in %2").arg(fileList->topLevelItemCount()).arg(currentPath));
        enableDownloadButton();
        return;
    }

    if (cmd == QFtp::Cd && error)
        statusLabel->setText(tr("Cannot open directory: %1").arg(ftp->errorString()));
}

void FtpWindow::finishTransfer(bool error)
{
    if (!transferFile)
        return;
    const QString path = transferFile->fileName();
    const qint64 size = transferIsDownload ? transferFile->size() : QFileInfo(path).size();
    transferFile->close();
    if (error && transferIsDownload)
        transferFile->remove();
    delete transferFile;
    transferFile = 0;
    cancelButton->setEnabled(false);
    progressBar->setRange(0, 1000);
    progressBar->setValue(error ? 0 : 1000);
    enableDownloadButton();
    if (error)
        statusLabel->setText(tr("Transfer failed: %1").arg(ftp ? ftp->errorString() : QString()));
    else if (transferIsDownload)
        statusLabel->setText(tr("Saved %1 (%2 bytes)").arg(path).arg(size));
    else
        statusLabel->setText(tr("Uploaded %1 (%2 bytes)").arg(QFileInfo(path).fileName()).arg(size));
}

void FtpWindow::addToList(const QUrlInfo &urlInfo)
{
    const QString name = urlInfo.name();
    if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String(".."))
        return;

    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, name);
    item->setText(1, urlInfo.isDir() ? QString() : QString::number(urlInfo.size()));
    item->setText(2, urlInfo.owner());
    item->setText(3, urlInfo.group());
    item->setText(4, urlInfo.lastModified().isValid()
                  ? urlInfo.lastModified().toString(QLatin1String("yyyy-MM-dd hh:mm"))
                  : QString());
    item->setIcon(0, QPixmap(urlInfo.isDir() ? ":/images/dir.png" : ":/images/file.png"));
    isDirectory.insert(name, urlInfo.isDir());
    fileList->addTopLevelItem(item);
    if (!fileList->currentItem()) {
        fileList->setCurrentItem(item);
        fileList->setEnabled(true);
    }
}

void FtpWindow::processItem(QTreeWidgetItem *item, int)
{
    if (!item)
        return;
    const QString name = item->text(0);
    if (isDirectory.value(name))
        enterDirectory(name);
    else
        downloadFile();
}

void FtpWindow::updateDataTransferProgress(qint64 readBytes, qint64 totalBytes)
{
    if (totalBytes > 0) {
        progressBar->setRange(0, 1000);
        progressBar->setValue(int((readBytes * 1000) / totalBytes));
        progressBar->setFormat(tr("%1 / %2").arg(readBytes).arg(totalBytes));
    } else {
        progressBar->setRange(0, 0);
        progressBar->setFormat(tr("%1 bytes").arg(readBytes));
    }
}

void FtpWindow::enableDownloadButton()
{
    const QString name = selectedName();
    downloadButton->setEnabled(ftp && !transferFile && !name.isEmpty() && !isDirectory.value(name));
}

QString FtpWindow::selectedName() const
{
    QTreeWidgetItem *current = fileList->currentItem();
    return current ? current->text(0) : QString();
}
