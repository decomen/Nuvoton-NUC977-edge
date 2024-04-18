#define def_mqtt_default_ini ";默认配置\r\n"\
"[common]\r\n"\
"; 用户名\r\n"\
"username=\r\n"\
"\r\n"\
"; 访问密码\r\n"\
"password=\r\n"\
"\r\n"\
"; client_id以以下优先级进行使用\r\n"\
"; 优先级：client_id > 4G路由器设备SN\r\n"\
"client_id=0123456789\r\n"\
"\r\n"\
"; 订阅的主题名称(可用于做一些控制)\r\n"\
"topic_sub=dm_a401f/test_sub\r\n"\
"\r\n"\
"; 发布数据的主题名称\r\n"\
"topic_pub=dm_a401f/test_pub\r\n"\
"\r\n"\
"; MQTT keepalive 间隔(秒)\r\n"\
"keepalive=60\r\n"\
"\r\n"\
"; MQTT QOS \r\n"\
"qos=0\r\n"\
"\r\n"\
"; MQTT retained\r\n"\
"retained=0\r\n"\
"\r\n"\
"; 实时数据上传开关 0 关, 1开\r\n"\
"real_flag=1\r\n"\
"; 实时数据间隔(毫秒)\r\n"\
"real_interval=30000\r\n"\
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
"no_min=0\r\n"\
";是否发送最大值 1 表示不发\r\n"\
"no_max=0\r\n"\
";是否发送平均值 1 表示不发\r\n"\
"no_avg=0\r\n"\
"\r\n"

