#include "pfsfolder.h"

pfsFolder::pfsFolder(QListWidget *view): QListWidgetItem(view)
{
    this->setIcon(QIcon(":images/folder.png"));
    this->setText("foldersito");
}


pfsFolder::~pfsFolder()
{

}


void pfsFolder::setDirEntry(dir_info_t* dEntry)
{
    this->dirEntry = dEntry;
}

dir_info_t* pfsFolder::getDirEntry()
{
    return this->dirEntry;
}
