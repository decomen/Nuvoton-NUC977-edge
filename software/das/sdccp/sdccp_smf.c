
#include "board.h"

#include "sdccp_smf.h"
#include "stdlib.h"

static rt_bool_t s_smf_read_flag[BOARD_TCPIP_MAX];
static s_instrumentData_t s_smf_data_bak[BOARD_TCPIP_MAX];

void smf_open(int index)
{
    if ((s_CCDataQueue[index] = rt_mq_create("smf", sizeof(s_instrumentData_t), 2 * NET_CLI_NUMS, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("smf rt_mq_create falied..\n");
        return ;
    }
    s_smf_read_flag[index] = RT_FALSE;
}

void smf_close(int index)
{
    int cli;
    if (s_CCDataQueue[index]) {
        rt_mq_delete(s_CCDataQueue[index]);
        s_CCDataQueue[index] = RT_NULL;
    }
    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
        smf_close_cli(index, cli);
    }
    s_smf_read_flag[index] = RT_FALSE;
}

void smf_new_cli(int index, int cli)
{
    RT_KERNEL_FREE(s_pCCBuffer[index][cli]);
    s_pCCBuffer[index][cli] = RT_KERNEL_CALLOC(SDCCP_SMF_BUF_SIZE);
    s_CCBufferPos[index][cli] = 0;
    s_cc_reinit_flag[index][cli] = RT_FALSE;
    s_smf_read_flag[index] = RT_FALSE;
}

void smf_close_cli(int index, int cli)
{
    RT_KERNEL_FREE(s_pCCBuffer[index][cli]);
    s_pCCBuffer[index][cli] = RT_NULL;
    s_CCBufferPos[index][cli] = 0;
    s_cc_reinit_flag[index][cli] = RT_FALSE;
    s_smf_read_flag[index] = RT_FALSE;
}


void smf_start_work(int index)
{
    int cli;
    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
        s_CCBufferPos[index][cli] = 0;
        s_cc_reinit_flag[index][cli] = RT_FALSE;
    }
}

void smf_exit_work(int index)
{
    int cli;
    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
        s_CCBufferPos[index][cli] = 0;
        s_cc_reinit_flag[index][cli] = RT_FALSE;
    }
}

static void __smf_parse_data(int index, char *buf, int len)
{
    if (s_CCDataQueue[index]) {
        s_instrumentData_t result = { 0 };
        s_instrumentData_t *pdata = &result;
        int year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0;
        sscanf(buf,"%04d-%02d-%02d %02d:%02d:%02d,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%f,%[^,],%d\r\n",
            &year,&mon,&day,&hour,&min,&sec,pdata->Sensor[0].name,&pdata->Sensor[0].fval,pdata->Sensor[1].name,&pdata->Sensor[1].fval,
            pdata->Sensor[2].name,&pdata->Sensor[2].fval,pdata->Sensor[3].name,&pdata->Sensor[3].fval,pdata->Sensor[4].name,&pdata->Sensor[4].fval,
            pdata->Sensor[5].name,&pdata->Sensor[5].fval,pdata->Sensor[6].name,&pdata->Sensor[6].fval,
            pdata->inter_temp.name,&pdata->inter_temp.fval, pdata->press.name,&pdata->press.fval,
            pdata->RH.name,&pdata->RH.fval,pdata->sensor_temp.name,&pdata->sensor_temp.fval,
            pdata->PM1.name,&pdata->PM1.fval,pdata->PM2_5.name,&pdata->PM2_5.fval,
            pdata->PM10.name,&pdata->PM10.fval,pdata->unitId,&pdata->status);
        sprintf(pdata->time, "%04d-%02d-%02d %02d:%02d:%02d", year, mon, day, hour, min, sec);

         /*sult.PM2_5.fval = PM2_5_CHANG(result.PM2_5.fval);
         result.PM10.fval = PM10_CHANG(result.PM10.fval); 
         result.Sensor[SENSOR_NO2_INDEX].fval = NO2_CHANG(result.Sensor[SENSOR_NO2_INDEX].fval);
         result.Sensor[SENSOR_O3_INDEX].fval = O3_CHANG(result.Sensor[SENSOR_O3_INDEX].fval);
         result.Sensor[SENSOR_SO2_INDEX].fval = S02_CHANG(result.Sensor[SENSOR_SO2_INDEX].fval);
         result.Sensor[SENSOR_CO_INDEX].fval = CO_CHANG(result.Sensor[SENSOR_CO_INDEX].fval);*/
        s_smf_read_flag[index] = RT_FALSE;
        rt_mq_send(s_CCDataQueue[index], &result, sizeof(result));
    }
}

rt_bool_t smf_read_data(int index, int timeout, s_instrumentData_t *result)
{
    if (!s_smf_read_flag[index]) {
        if ((RT_EOK == rt_mq_recv(s_CCDataQueue[index], result, sizeof(s_instrumentData_t), rt_tick_from_millisecond(timeout)))) {
            s_smf_data_bak[index] = *result;
            s_smf_read_flag[index] = RT_TRUE;
        }
    } else {
        *result = s_smf_data_bak[index];
    }
}

rt_bool_t smf_end_read_data(int index)
{
    s_smf_read_flag[index] = RT_FALSE;
}

//每收到一包数据，调用该函数
rt_bool_t smf_put_bytes(int index, int cli, rt_uint8_t *data, int len)
{
    rt_uint8_t *buffer = s_pCCBuffer[index][cli];
    rt_base_t pos = s_CCBufferPos[index][cli];

    for (int n = 0; n < len; n++) {
        buffer[pos++] = data[n];
        if (pos >= SDCCP_SMF_BUF_SIZE) pos = 0;
        if (pos >= 2 && buffer[pos - 2] == SMF_EOM1 && buffer[pos - 1] == SMF_EOM2) {
            __smf_parse_data(index, (char *)buffer, pos);
            pos = 0;
        }
    }
    s_CCBufferPos[index][cli] = pos;
    return RT_EOK;
}


