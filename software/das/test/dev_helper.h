#ifndef __DEV_HELPER_H__
#define __DEV_HELPER_H__

typedef struct 
{
	das_system_info_t devinfo;
	unsigned int ulCrc32;
}s_devinfo_packet_t;

int ReadDevInfo(void);
int WriteDevInfo(char *DevId, char *psn, char *pro_date);
void gen_dev_json_file();

int vGetDevUUID(char *uid,char *mac);

#endif
