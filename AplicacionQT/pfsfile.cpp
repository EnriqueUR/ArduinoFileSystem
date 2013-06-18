#include "pfsfile.h"

pfsFile::pfsFile(QListWidget *view): QListWidgetItem(view)
{
    this->setIcon(QIcon(":images/file.png"));
    this->setText("potato");
}

pfsFile::~pfsFile()
{

}

void pfsFile::setDirEntry(dir_info_t* dEntry)
{
    this->dirEntry = dEntry;
}

dir_info_t* pfsFile::getDirEntry()
{
    return this->dirEntry;
}
