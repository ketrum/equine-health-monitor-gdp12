#ifndef __FATFS_H
#define __FATFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "ff.h"

DWORD get_fattime(void);
uint64_t FATFS_speedTest(uint32_t kilobytesToWrite);
bool FATFS_isFilesystemAvailable();
bool FATFS_testFilesystem();
void FATFS_initializeFilesystem();

#ifdef __cplusplus
}
#endif

#endif // __FATFS_H