#ifndef CUSTOMQLISTWIDGET_H
#define CUSTOMQLISTWIDGET_H

#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include "pfsfile.h"
#include "pfsfolder.h"
#include "diskfunctions.h"
#include <QFileDialog>
#include <QString>
#include <QInputDialog>
#include "mainwindow.h"
#include <QProgressBar>

class customQListWidget: public QListWidget
{
    Q_OBJECT

public:
    customQListWidget(QWidget *parent = 0);
    void setEstoyEnRoot(bool );
    ~customQListWidget();
    void showRootFiles();
    void showFolderFiles();
    QString getCurrentFolder();


private:
    QMenu *folderContextMenu, *fileContextMenu, *rootAccessContextMenu, *folderAcessContextMenu;
    void addActionsToFile();
    void addActionsToFolder();
    void addActionsToRootAccess();
    void addActionsToFolderAcess();
    void showDirEntries(vector<dir_info_t*>);
    bool estoyEnRoot;
    QString selectedFolder;

private slots:
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void AddFile();
    void AddFolder();
    void CopyFileFromSD();
    void RenameFile();
    void RenameFolder();
    void DeleteFile();
    void DeleteFolder();

signals:
    void folderAccess();

};

#endif // CUSTOMQLISTWIDGET_H
