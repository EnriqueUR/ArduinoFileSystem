#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include "customqlistwidget.h"
#include <cstdio>
#include <fcntl.h>
//typedefs uint32_t
#include "stdint.h"
#include "diskfunctions.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionFormatSd_triggered();
    void enableBackAction();

    void on_actionBack_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
