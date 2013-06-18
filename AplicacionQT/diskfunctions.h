#ifndef DISKFUNCTIONS_H
#define DISKFUNCTIONS_H

#include <QString>
#include <stdio.h>
#include <iostream>
#include <SdPfsStructs.h>
#include <QStringList>
#include <vector>
#include <QProgressBar>
#include <QMessageBox>

using namespace std;

class DiskFunctions
{
public:
    static DiskFunctions* getInstace();

    //FUNCIONES DE LECTURA
    char* getSdName();
    pfs_boot_t* getSuperBlock();
    int getFreeCluster();
    vector<dir_info_t*> getRootFiles();
    vector<dir_info_t*> getFolferFiles(dir_t *folder);

    //FUNCIONES DE ESCRITURA
    void formatSd(char *volumeName);
    void copyFileToPc(char *fileDst, dir_info_t *fileSrc);
    dir_info_t* writeFileToSD(QString path);
    dir_info_t* createFolder(QString folderName);
    void setCurrentFolder(dir_info_t *);
    void setRootAsCurrentFolder();
    dir_info_t* renameFile(char *, dir_info_t *);
    dir_info_t* renameFolder(char *, dir_info_t *);
    void deleteFile(dir_info_t *);
    void deleteFolder(dir_info_t *);

    void swag();
    void setProgressBar(QProgressBar *);
private:
    //QString sdPath;
    QProgressBar *progressBar;
    QString currentDirectory;
    dir_info_t *currentFolder;
    int cantClusters;
    int *fat;
    pfs_boot_t *sp;
    int firstRootCluster;
    DiskFunctions();
    ~DiskFunctions();
    static DiskFunctions *instance;

    //FUNCIONES DE LECTURA
    vector<dir_info_t*> getDirEntries(int cluster, int count);

    //FUNCIONES DE ESCRITURA
    dir_info_t* writeDirEntry(dir_t *dirEntry, int dirEntryCluster, int dirEntryCount);
    void updateDirEntry(dir_t *dirEntry, int dirEntryCluster, int dirEntryCount);
    void writeFTable();
    void updateSuperBlock();

};

#endif // DISKFUNCTIONS_H
