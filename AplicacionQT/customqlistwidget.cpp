#include "customqlistwidget.h"

customQListWidget::customQListWidget(QWidget *parent):QListWidget(parent)
{
    this->setViewMode(QListView::IconMode);
    this->setMovement(QListView::Static);
    this->setIconSize(QSize(50,50));
    this->setGridSize(QSize(80,80));

    //Agregar las acciones a los contextMenus
    addActionsToFile();
    addActionsToFolder();
    addActionsToFolderAcess();
    addActionsToRootAccess();

    this->estoyEnRoot = true;

    this->showRootFiles();
}

void customQListWidget::setEstoyEnRoot(bool estoyEnRoot)
{
    this->estoyEnRoot = estoyEnRoot;
}

void customQListWidget::addActionsToFile()
{
    this->fileContextMenu = new QMenu(this);

    QAction *qAction1 = new QAction("Copy to...", fileContextMenu);
    connect(qAction1, SIGNAL(triggered()),this,SLOT(CopyFileFromSD()));
    fileContextMenu->addAction(qAction1);

    QAction *qAction2 = new QAction("Rename File", fileContextMenu);
    connect(qAction2, SIGNAL(triggered()),this,SLOT(RenameFile()));
    fileContextMenu->addAction(qAction2);

    QAction *qAction3 = new QAction("Delete File", fileContextMenu);
    connect(qAction3, SIGNAL(triggered()),this,SLOT(DeleteFile()));
    fileContextMenu->addAction(qAction3);

}

void customQListWidget::addActionsToFolder()
{
    this->folderContextMenu = new QMenu(this);

    QAction *qAction1 = new QAction("Rename Folder", folderContextMenu);
    connect(qAction1, SIGNAL(triggered()),this,SLOT(RenameFolder()));
    folderContextMenu->addAction(qAction1);

    QAction *qAction2 = new QAction("Delete Folder", folderContextMenu);
    connect(qAction2, SIGNAL(triggered()),this,SLOT(DeleteFolder()));
    folderContextMenu->addAction(qAction2);
}

void customQListWidget::addActionsToFolderAcess()
{
    this->folderAcessContextMenu = new QMenu(this);

    QAction *qAction1 = new QAction("Add File", folderAcessContextMenu);
    connect(qAction1, SIGNAL(triggered()),this,SLOT(AddFile()));
    folderAcessContextMenu->addAction(qAction1);
}

void customQListWidget::addActionsToRootAccess()
{
    this->rootAccessContextMenu = new QMenu(this);

    QAction *qAction1 = new QAction("Add Folder", rootAccessContextMenu);
    connect(qAction1, SIGNAL(triggered()),this,SLOT(AddFolder()));
    rootAccessContextMenu->addAction(qAction1);
}

customQListWidget::~customQListWidget()
{
}

void customQListWidget::mousePressEvent(QMouseEvent *event)
{
    if(!this->itemAt(event->pos()))
        if(this->selectedItems().count() != 0)
            this->selectedItems().at(0)->setSelected(false);

    if(event->button() == Qt::LeftButton)
    {
        QListWidget::mousePressEvent(event);
        return;
    }

    if(this->itemAt(event->pos()))
    {
        this->setCurrentItem(this->itemAt(event->pos()));
        this->itemAt(event->pos())->setSelected(true);
        QListWidgetItem *item = this->itemAt(event->pos());
        if(dynamic_cast<pfsFile*>(item) != 0)
            this->fileContextMenu->exec(event->globalPos());
        else
            this->folderContextMenu->exec(event->globalPos());
    }
    else
    {
        if(this->selectedItems().count() != 0)
            this->selectedItems().at(0)->setSelected(false);

        if(this->estoyEnRoot)
            this->rootAccessContextMenu->exec(event->globalPos());
        else
            this->folderAcessContextMenu->exec(event->globalPos());

    }

    QListWidget::mousePressEvent(event);
}

void customQListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(this->itemAt(event->pos()))
    {
        this->itemAt(event->pos())->setSelected(true);
        QListWidgetItem *item = this->itemAt(event->pos());
        if(dynamic_cast<pfsFolder*>(item) != 0)
        {
            //Le hago saber a DiskFunctions y al ListWidget que ya no estoy en root
            pfsFolder *temp = (pfsFolder *)this->selectedItems().at(0);
            DiskFunctions::getInstace()->setCurrentFolder(temp->getDirEntry());
            this->estoyEnRoot = false;

            this->selectedFolder = this->currentItem()->text();
            showFolderFiles();

            emit folderAccess();
        }
    }
}

QString customQListWidget::getCurrentFolder()
{
    return this->selectedFolder;
}

void customQListWidget::AddFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,tr("File to Copy"),"/home/moises/wavs/" , tr("Audio files (*.wav)"));
    if(filePath.isEmpty())
        return;

    pfsFile *f = new pfsFile(this);

    //ESCRIBO EL FILE
    dir_info_t *temp = DiskFunctions::getInstace()->writeFileToSD(filePath);

    //LE SETEO SU DIR_ENTRY Y LO AGREG
    f->setDirEntry(temp);

    //LO MUESTRO EN LIST WIDGET
    f->setText(QString::fromAscii((char*)temp->dirEntry->name, sizeof(temp->dirEntry->name) - 3));
    this->addItem(f);
}

void customQListWidget::AddFolder()
{
    bool ok;
    QString folderName = QInputDialog::getText(this,tr("New Folder..."),tr("Folder name: "),QLineEdit::Normal,
                                               "", &ok);
    if(ok && !folderName.isEmpty())
    {
        pfsFolder *f = new pfsFolder(this);

        //ESCRIBO LA CARPETA
        dir_info_t *temp = DiskFunctions::getInstace()->createFolder(folderName);

        //LE SETEO EL DIR_ENTRY
        f->setDirEntry(temp);

        //LO MUESTRO EN EL LIST WIDGET
        f->setText(QString::fromAscii((char*)temp->dirEntry->name, sizeof(temp->dirEntry->name)));
        this->addItem(f);

    }
}

void customQListWidget::showDirEntries(vector<dir_info_t *> files)
{
    this->clear();

    dir_t *dirEntry;
    dir_info_t *dirEntryInfo;
    for(int i=0; i<files.size(); i++)
    {
        dirEntryInfo = files.at(i);
        dirEntry = dirEntryInfo->dirEntry;
        dirEntry->name[8] = '\0';
        //SI EL DIR_ENTRY ES UNA CARPETA
        if(dirEntry->attributes == 0x10)
        {
            pfsFolder *folder = new pfsFolder();
            folder->setText(QString::fromAscii((char*)dirEntry->name));
            folder->setDirEntry(dirEntryInfo);
            this->addItem(folder);
        }
        else
        {
            pfsFile *file = new pfsFile();
            file->setText(QString::fromAscii((char*)dirEntry->name));
            file->setDirEntry(dirEntryInfo);
            this->addItem(file);
        }
    }
}

void customQListWidget::showRootFiles()
{    
    showDirEntries(DiskFunctions::getInstace()->getRootFiles());
}

void customQListWidget::showFolderFiles()
{
    pfsFolder *temp = (pfsFolder *)this->selectedItems().at(0);
    showDirEntries(DiskFunctions::getInstace()->getFolferFiles(temp->getDirEntry()->dirEntry));
}


void customQListWidget::CopyFileFromSD()
{
    QString filePath = QFileDialog::getSaveFileName(this,tr("File to Copy"),"/" , tr("Audio files (*.wav)"));

    pfsFile *f = (pfsFile *)this->currentItem();
    dir_info_t *temp = f->getDirEntry();

    DiskFunctions::getInstace()->copyFileToPc((char*)filePath.toStdString().c_str(), temp);
}

void customQListWidget::RenameFile()
{
    bool ok;
    QString newFileName = QInputDialog::getText(this,tr("Renombrando..."),tr("Nuevo nombre el archivo: "),QLineEdit::Normal,
                                               "", &ok);

    if(ok && !newFileName.isEmpty())
    {
        pfsFile *f = (pfsFile *)this->currentItem();
        dir_info_t *temp = f->getDirEntry();

        newFileName.resize(sizeof(temp->dirEntry->name) - 3);

        temp = DiskFunctions::getInstace()->renameFile((char*)newFileName.toStdString().c_str(), temp);

        //ACTUALIZAR LA INFO DEL ITEM
        this->currentItem()->setText(QString::fromAscii((char*)temp->dirEntry->name, sizeof(temp->dirEntry->name) - 3));
        ((pfsFile*)this->currentItem())->setDirEntry(temp);
    }

}

void customQListWidget::RenameFolder()
{
    bool ok;
    QString newFolderName = QInputDialog::getText(this,tr("Renombrando..."),tr("Nuevo nombre de la carpeta: "),QLineEdit::Normal,
                                               "", &ok);

    if(ok && !newFolderName.isEmpty())
    {
        pfsFolder *f = (pfsFolder *)this->currentItem();
        dir_info_t *temp = f->getDirEntry();

        newFolderName.resize(sizeof(temp->dirEntry->name));

        DiskFunctions::getInstace()->renameFolder((char*)newFolderName.toStdString().c_str(), temp);

        //ACTUALIZAR LA INFO DEL ITEM
        this->currentItem()->setText(QString::fromAscii((char*)temp->dirEntry->name, sizeof(temp->dirEntry->name)));
        ((pfsFolder*)this->currentItem())->setDirEntry(temp);
    }
}


void customQListWidget::DeleteFile()
{
    pfsFile *f = (pfsFile *)this->currentItem();
    dir_info_t *temp = f->getDirEntry();

    DiskFunctions::getInstace()->deleteFile(temp);
    this->takeItem(this->currentRow());
}

void customQListWidget::DeleteFolder()
{
    pfsFolder *f = (pfsFolder *)this->selectedItems().at(0);
    dir_info_t *temp = f->getDirEntry();

    DiskFunctions::getInstace()->deleteFolder(temp);
    this->takeItem(this->currentRow());
}
