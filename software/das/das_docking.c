
#include "board.h"
#include <arpa/inet.h>

#define DAS_CPU_LIMIT           1

#define DM_IO_CTL       _IO('I',100)
#define DEV_IN_NAME     "/dev/dm_io"  
#define DEV_OUT_NAME    "/dev/dm_output"
#define DEV_ADC_NAME    "/dev/dm_adc" 

#define RELAY_CTL       _IO('O',100)
#define OUTPUT_CTL      _IO('O',101)
#define RELAY_GET       _IO('O',200)
#define OUTPUT_GET      _IO('O',201)
#define SPI_READ_ADC    _IO('S',100)

typedef struct {
    unsigned int channel;
    unsigned short adcValue;
}s_Adc7689_t;

typedef struct CPU_PACKED {
    char name[20];
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
    unsigned int iowait;
    unsigned int irq;
     unsigned int softirq;
}CPU_OCCUPY;

/**
 note: use das_strncpy/das_strcpy_s to copy string
 note: use das_string_equals        to equals string
 note: use das_string_startwith     to equals string start
 note: use das_strdup               to dup string
 
 note: use das_set_time             to set system time
 note: use das_get_time             to get UTC/GMT timestamp
 note: use das_sys_time             to get system running time (sec)
 note: use das_sys_msectime         to get system running time (msec)
*/

static int get_cmd_result(const char *cmd, char *result, int sz)
{
    assert(result != NULL);
    char buffer[2048] = {0};
    char *p = result;
    int ofs = 0;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    while(!feof(pipe) && (sz - ofs > 0)) {
        if(fgets(buffer, sizeof(buffer), pipe)) {
            ofs += snprintf(&p[ofs], sz - ofs, "%s", buffer);
        }
    }
    pclose(pipe);
    return 0;
}

static int cal_cpu_occupy(const CPU_OCCUPY *o, const CPU_OCCUPY *n) 
{
    unsigned long od, nd;
    int cpu_use = 0;

    od = (unsigned long)(o->user + o->nice + o->system + o->idle +
        o->iowait + o->irq + o->softirq);
    nd = (unsigned long)(n->user + n->nice + n->system + n->idle +
        o->iowait + o->irq + o->softirq);

    if ((nd - od) != 0) {
        cpu_use = (int)((unsigned long)(nd - od - (n->idle - o->idle))*100/(nd - od));
    } else {
        cpu_use = 0;
    }
    return cpu_use;
}

static void get_cpu_occupy(CPU_OCCUPY cpu_occupy[DAS_CPU_LIMIT], int cpu_cnt) 
{
    FILE *fd;
    char buff[256] = {0};
    int n = cpu_cnt;
    CPU_OCCUPY *p_cpu_occupy;

    if (n > DAS_CPU_LIMIT) {
        n = DAS_CPU_LIMIT;
    }
    fd = fopen("/proc/stat", "r");
    fgets(buff, sizeof(buff), fd);
    for (p_cpu_occupy = cpu_occupy; p_cpu_occupy < cpu_occupy + cpu_cnt;
        p_cpu_occupy++) {
        memset(buff, 0, sizeof(buff));
        fgets(buff, sizeof(buff), fd);
        sscanf(buff, "%s %u %u %u %u %u %u %u", p_cpu_occupy->name, &p_cpu_occupy->user,
            &p_cpu_occupy->nice, &p_cpu_occupy->system, &p_cpu_occupy->idle,
            &p_cpu_occupy->iowait, &p_cpu_occupy->irq, &p_cpu_occupy->softirq);
//        printf("%d %d %d %d %d\n", __LINE__, p_cpu_occupy->user,
//            p_cpu_occupy->nice, p_cpu_occupy->system, p_cpu_occupy->idle);

    }
    
    fclose(fd);    
}

static int get_addr(const char* eth, char *addr, int len, int flag)
{
    int sockfd = 0;
    struct sockaddr_in *sin;
    struct ifreq ifr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error!\n");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, (sizeof(ifr.ifr_name) - 1), "%s", eth);

    if(ioctl(sockfd, flag, &ifr) < 0 )
    {
        perror("ioctl error!\n");
        close(sockfd);
        return -1;
    }
    close(sockfd);

    if (SIOCGIFHWADDR == flag){
        unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
        memcpy((void *)addr, (const void *)&ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
        sprintf(addr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        sin = (struct sockaddr_in *)&ifr.ifr_addr;
        snprintf((char *)addr, len, "%s", inet_ntoa(sin->sin_addr));
    }

    return 0;
}

int das_do_get_system_resource(cpu_usage_t *res)
{
    CPU_OCCUPY cpu_stat1[DAS_CPU_LIMIT], cpu_stat2[DAS_CPU_LIMIT];
    // fill res
    char result[256] = {0};
    int total_mem = 0, used_mem = 0, shared_mem = 0,  buffers_mem = 0, cached_mem = 0;
    
    memset(result, 0, sizeof(result));
    get_cmd_result("free | grep Mem | awk '{print $2\":\"$3\":\"$5\":\"$6\":\"$7 }'", result, sizeof(result));
    sscanf(result, "%d:%d:%d:%d:%d", &total_mem, &used_mem, &shared_mem, &buffers_mem, &cached_mem);
    if (used_mem) {
            res->mem_all = total_mem;
            res->mem_max_use = used_mem;
            used_mem -= cached_mem;
            res->mem_now_use = used_mem;
    } else {
        return -1;
    }

    get_cpu_occupy(cpu_stat1, DAS_CPU_LIMIT);
    sleep(1);
    get_cpu_occupy(cpu_stat2, DAS_CPU_LIMIT);

    for(int i = 0; i < DAS_CPU_LIMIT; i++) {
        res->cpu = cal_cpu_occupy(&cpu_stat1[i], &cpu_stat2[i]);
    }

    int total_rom, used_rom;
    memset(result, 0, sizeof(result));
    total_rom = 0; used_rom = 0;
    res->flash_size = 0; res->flash_use = 0;
    get_cmd_result("df -k | grep \"nand\" | awk '{print $2\":\"$3 }'", result, sizeof(result));
    sscanf(result, "%d:%d", &total_rom, &used_rom);
    if (total_rom) {
        res->flash_size = total_rom;
        res->flash_use = used_rom;
    }
    
    memset(result, 0, sizeof(result));
    total_rom = 0; used_rom = 0;
    res->sd_size = 0; res->sd_use = 0;
    get_cmd_result("df -k | grep \"sdcard\" | awk '{print $2\":\"$3 }'", result, sizeof(result));
    sscanf(result, "%d:%d", &total_rom, &used_rom);
    if (total_rom) {
        res->sd_size = total_rom;
        res->sd_use = used_rom;
    }
    
    return 0;
}

const char *das_do_get_net_driver_name(const char *type, uint32_t index)
{
    if (das_string_equals(type, DAS_NET_TYPE_ETH, 1)) {
        switch(index) {
        case 0: return "eth0";
#if TEST_ON_PC
        case 1: return "eth0:0";
#else
        case 1: return "eth1";
#endif
        default: return NULL;
        }
    } else if (das_string_equals(type, DAS_NET_TYPE_GPRS, 1) ||
               das_string_equals(type, DAS_NET_TYPE_LTE, 1) ||
               das_string_equals(type, DAS_NET_TYPE_NBIOT, 1)) {
        switch(index) {
#if TEST_ON_PC
        case 0: return "eth0";
#else
        case 0: return "ppp0";
#endif
        case 1: return "ppp1";
        default: return NULL;
        }
    }
    return NULL;
}


//移远4G ttyUSB2 -> AT
//M26 2G模块 ttyS6


static int ec20_flag = 0;
const char *das_do_get_uart_driver_name(uint32_t index)
{
    switch(index) {
#if TEST_ON_PC
    case 0: return "/dev/ttyS1";
#else
    case 0: return "/dev/ttyS5";
#endif
    case 1: return "/dev/ttyS10";
    case 2: {
        if (g_xCellNetType == E_GPRS_M26) {
            return "/dev/ttyS6";
        } else if ( (g_xCellNetType == E_4G_EC20) || (g_xCellNetType == E_4G_EC200S) )  {
            if(ec20_flag == 0){
                ec20_flag = 1;
                return "/dev/ttyUSB4";
            }else {
                ec20_flag = 0;
                return "/dev/ttyUSB2";
            }
        }/*else if(g_xCellNetType == E_4G_EC200S){
            return "/dev/ttyUSB4";
        } */else {
            return "/dev/ttyS6";
        }
    }
    case 4: {
        if (BOARD_USE_LORA) {
            return "/dev/ttyS6";
        }
    }
    default: return NULL;
    }
    return NULL;
}

const char das_do_get_uart_parity_char(int parity)
{
    switch(parity) {
    case 0: return 'N';
    case 1: return 'O';
    case 2: return 'E';
    default: return 'N';
    }
}

int das_do_is_net_up(char *type, uint32_t index)
{
    char result[1024] = {0}, cmd[128] = {0};
    const char *interface_name = das_do_get_net_driver_name(type, index);
    sprintf(cmd, "ifconfig %s | grep RUNNING", interface_name);
    get_cmd_result(cmd, result, sizeof(result));
    if (strlen(result)) {
        return 1;
    } else {
        return 0;
    }
    return 0;
}

int das_do_is_enet_up(void)
{
    return das_do_is_net_up(DAS_NET_TYPE_ETH, 0);
}

int das_do_is_gprs_up(void)
{
    return das_do_is_net_up(DAS_NET_TYPE_GPRS, 0);
}

int das_do_check_dns(const char *dns)
{
    char cmd[128] = {0};
    char result[256] = {0};
    sprintf(cmd, "grep '%s' /etc/resolv.conf", dns);
    get_cmd_result(cmd, result, sizeof(result));
    return result[0] != '\0';
}

void das_do_del_default_routes(void)
{
    system("route del default");
    system("route del default");
    system("route del default");
}

void das_do_del_route(const char *dev)
{
    int n = 10;
    char cmd[128] = {0};
    char result[128] = {0};

    while (n--) {
        sprintf(cmd, "iproute | grep 'dev %s' | head -1", dev);
        get_cmd_result(cmd, result, sizeof(result));
        if (result[0] != '\0') {
            sprintf(cmd, "iproute del %s", result);
        } else {
            break;
        }
    }
}

int das_do_set_net_info(const struct das_net_list_node *node)
{
    // set eth
    struct das_net_list_node eth_info;
    char cmd[128] = {0};
    const char *interface_name = das_do_get_net_driver_name((const char *)node->TYPE, node->INDEX);
    memset(&eth_info, 0, sizeof(eth_info));
    if (interface_name) {
        das_do_get_net_info(node->TYPE, node->INDEX, &eth_info);
        das_do_del_route(interface_name);
        if (eth_info.DHCP == 0 && node->DHCP) {
            system("killall udhcpc");
            system("udhcpc &");
        } else if (node->DHCP == 0) {
            system("killall udhcpc");
            if (!das_string_equals(node->IP, eth_info.IP, 1) || !das_string_equals(node->MASK, eth_info.MASK, 1)) {
                sprintf(cmd, "ifconfig %s %s netmask %s", interface_name, node->IP, node->MASK);
                system(cmd);
                printf("cmd: %s\r\n",cmd);
            }

            int dns_flag = 0;
            struct sockaddr_in peer;
            system(
                "rm -f /tmp/resolv.prev;"
                "cp /etc/resolv.conf /tmp/resolv.prev;"
                "grep domain /tmp/resolv.prev > /etc/resolv.conf;"
                "grep search /tmp/resolv.prev >> /etc/resolv.conf"
            );
            if (node->DNS1[0] && inet_aton(node->DNS1, &peer.sin_addr) && !das_do_check_dns(node->DNS1)) {
                dns_flag = 1;
                sprintf(cmd, "echo 'nameserver %s # %s' >> /etc/resolv.conf", node->DNS1, interface_name);
            }
            if (node->DNS1[0] && node->DNS2[0] && inet_aton(node->DNS2, &peer.sin_addr) && !das_do_check_dns(node->DNS2)) {
                dns_flag = 1;
                sprintf(cmd, "echo 'nameserver %s # %s' >> /etc/resolv.conf", node->DNS2, interface_name);
            }

            // 没有设置DNS, 默认设置 8.8.8.8
            if (!dns_flag) {
                sprintf(cmd, "echo 'nameserver %s # %s' >> /etc/resolv.conf", "8.8.8.8", interface_name);
            }
            system(cmd);
            system("grep ppp /tmp/resolv.prev >> /etc/resolv.conf");
        }
        if(!das_do_is_gprs_up()){
            //net_do_add_route(interface_name);
            if(!g_net_cfg.dhcp){
                net_do_add_eth0_route();
            }
        }else {
           //net_do_add_route(interface_name); 
           net_do_add_ppp0_route();
        }
    }
    
    return 0;
}

int das_do_get_net_info(const char *type, uint32_t index, struct das_net_list_node *net)
{
    // fill net
    char result[1024] = {0}, cmd[128] = {0};
    const char* ptr = NULL;
    if (das_string_equals(type, DAS_NET_TYPE_ETH, 1) || das_string_equals(type, DAS_NET_TYPE_GPRS, 1)) {
        const char *interface_name = das_do_get_net_driver_name(type, index);
        if (interface_name) {
            sprintf(cmd, "ifconfig | grep %s", interface_name);
            get_cmd_result(cmd, result, sizeof(result));
            if (strlen(result)) {
                if(das_check_process("udhcpc")) {
                    net->DHCP = 1;
                } else {
                    net->DHCP = 0;
                }
                if (net->DHCP) {
                    memset(cmd, 0, sizeof(cmd));
                    memset(result, 0, sizeof(result));
                    sprintf(cmd, "iproute | grep 'dev %s'", interface_name);
                    get_cmd_result(cmd, result, sizeof(result));
                    if (strlen(result)) {
                        if ((ptr = strstr(result, "default via"))) {
                            ptr += strlen("default via ");
                            char* ptr_getway = net->GATEWAY;
                            while(*ptr && *ptr != ' ') {
                                *(ptr_getway++) = *(ptr++);
                            }
                        }
                    }
                }

                get_addr(interface_name, net->IP, sizeof(net->IP), SIOCGIFADDR);
                get_addr(interface_name, net->MASK, sizeof(net->MASK), SIOCGIFNETMASK);
                get_addr(interface_name, net->MAC, sizeof(net->MAC), SIOCGIFHWADDR);
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "grep '%s' /etc/resolv.conf", interface_name);
                memset(result, 0, sizeof(result));
                get_cmd_result(cmd, result, sizeof(result));
                if (strlen(result)) {
                    if ((ptr = strstr(result, "nameserver")) > 0) {
                        ptr += strlen("nameserver ");
                        char* ptr_net_dns = net->DNS1;
                        while(*ptr && *ptr != '\n' && *ptr != ' ') {
                            *(ptr_net_dns++) = *(ptr++);
                        }
                        if((ptr = strstr(ptr, "nameserver")) > 0) {
                            ptr += strlen("nameserver ");
                            ptr_net_dns = net->DNS2;
                            while (*ptr && *ptr != '\n' && *ptr != ' ') {
                                *(ptr_net_dns++) = *(ptr++);
                            }
                        }
                        struct sockaddr_in peer;
                        if (!inet_aton(net->DNS1, &peer.sin_addr)) net->DNS1[0] = '\0';
                        if (!inet_aton(net->DNS2, &peer.sin_addr)) net->DNS2[0] = '\0';
                    }
                }

                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ifconfig %s | grep RUNNING", interface_name);
                memset(result, 0, sizeof(result));
                get_cmd_result(cmd, result, sizeof(result));
                if (strlen(result)) {
                    das_strncpy(net->STATUS, DAS_NET_STATUS_UP, sizeof(DAS_NET_STATUS_UP));
                } else {
                    das_strncpy(net->STATUS, DAS_NET_STATUS_DOWN, sizeof(DAS_NET_STATUS_DOWN));
                }
            } else {        
                return -1;
            }
        }
    }
    else if (das_string_equals(type, DAS_NET_TYPE_LTE, 1)) {
        ;
    } else if (das_string_equals(type, DAS_NET_TYPE_GPRS, 1)) {
        ;
    } else if (das_string_equals(type, DAS_NET_TYPE_NBIOT, 1)) {
        ;
    }
    das_strncpy(net->TYPE, type, sizeof(net->TYPE));
    net->INDEX = index;
    
    return 0;
}

int das_do_io_ctrl(int ctrl, s_io_t *iodata)
{
    static int fd = -1;
    if (fd < 0) fd = open(DEV_IN_NAME, O_RDWR);
    if (-1 == fd) return -1;

    if(ioctl(fd, ctrl, iodata) < 0) {
        close(fd);
        fd = -1;
        return -1;
    }
    
    return 0;
}

// all ttl
int das_do_get_di_state(int index)
{
    s_io_t iodata = {.gpio = index, .dir = 0, .val = 0};
    das_do_io_ctrl(DM_IO_CTL, &iodata);
    return iodata.val;
}

int das_do_do_ctrl(int ctrl, s_Output_t *iodata)
{
    static int fd = -1;
    if (fd < 0) fd = open(DEV_OUT_NAME, O_RDWR);
    if (-1 == fd) return -1;

    if(ioctl(fd, ctrl, iodata) < 0) {
        close(fd);
        fd = -1;
        return -1;
    }
    return 0;
}

int das_do_get_do_state(int type, int index)
{    
    s_Output_t output_data = {0, 0};
    output_data.gpio = index;
    switch (type) {
    case DAS_DIDO_TYPE_RELAY:
        das_do_do_ctrl(RELAY_GET, &output_data);
        break;
    case DAS_DIDO_TYPE_TTL:
        das_do_do_ctrl(OUTPUT_GET, &output_data);
        break;
    default: 
        break;
    }
    return output_data.val;
}

int das_do_set_do_state(int type, int index, int state)
{    
    s_Output_t output_data = {0, 0};
    output_data.gpio = index;
    switch (type) {
    case DAS_DIDO_TYPE_RELAY:
        switch (state) {
        case DAS_DIDO_RELAY_OFF:
            output_data.val = 0;
            break;
        case DAS_DIDO_RELAY_ON:
            output_data.val = 1;
            break;
        default: 
            break;
        }
        das_do_do_ctrl(RELAY_CTL, &output_data);
        break;
    case DAS_DIDO_TYPE_TTL:
        switch (state) {
        case DAS_DIDO_TTL_LOW:
            output_data.val = 0;
            break;
        case DAS_DIDO_TTL_HIGHT:
            output_data.val = 1;
            break;
        default: 
            break;
        }
        das_do_do_ctrl(OUTPUT_CTL, &output_data);
        break;
    default: 
        break;
    }
    return 0;
}

int das_do_get_ai_value(int channel, float *value)
{
    static int fd = -1;
    s_Adc7689_t adc = {0, 0};
    *value = 0;
    if (fd < 0) fd = open(DEV_ADC_NAME, O_RDWR);
    if (-1 == fd) return -1;
    adc.channel = channel;
    if (ioctl(fd, SPI_READ_ADC, &adc) < 0) {
        close(fd);
        fd = -1;
        return -1;
    }
    *value = adc.adcValue;
    return 0;
}

int das_do_open_hw_com(int index)
{
    // open com
    int fd = -1;
    const char *dev_name = das_do_get_uart_driver_name(index);
    if (dev_name && dev_name[0]) {
        fd = open(dev_name, O_RDWR|O_NOCTTY|O_NDELAY);
    }
    rt_kprintf("open %s success\r\n", dev_name);
    return fd;
}

int das_do_set_hw_com_info(int fd, int baud_rate, int data_bits, int stop_bits, int parity)
{
    // set com
    if (fd >= 0) {
        struct termios options;
        if  (tcgetattr( fd,&options)  !=  0)
        {
            return -1;
        }
        /* configure new values */
        cfmakeraw(&options); /*see man page */
        options.c_iflag |=IGNPAR; /*ignore parity on input */
        options.c_oflag &= ~(OPOST | ONLCR | OLCUC | OCRNL | ONOCR | ONLRET | OFILL);

        options.c_cflag  |=  CLOCAL | CREAD; //CLOCAL:忽略modem控制线  CREAD：打开接受者
        //options.c_cflag &= ~CSIZE; //字符长度掩码。取值为：CS5，CS6，CS7或CS8

        switch(data_bits)
        {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        }
        // 校验位 0: NONE 1: odd 2: even 校验位（下拉框） 
        switch(parity)
        {
        case 1:
            //options.c_cflag |= PARENB; //允许输出产生奇偶信息以及输入到奇偶校验
            //options.c_cflag |= PARODD;  //输入和输出是奇及校验
            //options.c_iflag |= (INPCK | ISTRIP); // INPACK:启用输入奇偶检测；ISTRIP：去掉第八位
             options.c_cflag |= PARENB;
             options.c_cflag |= PARODD;
             options.c_cflag &= ~CSIZE;
             options.c_cflag |= CS8;

            break;
        case 2:
            //options.c_iflag |= (INPCK | ISTRIP);
            //options.c_cflag |= PARENB;
            //options.c_cflag &= ~PARODD;
             options.c_cflag |= PARENB;
             options.c_cflag &= ~PARODD;
             options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS8;
            break;
        case 0:
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS8;
            break;
        }

        switch(baud_rate)
        {
		case 1200:
			cfsetispeed(&options, B1200);
			cfsetospeed(&options, B1200);
			break;
		case 2400:
			cfsetispeed(&options, B2400);
			cfsetospeed(&options, B2400);
			break;
		case 4800:
			cfsetispeed(&options, B4800);
			cfsetospeed(&options, B4800);
			break;
		case 9600:
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			break;
		case 19200:
            cfsetispeed(&options,B19200);
            cfsetospeed(&options,B19200);
            break;
        case 38400:
            cfsetispeed(&options,B38400);
            cfsetospeed(&options,B38400);
            break;
        case 57600:
            cfsetispeed(&options,B57600);
            cfsetospeed(&options,B57600);
            break;
		case 115200:
			cfsetispeed(&options, B115200);
			cfsetospeed(&options, B115200);
			break;
		case 460800:
			cfsetispeed(&options, B460800);
			cfsetospeed(&options, B460800);
			break;
		default:
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			break;
        }

        if(stop_bits == 1 )
            options.c_cflag &=  ~CSTOPB; //CSTOPB:设置两个停止位，而不是一个
        else if (stop_bits == 2 )
            options.c_cflag |=  CSTOPB;

        options.c_cc[VTIME]  = 0; //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
        options.c_cc[VMIN] = 0; //VMIN:非canonical模式读到最小字符数
        tcflush(fd,TCIFLUSH); // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
        if((tcsetattr(fd,TCSANOW,&options))!=0) //TCSANOW:改变立即发生
        {
            perror("com set error");
            return -1;
        }
    }
    return 0;
}

int das_do_close_hw_com(int fd)
{
    // close com
    if (fd >= 0) {
        close(fd);
    }
    return 0;
}


#if 0
int das_do_get_hw_eth_info(uint32_t index, struct das_hw_eth_list_node *node)
{
    // fill node
    int ret;
    struct das_net_list_node net_info;
    memset(&net_info, 0, sizeof(net_info));

    ret = das_do_get_net_info(DAS_NET_TYPE_ETH, index, &net_info);
    
    if (0 == ret) {
        node->DHCP = net_info.DHCP;
        das_strncpy(node->DNS1, net_info.DNS1, strlen(net_info.DNS1) + 1);
        das_strncpy(node->DNS2, net_info.DNS2, strlen(net_info.DNS2) + 1);
        das_strncpy(node->GATEWAY, net_info.GATEWAY, strlen(net_info.GATEWAY) + 1);
        das_strncpy(node->IP, net_info.IP, strlen(net_info.IP) + 1);
        das_strncpy(node->MAC, net_info.MAC, strlen(net_info.MAC) + 1);
        das_strncpy(node->MASK, net_info.MASK, strlen(net_info.MASK) + 1);
        das_strncpy(node->NAME, net_info.NAME, strlen(net_info.NAME) + 1);
        das_strncpy(node->STATUS, net_info.STATUS, strlen(net_info.STATUS) + 1);
    }

    return ret;
}

int das_do_get_hw_lte_info(uint32_t index, struct das_hw_lte_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_get_hw_gprs_info(uint32_t index, struct das_hw_gprs_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_get_hw_nbiot_info(uint32_t index, struct das_hw_nbiot_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_get_hw_zigbee_info(uint32_t index, struct das_hw_zigbee_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_get_hw_com_info(uint32_t index, struct das_hw_com_list_node *node)
{
    // fill node
    // auto identification DAS_COM_TYPE_485, DAS_COM_TYPE_232
    
    return 0;
}

int das_do_get_hw_di_info(uint32_t type, uint32_t index, struct das_hw_di_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_get_hw_do_info(uint32_t type, uint32_t index, struct das_hw_do_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_get_hw_ai_info(uint32_t index, struct das_hw_ai_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_get_hw_lora_info(uint32_t index, struct das_hw_lora_list_node *node)
{
    // fill node
    
    return 0;
}

int das_do_set_hw_lte_info(const struct das_hw_lte_list_node *node)
{
    // set lte
    
    return 0;
}

int das_do_set_hw_gprs_info(const struct das_hw_gprs_list_node *node)
{
    // set gprs
    
    return 0;
}

int das_do_set_hw_nbiot_info(const struct das_hw_nbiot_list_node *node)
{
    // set nbiot
    
    return 0;
}

int das_do_open_hw_zigbee(const struct das_hw_zigbee_list_node *node)
{
    int fd = -1;
    // open zigbee
    
    return fd;
}

int das_do_set_hw_zigbee_info(int fd, const struct das_hw_zigbee_list_node *node)
{
    // set zigbee
    if (fd >= 0) {
        
    }
    return 0;
}

int das_do_close_hw_zigbee(int fd)
{
    // close zigbee
    if (fd >= 0) {
        
    }
    return 0;
}

int das_do_open_hw_com(const struct das_hw_com_list_node *node)
{
    // open com
    int fd = -1;

    if (strlen(node->NAME)) {
        char com_full_path[20] = {0};
        sprintf(com_full_path, "/dev/%s", node->NAME);
        fd = open(com_full_path, O_RDWR|O_NOCTTY|O_NDELAY);
    }
    
    return fd;
}

int das_do_set_hw_com_info(int fd, const struct das_hw_com_list_node *node)
{
    // set com
    if (fd >= 0) {
        struct termios options;
        if  (tcgetattr( fd,&options)  !=  0)
        {
            return -1;
        }
        memset( &options, 0, sizeof( options ) );
        options.c_cflag  |=  CLOCAL | CREAD; //CLOCAL:忽略modem控制线  CREAD：打开接受者
        options.c_cflag &= ~CSIZE; //字符长度掩码。取值为：CS5，CS6，CS7或CS8

        switch( node->DATA_BIT )
        {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        }

        switch( node->PARITY )
        {
        case 2:
            options.c_cflag |= PARENB; //允许输出产生奇偶信息以及输入到奇偶校验
            options.c_cflag |= PARODD;  //输入和输出是奇及校验
            options.c_iflag |= (INPCK | ISTRIP); // INPACK:启用输入奇偶检测；ISTRIP：去掉第八位
            break;
        case 1:
            options.c_iflag |= (INPCK | ISTRIP);
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            break;
        case 0:
            options.c_cflag &= ~PARENB;
            break;
        }

        switch( node->BAUD )
        {
        case 2400:
            cfsetispeed(&options, B2400);
            cfsetospeed(&options, B2400);
            break;
        case 4800:
            cfsetispeed(&options, B4800);
            cfsetospeed(&options, B4800);
            break;
        case 9600:
            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);
            break;
        case 19200:
            cfsetispeed(&options,B19200);
            cfsetospeed(&options,B19200);
            break;
        case 38400:
            cfsetispeed(&options,B38400);
            cfsetospeed(&options,B38400);
            break;
        case 115200:
            cfsetispeed(&options, B115200);
            cfsetospeed(&options, B115200);
            break;
        case 460800:
            cfsetispeed(&options, B460800);
            cfsetospeed(&options, B460800);
            break;
        default:
            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);
            break;
        }

        if( node->STOP_BIT == 1 )
            options.c_cflag &=  ~CSTOPB; //CSTOPB:设置两个停止位，而不是一个
        else if ( node->STOP_BIT == 2 )
            options.c_cflag |=  CSTOPB;

        options.c_cc[VTIME]  = 0; //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
        options.c_cc[VMIN] = 0; //VMIN:非canonical模式读到最小字符数
        tcflush(fd,TCIFLUSH); // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
        if((tcsetattr(fd,TCSANOW,&options))!=0) //TCSANOW:改变立即发生
        {
            perror("com set error");
            return -1;
        }
    }
    return 0;
}

int das_do_close_hw_com(int fd)
{
    // close com
    if (fd >= 0) {
        close(fd);
    }
    return 0;
}

int das_do_set_hw_di_info(const struct das_hw_di_list_node *node)
{
    // set di
    return 0;
}

#endif

struct ethtool_value {
    uint32_t cmd;
    uint32_t data;
};

int das_do_check_enet_link(void)
{
    struct ethtool_value edata;
    int fd = -1;
    struct ifreq ifr;
    int result = -1;

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0));
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return -2;
    }
    edata.cmd = 0x0000000A;
    ifr.ifr_data = (void *)&edata;
    if (0 == ioctl(fd, 0x8946, &ifr)) {
        result = (edata.data ? 1 : 0);
    } else if (errno == EOPNOTSUPP) {
        result = -3;
    }
    close(fd);
    return result;
}


