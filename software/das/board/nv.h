
#ifndef _NV_H_
#define _NV_H_

#define NV_DEV_FILE "/dev/mtd2"
#define NV_SIZE (0x70000)

#define FACTORY_OFFSET  0                           //出厂信息存放位置分区偏移地址
#define VER_INFO_OFFSET (FACTORY_OFFSET + 64*1024)  //当前版本信息存放位置。

#define UPDATE_FLAG_OFFSET  (NV_SIZE - 64*1024)               //升级信息标志位信息存储偏移地址
#define REG_INFO_OFFSET     (UPDATE_FLAG_OFFSET - 64*1024)    //试用信息存储偏移地址
#define ADC_INFO_OFFSET     (REG_INFO_OFFSET - 64*1024)       //ADC校验信息偏移地址
#define KEY_INFO_OFFSET     (ADC_INFO_OFFSET - 64*1024)        //激活码存储位置

int block_write(char *blkname, off_t offset, void *data, size_t size);
void block_erase(int offset);
int block_read(char *blkname, off_t offset, void *data, size_t size);

#endif

