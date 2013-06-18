struct superblock{
	uint8_t  sectorsPerCluster;
	uint32_t totalSectors;
	uint32_t fTableSectors;
	uint32_t rootStartCluster;
	uint16_t bytesPerSector; 	
	uint16_t rootDirEntryCount;
	uint8_t  volumeLabel[11];

	uint8_t  padding[512-1-4-4-4-2-2-11];
};typedef struct superblock sb_t; //512 bytes

struct directoryEntry{
	uint8_t  name[11];  
	uint8_t  attributes;
	uint32_t fileSize;
	uint32_t firstCluster;
	uint8_t  padding[12];
};typedef struct directoryEntry dir_t; //32 bytes

#define DIR_NAME_0XE5          0X05
        /** name[0] value for entry that is free after being "deleted" */
#define DIR_NAME_DELETED       ' '
        /** name[0] value for entry that is free and no allocated entries follow */
#define DIR_NAME_FREE          0X00
        /** file is read-only */
#define DIR_ATT_READ_ONLY      0X01
        /** File should hidden in directory listings */
#define DIR_ATT_HIDDEN         0X02
        /** Entry is for a system file */
#define DIR_ATT_SYSTEM         0X04
        /** Directory entry contains the volume label */
#define DIR_ATT_VOLUME_ID      0X08
        /** Entry is for a directory */
#define DIR_ATT_DIRECTORY      0X10
       /** Old DOS archive bit for backup support */
#define DIR_ATT_ARCHIVE        0X20
       /** Test value for long name entry.  Test is
           d->attributes & DIR_ATT_LONG_NAME_MASK) == DIR_ATT_LONG_NAME. */
#define DIR_ATT_LONG_NAME      0X0F
        /** Test mask for long name entry */
#define DIR_ATT_LONG_NAME_MASK 0X3F
        /** defined attribute bits */
#define DIR_ATT_DEFINED_BITS   0X3F
        /** Directory entry is part of a long name */
#define DIR_IS_LONG_NAME(dir)\
           (((dir).attributes & DIR_ATT_LONG_NAME_MASK) == DIR_ATT_LONG_NAME)
        /** Mask for file/subdirectory tests */
#define DIR_ATT_FILE_TYPE_MASK (DIR_ATT_VOLUME_ID | DIR_ATT_DIRECTORY)
        /** Directory entry is for a file */
#define DIR_IS_FILE(dir) (((dir).attributes & DIR_ATT_FILE_TYPE_MASK) == 0)
        /** Directory entry is for a subdirectory */
#define DIR_IS_SUBDIR(dir)\
            (((dir).attributes & DIR_ATT_FILE_TYPE_MASK) == DIR_ATT_DIRECTORY)
        /** Directory entry is for a file or subdirectory */
#define DIR_IS_FILE_OR_SUBDIR(dir) (((dir).attributes & DIR_ATT_VOLUME_ID) == 0)

