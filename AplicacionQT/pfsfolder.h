#ifndef PFSFOLDER_H
#define PFSFOLDER_H

#include <QListWidgetItem>
#include "diskfunctions.h"

class pfsFolder: public QObject, public QListWidgetItem
{
    Q_OBJECT

public:
    pfsFolder(QListWidget *view = 0);
    ~pfsFolder();
    void setDirEntry(dir_info_t* dEntry);
    dir_info_t *getDirEntry();

private:
    dir_info_t *dirEntry;
};

#endif // PFSFOLDER_H
