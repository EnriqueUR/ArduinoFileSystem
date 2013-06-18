#ifndef PFSFILE_H
#define PFSFILE_H

#include <QListWidgetItem>
#include <QMessageBox>
#include <QEvent>
#include "diskfunctions.h"

class pfsFile: public QObject, public QListWidgetItem
{
    Q_OBJECT

public:
    pfsFile(QListWidget *view = 0);
    void setDirEntry(dir_info_t* dEntry);
    dir_info_t *getDirEntry();
    ~pfsFile();

private:
    dir_info_t *dirEntry;
};

#endif // PFSFILE_H
