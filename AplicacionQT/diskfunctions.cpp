#include "diskfunctions.h"

DiskFunctions *DiskFunctions::instance = 0;

//Nombre de la unidad SD que se crea en el /dev
QString sdPath = "/dev/sdb";

DiskFunctions::DiskFunctions()
{
    currentDirectory = "/";

    //TENER LA FAT EN MEMORIA
    this->progressBar = 0;
    FILE *file = fopen(sdPath.toStdString().c_str(),"r+");
    if(file == 0)
    {
        cout << "No hay sd en " << sdPath.toStdString() << endl;
        return;
    }

    this->sp = (pfs_boot_t*)malloc(sizeof(pfs_boot_t));
    sp = getSuperBlock();
    this->cantClusters = (sp->totalSectors + (sp->sectorsPerCluster*512 -1)) / sp->sectorsPerCluster;
    fat = (int*)malloc(sizeof(int)*cantClusters);


    this->firstRootCluster = sp->rootStartCluster;


    long ftStartPosition = 1*(sp->bytesPerSector*sp->sectorsPerCluster);
    fseek(file,ftStartPosition,0);
    fread(fat,sizeof(int),this->cantClusters,file);

    for(int i=0; i<600; i++)
        cout << "pos[" << i << "]: " << fat[i] << endl;


    fclose(file);
}

char* DiskFunctions::getSdName()
{
    return (char*)this->sp->volumeLabel;
}

void DiskFunctions::formatSd(char *volumeName)
{

    FILE *sd = fopen(sdPath.toStdString().c_str(), "r+");
    if(sd == 0)
    {
        cout << "No hay sd en " << sdPath.toStdString() << endl;
        return;
    }
    pfs_boot_t *superBlock = (pfs_boot_t*)malloc(sizeof(pfs_boot_t));

    fseek(sd,0, SEEK_END);
    long sdSize = ftell(sd);
    rewind(sd);

    int bytesPerSector = 512;
    int sectorsPerCluster = 8;
    long totalSectors = (long)sdSize/bytesPerSector;

    long cantClusters = (totalSectors + (bytesPerSector-1))/sectorsPerCluster;
    long FTableSectors = (cantClusters * sizeof(int)) / bytesPerSector;
    long totalClustersFTable = (FTableSectors + bytesPerSector - 1)/sectorsPerCluster;

    //ESCRIBIR EL SUPER BLOCK!
    superBlock->bytesPerSector = bytesPerSector;
    superBlock->FTableSectors = FTableSectors;
    superBlock->rootDirEntryCount = 0;
    superBlock->rootStartCluster = totalClustersFTable + 1;
    superBlock->sectorsPerCluster = sectorsPerCluster;
    superBlock->totalSectors = totalSectors;
    memcpy(superBlock->volumeLabel, volumeName, 11);

    fwrite(superBlock, sizeof(pfs_boot_t), 1, sd);


    //ESCRIBIR LA FAT!!!
    int *fat = (int*)malloc(sizeof(int)*cantClusters);
    long ftStartPosition = 1*(superBlock->bytesPerSector*superBlock->sectorsPerCluster);
    for(int i=0; i<totalClustersFTable+1+1; i++)
        fat[i] = -1;

    fseek(sd,ftStartPosition,0);
    fwrite(fat,sizeof(int),cantClusters,sd);


    //DESPUES DE ESCRIBIR LA FTABLE, ESCIBIR BOJOTES DE CEROS
    int clusterSize = superBlock->bytesPerSector * superBlock->sectorsPerCluster;
    char emptyCluster[clusterSize];
    memset(emptyCluster, 0, clusterSize);

    long clustersRestantes = cantClusters - (2 + FTableSectors + 1);

    //for(int i=0; i<=clustersRestantes; i++)
    for(int i=0; i<=10000; i++)
        fwrite(emptyCluster, sizeof(char), sizeof(emptyCluster), sd);


    QMessageBox a;
    a.setText("Se termino de formatear :P");

    //POTATO
    free(superBlock);
    fclose(sd);

    instance = 0;

    a.exec();
}

DiskFunctions* DiskFunctions::getInstace()
{
    if(!instance)
    {
        instance = new DiskFunctions();
    }

    return instance;
}

DiskFunctions::~DiskFunctions()
{
    free(fat);
}

PFS_boot* DiskFunctions::getSuperBlock()
{
    //POSICION DEL SUPER BLOCK
    long seekPosition = 0;

    FILE *file = fopen(sdPath.toStdString().c_str(),"r+");
    if(file == 0)
    {
        cout << "No hay sd en " << sdPath.toStdString() << endl;
        return 0;
    }

    pfs_boot_t *sp = (pfs_boot_t*)malloc(sizeof(pfs_boot_t));

    printf("Lei el super block\n");


    fseek(file,seekPosition,0);
    fread(sp,sizeof(pfs_boot_t),1,file);

    fclose(file);


    return sp;
}




dir_info_t* DiskFunctions::writeFileToSD(QString path)
{

    FILE *file = fopen(path.toStdString().c_str(),"r");
    fseek(file,0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    long uncopiedBytes = fileSize;
    int position;

    int sizeBuffer = this->sp->bytesPerSector * this->sp->sectorsPerCluster;
    char *buffer = (char*)malloc(sizeBuffer);

    FILE *sd = fopen(sdPath.toStdString().c_str(),"r+");
    int firstCluster = getFreeCluster();

    //AGREGADO
    long progress = uncopiedBytes;
    progress /= sizeBuffer;
    if(this->progressBar != 0)
        this->progressBar->setRange(0,progress);
    //^

//    fat[firstCluster] = -1;
    while(uncopiedBytes > sizeBuffer)
    {
        fread(buffer,1,sizeBuffer,file);

        position = getFreeCluster();
        long positionToWrite = position*sizeBuffer;
        fseek(sd,positionToWrite,0);
        fwrite(buffer , 1, sizeBuffer, sd);

        uncopiedBytes -= sizeBuffer;
        //AGREGADO
        progress = (fileSize);
        progress -= uncopiedBytes;
        progress /= sizeBuffer;
        if(this->progressBar != 0)
            this->progressBar->setValue(progress);
        //^
        if(uncopiedBytes > 0)
        {
            fat[position] = -1;
            int nextDataSector = getFreeCluster();
            fat[position] = nextDataSector;
        }
        cout << "escribi en cluster: " << position << endl;
    }

    if(uncopiedBytes == 0)
    {
        fat[position] = 0x0FFFFFFF;
    }
    else
    {
        char *temp = (char*)malloc(uncopiedBytes);
        fread(temp,1,uncopiedBytes,file);

        position = getFreeCluster();
        long positionToWrite = position*sizeBuffer;
        fseek(sd,0,positionToWrite);
        fwrite(buffer , 1, uncopiedBytes, file);

        fat[position] = 0x0FFFFFFF;

        cout << "escribi en cluster: " << position << endl;
    }
    //AGREGADO
    progress = fileSize;
    progress /= sizeBuffer;
    if(this->progressBar != 0) this->progressBar->setValue(progress);
    //^

    //ARMAR EL D_ENTRY Y ESCRIBIRLO
    dir_t *de = (dir_t*)malloc(sizeof(dir_t));

    de->attributes = 0x07;
    de->fileSize = fileSize;
    de->firstCluster = firstCluster;
    QString fileName = path.split("/").last().replace(".wav","");

    mempcpy(de->name,fileName.toStdString().c_str(), sizeof(de->name));
    int i;
    for(i=0; i<sizeof(de->name); i++)
    {
        if(de->name[i] == '\0')
            break;

        de->name[i] = QChar::fromAscii(de->name[i]).toUpper().toAscii();
    }
    for(i=i; i<sizeof(de->name); i++)
        de->name[i] = ' ';

    de->name[8] = 'W';
    de->name[9] = 'A';
    de->name[10] = 'V';


    dir_info_t* dirInfo = (dir_info_t*)malloc(sizeof(dir_info_t));

    //ESCRIBIR EL DIR_ENTRY
    if(currentDirectory == "/")
    {
        dirInfo = writeDirEntry(de, sp->rootStartCluster, this->sp->rootDirEntryCount*sizeof(dir_t));
        updateSuperBlock();
    }
    else
    {
        //SI LA CARPETITA ESTA VACIA
        if(this->currentFolder->dirEntry->firstCluster == 0x0FFFFFFF)
        {
            int freeCluster = getFreeCluster();
            fat[freeCluster] = 0x0FFFFFFF;
            this->currentFolder->dirEntry->firstCluster = freeCluster;
        }

        //TODO: FALTA EL ALGORITMO PARA BUSCAR EL SIGUIENTE CLUSTER
        dirInfo = writeDirEntry(de, this->currentFolder->dirEntry->firstCluster, this->currentFolder->dirEntry->fileSize);
        this->currentFolder->dirEntry->fileSize++;
        updateDirEntry(this->currentFolder->dirEntry, this->currentFolder->clusterPosition, this->currentFolder->offset);
    }

    //free(de);
    free(buffer);
    fclose(file);
    fclose(sd);
    cout << "Saliendo de write file\n";

    return dirInfo;
}

dir_info_t* DiskFunctions::createFolder(QString folderName)
{
    //ARMAR EL D_ENTRY Y ESCRIBIRLO
    dir_t *de = (dir_t*)malloc(sizeof(dir_t));

    de->attributes = 0x10;
    de->fileSize = 0;
    de->firstCluster = 0x0FFFFFFF;

    mempcpy(de->name,folderName.toStdString().c_str(), sizeof(de->name));
    int i;
    for(i=0; i<sizeof(de->name); i++)
    {
        if(de->name[i] == '\0')
            break;

        de->name[i] = QChar::fromAscii(de->name[i]).toUpper().toAscii();
    }
    for(i=i; i<sizeof(de->name); i++)
        de->name[i] = ' ';

    //LOS FOLDERS LOS METO EN ROOT
    dir_info_t* dirInfo = writeDirEntry(de, sp->rootStartCluster, sp->rootDirEntryCount);
    this->sp->rootDirEntryCount++;
    updateSuperBlock();


    return dirInfo;

    //free(de);
}

dir_info_t* DiskFunctions::writeDirEntry(dir_t *dirEntry, int dirEntryCluster, int dirEntryCount)
{
    dir_info_t* dirInfo = (dir_info_t*)malloc(sizeof(dir_info_t));

    //VOY A VER DONDE METO ESTE DIR_ENTRY :E

    int currentCluster = dirEntryCluster;
    int clusterSize = this->sp->bytesPerSector * this->sp->sectorsPerCluster;
    int bytesLeidos = 0;
    long dirEntryStartPosition = currentCluster * clusterSize;

    FILE *sd = fopen(sdPath.toStdString().c_str(), "r+");

    if(sd == 0)
    {
        cout << "No hay SD Card en " << sdPath.toStdString() << endl;
        free(dirInfo);
        return 0;
    }

    fseek(sd, dirEntryStartPosition, 0);
    dir_t *temp = (dir_t*)malloc(sizeof(dir_t));
    bool seEscribioDirEntry = false;
    for(int i=0; i<dirEntryCount; i++)
    {
        if(bytesLeidos == clusterSize)
        {
            currentCluster = fat[currentCluster];
            bytesLeidos = 0;
        }

        fread(temp,1,sizeof(dir_t),sd);
        bytesLeidos += sizeof(dir_t);
        //SI ENCONTRE UN DIR_ENTRY ELIMINADO, SOBREESCRIBIRLO
        if(temp->attributes == 0x05)
        {
            bytesLeidos -= sizeof(dir_t);
            fseek(sd,(currentCluster*clusterSize) + bytesLeidos,0);
            fwrite(dirEntry, sizeof(dir_t), 1, sd);
            seEscribioDirEntry = true;
            break;
        }
    }

    if(!seEscribioDirEntry)
    {
        //VERIFICAR SI HAY QUE ESCRIBIRLO EN UN NUEVO CLUSTER PARA PODER ENLAZAR Y MARCAR EL NUEVO COMO NO DISPONIBLE
        if(bytesLeidos == clusterSize)
        {
            int nextCluster = getFreeCluster();
            fat[currentCluster] = nextCluster;
            currentCluster = nextCluster;
            fat[currentCluster] = 0x0FFFFFFF;
            bytesLeidos = 0;
        }

        fwrite(dirEntry, sizeof(dir_t), 1, sd);
    }

    cout << "Escribi \"dirEntry\" en el cluster:" << dirEntryCluster << " con offset de:" << bytesLeidos << endl;

    //Escribir la FTable
    writeFTable();

    free(temp);
    fclose(sd);

    dirInfo->dirEntry = dirEntry;
    dirInfo->clusterPosition = currentCluster;
    //TODO: Calcular bien el offset
    dirInfo->offset = bytesLeidos;

    return dirInfo;
}

void DiskFunctions::updateDirEntry(dir_t *dirEntry, int dirEntryCluster, int offset)
{
    FILE *sd = fopen(sdPath.toStdString().c_str(), "r+");

    if(sd == 0)
    {
        cout << "No hay SD Card en " << sdPath.toStdString() << endl;
        return;
    }

    long clusterSize = this->sp->bytesPerSector * this->sp->sectorsPerCluster;
    long dirEntryStartPosition = dirEntryCluster * clusterSize;
    long seekPosition = dirEntryStartPosition + offset;

    fseek(sd,seekPosition,0);
    fwrite(dirEntry,sizeof(dir_t),1,sd);

    fclose(sd);

}


void DiskFunctions::updateSuperBlock()
{
    long seekPosition = 0;

    FILE *sd = fopen(sdPath.toStdString().c_str(), "r+");
    if(sd == 0)
    {
        cout << "No hay SD Card en " << sdPath.toStdString() << endl;
        return;
    }

    fseek(sd,seekPosition, 0);
    fwrite(this->sp, sizeof(pfs_boot_t), 1, sd);

    fclose(sd);
}



void DiskFunctions::writeFTable()
{
    long ftStartPosition = 1*(sp->bytesPerSector * sp->sectorsPerCluster);
    FILE *sd = fopen(sdPath.toStdString().c_str(), "r+");
    if(sd == 0)
    {
        cout << "No hay SD Card en " << sdPath.toStdString() << endl;
        return;
    }
    fseek(sd, ftStartPosition, 0);
    fwrite(fat,sizeof(int),this->cantClusters,sd);

    fclose(sd);
}

void DiskFunctions::swag()
{
    unsigned char buffer[100];
    FILE *swag = fopen(sdPath.toStdString().c_str(),"r");

    fseek(swag,543 * 4096,0);
    fread(buffer,100,sizeof(unsigned char),swag);

    fclose(swag);
    int x =123;
}

int DiskFunctions::getFreeCluster()
{
    for(int i=firstRootCluster + 1; i<this->cantClusters; i++)
    {
        if(fat[i] == 0)
            return i;
    }

    return 0;
}


vector<dir_info_t*> DiskFunctions::getRootFiles()
{
    return getDirEntries(this->sp->rootStartCluster, this->sp->rootDirEntryCount);
}

vector<dir_info_t*> DiskFunctions::getFolferFiles(dir_t *folder)
{
    return getDirEntries(folder->firstCluster,folder->fileSize);
}

vector<dir_info_t*> DiskFunctions::getDirEntries(int cluster, int count)
{
    vector<dir_info_t*> files;

    if(count == 0)
        return files;

    int rootDirEntryCount = count;
    long rootStartPosition = cluster * (this->sp->bytesPerSector*this->sp->sectorsPerCluster);

    FILE *sd = fopen(sdPath.toStdString().c_str(), "r+");
    if(sd == 0)
    {
        cout << "No hay SD Card en " << sdPath.toStdString() << endl;
        return files;
    }

    fseek(sd, rootStartPosition, 0);

    dir_t *dirEntry = (dir_t*)malloc(sizeof(dir_t));
    dir_info_t *dirEntryInfo = (dir_info_t*)malloc(sizeof(dir_info_t));

    int currentCluster = rootStartPosition / (this->sp->bytesPerSector*this->sp->sectorsPerCluster);
    int sizeCluster = this->sp->bytesPerSector * this->sp->sectorsPerCluster;
    int bytesLeidos = 0;

    while(rootDirEntryCount > 0)
    {
        fread(dirEntry,sizeof(dir_t),1,sd);

        if(dirEntry->attributes != 0x05)
        {
            dirEntryInfo->clusterPosition = currentCluster;
            dirEntryInfo->offset = bytesLeidos;
            dirEntryInfo->dirEntry = dirEntry;
            files.push_back(dirEntryInfo);
            rootDirEntryCount--;

            dirEntry = (dir_t*)malloc(sizeof(dir_t));
            dirEntryInfo = (dir_info_t*)malloc(sizeof(dir_info_t));
        }

        bytesLeidos += sizeof(dir_t);
        if(bytesLeidos == sizeCluster)
            currentCluster = fat[currentCluster];
    }

    return files;
}


void DiskFunctions::setCurrentFolder(dir_info_t * currentFolder)
{
    this->currentDirectory = "Folder= /" + QString::fromAscii((char*)currentFolder->dirEntry->name);
    this->currentFolder = currentFolder;
}

void DiskFunctions::setRootAsCurrentFolder()
{
    this->currentDirectory = "/";
}

void DiskFunctions::copyFileToPc(char *fileDst, dir_info_t *fileSrc)
{
    FILE *sd = fopen(sdPath.toStdString().c_str(), "r");
    if(sd == 0)
    {
        cout << "No hay SD Card en " << sdPath.toStdString() << endl;
        return;
    }


    FILE *file = fopen(fileDst, "w");
    if(file == 0)
    {
        cout << "No se pudo crear el archivo " << fileDst << endl;
        return;
    }

    long clusterSize = this->sp->bytesPerSector * this->sp->sectorsPerCluster;
    long sdOffset = fileSrc->clusterPosition * fileSrc->clusterPosition;
    sdOffset += fileSrc->offset;

    char *buffer = (char*)malloc(clusterSize);
    long uncopiedByes = fileSrc->dirEntry->fileSize;
    int clusterPosition = fileSrc->dirEntry->firstCluster;
    long fseekPosition = clusterPosition * clusterSize;

    //AGREGADO
    long progress = uncopiedByes;
    progress /= clusterSize;
    if(this->progressBar != 0) this->progressBar->setRange(0,progress);
    //^
    while(uncopiedByes > 0)
    {
        fseek(sd, fseekPosition, 0);

        if(uncopiedByes < clusterSize)
        {
            fread(buffer, 1, uncopiedByes, sd);
            fwrite(buffer,1,uncopiedByes,file);
            uncopiedByes = 0;
        }
        else
        {
            fread(buffer, 1, clusterSize, sd);
            fwrite(buffer,1,clusterSize,file);
            uncopiedByes -= clusterSize;
        }
        //AGREGADO
        progress = (fileSrc->dirEntry->fileSize);
        progress -= uncopiedByes;
        progress /= clusterSize;
        if(this->progressBar != 0)
            this->progressBar->setValue(progress);
        //^

        clusterPosition = fat[clusterPosition];
        fseekPosition = clusterPosition * clusterSize;
    }

    free(buffer);
    fclose(file);
    fclose(sd);
}

dir_info_t* DiskFunctions::renameFolder(char *newFolderName, dir_info_t *dirInfo)
{
    dir_t *de = dirInfo->dirEntry;

    mempcpy(de->name,newFolderName,sizeof(de->name));
    int i;
    for(i=0; i<sizeof(de->name); i++)
    {
        if(de->name[i] == '\0')
            break;

        de->name[i] = QChar::fromAscii(de->name[i]).toUpper().toAscii();
    }
    for(i=i; i<sizeof(de->name); i++)
        de->name[i] = ' ';

    updateDirEntry(de,dirInfo->clusterPosition,dirInfo->offset);

    //RETORNO EL DIR_ENTRY ACTUALIZADO
    dirInfo->dirEntry = de;
    return dirInfo;
}

dir_info_t* DiskFunctions::renameFile(char *newFileName, dir_info_t *dirInfo)
{
    dir_t *de = dirInfo->dirEntry;
    mempcpy(de->name,newFileName,sizeof(de->name));
    int i;
    for(i=0; i<sizeof(de->name); i++)
    {
        if(de->name[i] == '\0')
            break;

        de->name[i] = QChar::fromAscii(de->name[i]).toUpper().toAscii();
    }
    for(i=i; i<sizeof(de->name); i++)
        de->name[i] = ' ';

    de->name[8] = 'W';
    de->name[9] = 'A';
    de->name[10] = 'V';

    //SI LA CARPETA NO ESTA ELIMINADA, ACTUALIZAR EL DIR_ENTRY
    if(dirInfo->dirEntry->attributes != 0x05)
        updateDirEntry(de,dirInfo->clusterPosition,dirInfo->offset);

    //RETORNO EL DIR_ENTRY ACTUALIZADO
    dirInfo->dirEntry = de;
    return dirInfo;
}

void DiskFunctions::deleteFile(dir_info_t *dirInfo)
{
      dirInfo->dirEntry->attributes = 0x05;
      updateDirEntry(dirInfo->dirEntry, dirInfo->clusterPosition, dirInfo->offset);

      //liberar los bloques que usa el archivo en la FTable
      int currentCluster = dirInfo->dirEntry->firstCluster;
      int nextCluster = 0;
      int previousCluster = 0;

      FILE *sd = fopen(sdPath.toStdString().c_str(), "r+");
      if(sd == 0)
      {
          cout << "No hay SD Card en " << sdPath.toStdString() << endl;
          return;
      }

      long clusterPosition = 0;
      int clusterSize = this->sp->sectorsPerCluster * this->sp->bytesPerSector;
      char emptyBuffer[clusterSize];
      memset(emptyBuffer, 0, clusterSize);


      while(currentCluster != 0x0FFFFFFF)
      {
          //LIMPIAR EL CLUSTER
          clusterPosition = currentCluster * clusterSize;
          fseek(sd,clusterPosition,0);
          fwrite(emptyBuffer, sizeof(char), sizeof(emptyBuffer), sd);

          previousCluster = currentCluster;
          nextCluster = fat[currentCluster];
          fat[currentCluster] = 0;
          currentCluster = nextCluster;
      }
      fat[previousCluster] = 0;

      writeFTable();

      //POR ULTIMO, ACTUALIZAR LA CARPETA EN DONDE ESTOY
      this->currentFolder->dirEntry->fileSize--;
      updateDirEntry(this->currentFolder->dirEntry, this->currentFolder->clusterPosition, this->currentFolder->offset);
}


void DiskFunctions::deleteFolder(dir_info_t *dirInfo)
{
    vector<dir_info_t*> files = getFolferFiles(dirInfo->dirEntry);

    dirInfo->dirEntry->attributes = 0x05;
    this->currentFolder = dirInfo;
    updateDirEntry(dirInfo->dirEntry, dirInfo->clusterPosition, dirInfo->offset);

    //DESPUES DE ELIMIAR EL DIR_ENTRY, ACTUALIZO EL SUPER BLOCK
    this->sp->rootDirEntryCount--;
    updateSuperBlock();


    //si el firtCluster del dirEntry tiene ese valor, es porque nunca se la agrego un archivo
    //por ende, no hay nada por liberar nada en la FTable
    if(dirInfo->dirEntry->firstCluster == 0x0FFFFFFF)
        return;

    //liberar los bloques que usa el folder en la FTable
    int currentCluster = dirInfo->dirEntry->firstCluster;
    int nextCluster = 0;
    int previousCluster = dirInfo->dirEntry->firstCluster;

    while(currentCluster != 0x0FFFFFFF)
    {
        previousCluster = currentCluster;
        nextCluster = fat[currentCluster];
        fat[currentCluster] = 0;
        currentCluster = nextCluster;
    }

    fat[previousCluster] = 0;

    for(int i=0; i<files.size(); i++)
        deleteFile(files.at(i));
}

void DiskFunctions::setProgressBar(QProgressBar *pb){
    this->progressBar = pb;
}
