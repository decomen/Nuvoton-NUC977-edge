

#include <board.h>


/*

//擦除大小为块大小64KB,数据以64K为单位进行写入。
//nv分区:SPIFLASH偏移  0x90000 - 0x100000  (0x70000--->448K) (nv分区，存放出厂信息等)

*/


void gen_dev_json_file()
{
    cJSON *json =  cJSON_CreateObject();
    if(json){
        cJSON_AddStringToObject(json, "DeviceSn", g_sys_info.SN);
        cJSON_AddStringToObject(json, "DeviceName", g_sys_info.DEV_ID);
        cJSON_AddStringToObject(json, "DeviceHwVer", g_sys_info.HW_VER);
        cJSON_AddStringToObject(json, "DeviceSwVer", g_sys_info.SW_VER);
        cJSON_AddStringToObject(json, "DeviceHwId", g_sys_info.HW_ID);
        cJSON_AddStringToObject(json, "DeviceProDate", g_sys_info.PROD_DATE);
        cJSON_AddStringToObject(json, "VerDesc", g_sys_info.DESC);
        cJSON_AddNumberToObject(json, "DeviceModel", g_sys_info.DEV_MODEL);
        cJSON_AddNumberToObject(json, "DeviceRegStatus", g_sys_info.REG_STATUS);
        cJSON_AddNumberToObject(json, "DeviceRemain", g_sys_info.TEST_REMAIN);
    }
    char *json_str = cJSON_PrintUnformatted(json);
    printf("json_str: %s\r\n", json_str);
    FILE * fp = NULL;
    fp = fopen("/tmp/deviceInfo.json", "w+");
	if(fp==NULL){
		printf("Cannot open %s!\n", "/tmp/deviceInfo.json");
		return;
	}
    fprintf(fp, "%s",json_str);
    fclose(fp);

    cJSON_Delete(json);
}



int ReadDevInfo(void)
{
    vDevCfgInit();
        
}


int WriteDevInfo(char *DevId, char *psn, char *pro_date)
{
    char cmd[128] = {0};
    sprintf(cmd,"main write %s %s %s",DevId,psn,pro_date);
    system(cmd);
    system("main gen_json");
    printf("write devinfo pn: %s, sn: %s, pro_date: %s\r\n\n", DevId,psn, pro_date );
    return 0;
}



