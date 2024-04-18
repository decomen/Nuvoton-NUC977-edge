

DEF_CGI_HANDLER(devReset)
{
    rt_kprintf("vDoSystemReset\n");

    WEBS_PRINTF("{\"ret\":%d}", RT_EOK);
    WEBS_DONE(200);

    vDoSystemReset();
}

DEF_CGI_HANDLER(factoryReset)
{
    rt_kprintf("factoryReset\n");

    WEBS_PRINTF("{\"ret\":%d}", RT_EOK);
    WEBS_DONE(200);

    //dfs_mkfs( "elm", BOARD_SPIFLASH_FLASH_NAME );
    board_cfg_del_all();
    vDoSystemReboot();
}

DEF_CGI_HANDLER(diskFormat)
{
    rt_err_t err = RT_EOK;
    rt_kprintf("diskFormat\n");
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        const char *disk = cJSON_GetString(pCfg, "disk", "");
        if (disk) {
            if (0 == strcmp(disk, "spiflash")) {
                rt_kprintf("diskFormat spiflash\n");
                /*if (dfs_mkfs("elm", BOARD_SPIFLASH_FLASH_NAME) != 0) {
                    err = RT_ERROR;
                }*/
            } else if (0 == strcmp(disk, "sdcard")) {
                rt_kprintf("diskFormat sdcard\n");
                /*if (dfs_mkfs("elm", BOARD_SDCARD_NAME) != 0) {
                    err = RT_ERROR;
                }*/
            } else {
                err = RT_ERROR;
            }
        } else {
            err = RT_ERROR;
        }
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);

    vDoSystemReboot();
}

DEF_CGI_HANDLER(listDir)
{
    DIR *d = NULL;
    struct dirent *de;
    struct stat buf;
    int exists;
    int length;
    rt_bool_t first = RT_TRUE;
    const char *path = CGI_GET_ARG("path");
    char full_path[256];
    
    WEBS_PRINTF("{\"ret\":0,\"list\":[");
    if (path) {
        if ((d = opendir(path)) != NULL) {
            for (de = readdir(d); de != NULL; de = readdir(d)) {
                sprintf(full_path, "%s/%s", path, de->d_name);
                exists = stat(full_path, &buf);
                if (exists >= 0) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddStringToObject(pItem, "name", de->d_name);
                        cJSON_AddStringToObject(pItem, "type", S_ISDIR(buf.st_mode)?"dir":"file");
                        cJSON_AddNumberToObject(pItem, "size", buf.st_size);
                        {
                            struct tm lt;
                            char date[16] = { 0 };
                            das_localtime_r(&buf.st_mtime, &lt);
                            rt_sprintf(date, "%04d/%02d/%02d %02d:%02d:%02d",
                                       lt.tm_year + 1900,
                                       lt.tm_mon + 1,
                                       lt.tm_mday,
                                       lt.tm_hour,
                                       lt.tm_min,
                                       lt.tm_sec
                                      );
                            cJSON_AddStringToObject(pItem, "mtime", date);
                        }
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }
            closedir(d);
        }
    }
    WEBS_PRINTF("]}");
    WEBS_DONE(200);
}

// 为了安全不允许删除目录
DEF_CGI_HANDLER(delFile)
{
    const char *path = CGI_GET_ARG("path");
    char cmd[256 + 32];
    
    if (path) {
        sprintf(cmd, "rm -f %s", path);
        my_system(cmd);
    }
    WEBS_PRINTF("{\"ret\":0}");
    WEBS_DONE(200);
}


