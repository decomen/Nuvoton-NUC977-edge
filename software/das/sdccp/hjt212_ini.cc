#define def_hjt212_default_ini ";默认配置\r\n"\
"[common]\r\n"\
"; 系统编号\r\n"\
"st=80\r\n"\
"\r\n"\
"; 访问密码\r\n"\
"pw=123456\r\n"\
"\r\n"\
"; 设备编码以以下优先级进行使用\r\n"\
"; 优先级：Nid > MN > 4G路由器设备ID\r\n"\
"mn=NJCJ20180827000000000372\r\n"\
"\r\n"\
"; 是否进行校验流程开关 0 关, 1开\r\n"\
"verify_flag=0\r\n"\
"\r\n"\
"; 实时数据上传开关 0 关, 1开\r\n"\
"real_flag=1\r\n"\
"; 实时数据间隔(毫秒)---注意:以太网最小值1000, GPRS最小值3000\r\n"\
"period=300000\r\n"\
"\r\n"\
"; 分钟数据上传开关 0 关, 1开\r\n"\
"min_flag=1\r\n"\
"\r\n"\
"; 5分钟数据上传开关 0 关, 1开\r\n"\
"min_5_flag=1\r\n"\
"\r\n"\
"; 小时数据上传开关 0 关, 1开\r\n"\
"hour_flag=0\r\n"\
"\r\n"\
"; 整点上传开关 0 关, 1开\r\n"\
"sharp_flag=1\r\n"\
"\r\n"\
";关掉一些不需要的数据\r\n"\
";是否发送累计值 1 表示不发\r\n"\
"no_cou=1\r\n"\
";是否发送最小值 1 表示不发\r\n"\
"no_min=1\r\n"\
";是否发送最大值 1 表示不发\r\n"\
"no_max=1\r\n"\
";是否发送平均值 1 表示不发\r\n"\
"no_avg=1\r\n"\
"\r\n"\
";数据状态上传开关 1 表示不发\r\n"\
"DATState_flag=0\r\n"\
"\r\n"\
";数据包里面是否包含应答flash, 不配置就是不带，配置了就用配置的值\r\n"\
"response_flag=5\r\n"\
"\r\n"\
";是否使用hjt2017\r\n"\
";no_hjt2017=1\r\n"\
";是否包含DataTimeSec\r\n"\
";DataTimeSec=1\r\n"\
";是否发送心跳\r\n"\
";enable_heart=1\r\n"
