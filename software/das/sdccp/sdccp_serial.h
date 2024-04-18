
#ifndef __SDCCP_SERIAL_H__
#define __SDCCP_SERIAL_H__

void sdccp_serial_openall(void);
void sdccp_serial_closeall(void);
void sdccp_serial_senddata(int index, const void *data, rt_size_t size);

#endif


