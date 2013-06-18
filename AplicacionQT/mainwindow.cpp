#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->ui->mainToolBar->setIconSize(QSize(40,40));

    this->ui->mainToolBar->actions().at(0)->setDisabled(true);

    customQListWidget *temp = (customQListWidget*) this->ui->listWidget;
    connect(temp,SIGNAL(folderAccess()),this,SLOT(enableBackAction()));

    this->setWindowTitle(QString::fromAscii(DiskFunctions::getInstace()->getSdName()));
    DiskFunctions::getInstace()->setProgressBar(this->ui->progressBar);
}

void MainWindow::enableBackAction()
{
    this->ui->actionBack->setEnabled(true);
    this->ui->progressBar->setValue(0);


    customQListWidget *swag = (customQListWidget*)this->ui->listWidget;
    this->ui->label->setText("Path: /" + swag->getCurrentFolder());
}

void MainWindow::on_actionBack_triggered()
{
    this->ui->actionBack->setEnabled(false);
    this->ui->label->setText("Path: /");
    this->ui->progressBar->setValue(0);

    //Le hago saber a DiskFunctions y al ListWidget que ya no estoy en root
    customQListWidget *temp = (customQListWidget*) this->ui->listWidget;
    temp->setEstoyEnRoot(true);
    temp->showRootFiles();

}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionFormatSd_triggered()
{
    //TO DO
    bool ok;
    QString sdName = QInputDialog::getText(this,tr("Formateando..."),tr("Nuevo nombre para la SD: "),QLineEdit::Normal,
                                               "", &ok);

    if(ok && !sdName.isEmpty())
    {
        sdName.resize(11);
        this->ui->progressBar->setValue(0);
        DiskFunctions::getInstace()->formatSd((char*)sdName.toStdString().c_str());
        this->ui->listWidget->clear();
        ((customQListWidget*)this->ui->listWidget)->setEstoyEnRoot(true);
        this->ui->actionBack->setEnabled(false);
        //AGREGADO
        this->ui->label->setText("Path: /");

        this->setWindowTitle(sdName);
    }
}
