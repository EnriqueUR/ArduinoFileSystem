#define SPI_SD_INIT_RATE 11

/** ls() flag to print file size */
uint8_t const LS_SIZE = 2;
/** ls() flag for recursive list of subdirectories */
uint8_t const LS_R = 4;

//open flags
/** open() oflag for reading */
uint8_t const O_READ = 0X01;
/** open() oflag - same as O_IN */
uint8_t const O_RDONLY = O_READ;
/** open() oflag for write */
uint8_t const O_WRITE = 0X02;
/** open() oflag - same as O_WRITE */
uint8_t const O_WRONLY = O_WRITE;
/** open() oflag for reading and writing */
uint8_t const O_RDWR = (O_READ | O_WRITE);
/** open() oflag mask for access modes */
uint8_t const O_ACCMODE = (O_READ | O_WRITE);
/** create the file if nonexistent */
uint8_t const O_CREAT = 0X40;

//seek constants
uint8_t const SEEK_BEG_ = 1;
uint8_t const SEEK_CUR_ = 2;
uint8_t const SEEK_END_ = 4;