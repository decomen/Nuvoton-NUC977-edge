#ifndef __SDCCP_TYPES_H__
#define __SDCCP_TYPES_H__

#define SDCCP_PACK_PRECODE_LEN (2)
#define SDCCP_VER (0x00)

#define SDCCP_OK	(0)
#define SDCCP_ERR	(-1)
#define SDCCP_TIMEOUT	(-2)
#define SDCCP_RETRY	(-3)

#define PRECODE_0 (0x12)
#define PRECODE_1 (0xEF)


#define STATUS_OK     (1)
#define STATUS_FAILED (0)



#define SDCCP_CHECK_TIME_OUT(_begin, _t) ((rt_tick_get() - (_begin))  > (_t))	//判断超时

//注意: CREAT_HEADER 已经做了大小端处理, 不能再重复
#define CREAT_HEADER(h_, len_, type_, q_) do { \
		h_.PreCode[0] = PRECODE_0; \
		h_.PreCode[1] = PRECODE_1; \
		h_.Length = htons((len_)); \
		h_.Version = SDCCP_VER; \
		h_.Type = type_; \
		h_.Senquence = q_; \
		h_.CRC8 = btMDCrc8(0, &h_, sizeof(S_PACKAGE_HEAD) - 1);\
	} while (0)

typedef enum {
	FA_ACK		= 0x00,
	FA_POST		= 0x01,
	FA_REQUEST 	= 0x02,
	FA_RESPONSE 	= 0x03,
} S_PACKAGE_FRAME_TYPE_E;

typedef enum {
	MSG_LOGIN = 0x00,		//登录
	MSG_LOGOUT,			//注销
	MSG_TIME_SYNC ,			//时间同步
	MSG_SERVER_VERSION,		//服务器版本获取
	MSG_DEVICE_HEATBEAT,		//心跳包数据
	MSG_DEVICE_STATUS,		//状态数据
	MSG_NOTICE,			//通知数据
	MSG_REALTIME_DATA_UP,		//发送实时数据
	MSG_REALTIME_DATA_DOWN,		//获取实时数据
	MSG_HISTORY_DATA_QUERY,		//历史数据查询
	MSG_HISTORY_DATA_UP,		//上传历史数据
	MSG_HISTORY_DATA_DOWN,		//下载历史数据
	MSG_STATISTICS_DATA_UP,		//上传统计数据
	MSG_STATISTICS_DATA_DOWN,	//下载统计数据
	MSG_USER_CONFIG_UP,		//上传用户配置数据
	MSG_USER_CONFIG_DOWN,		//下载用户配置数据
	MSG_DEVICE_CONFIG_UP,		//发送控制盒配置数据
	MSG_DEVICE_CONFIG_DOWN,		//获取控制盒配置数据
	MSG_DEVICE_LOG_UP,		//发送Log数据
	MSG_DEVICE_LOG_DOWN,		//下载Log数据
	MSG_DEVICE_VERSION,		//控制盒版本获取
	MSG_UPDATE_UP,			//发送升级数据
	MSG_UPDATE_DOWN,		//获取升级数据
	MSG_ADVICE_UP,			//推送建议数据
	MSG_ADVICE_DOWN,		//拉取建议数据

	MSG_DEVICE_SET_SAMPLING,	//设置采样状态
	MSG_DEVICE_GET_SAMPLING,	//获取采样状态

	MSG_DEVICE_GET_PRODUCT_INFO,    //获取产品信息
	
    MSG_DEVICE_GET_POWER_STATUS,    //获取电池状态
    MSG_REAL_RAW_DATA_UP,           //发送原始数据
} S_PACKAGE_MSG_TYPE_E;

typedef enum {
	SAMPLING_NONE	= 0x00,	//未采集
	SAMPLING_ING	= 0x01,	//正在采集
} S_SAMPLING_STATUS_E;

typedef enum {
	DEVICE_CONFIG_THRESHOLD_GAIN    = 0x00,     // 增益值
	DEVICE_CONFIG_THRESHOLD_BODYMOVE_T,         // 体动判断时间阈值
	DEVICE_CONFIG_THRESHOLD_TURNOVER_T,         // 翻身判断时间阈值

    DEVICE_CONFIG_REAL_STEP         = 0xF0,
    DEVICE_CONFIG_INVALID = 0xFF
} S_DEVICE_CONFIG_TYPE_E;

// 监控类数据
typedef enum {	//描述类型表

	// 心率呼吸相关
	MONITOR_DESC_BR	= 0x00,		//呼吸频率 1B 次/分
	MONITOR_DESC_HR,		//心跳频率 1B 次/分
	MONITOR_DESC_BR_HR_STATUS,	//状态 1B
	MONITOR_DESC_STATUS_VALUE, 	//与状态相关的计数值, 实时数据 2B, 历史数据 1B
	MONITOR_DESC_QUALITY,		//睡眠质量

	// 环境相关
	MONITOR_DESC_ENV_TEMPERATURE = 0x10,	//环境温度 1B
	MONITOR_DESC_ENV_HUMIDITY,		//环境温度 1B
	MONITOR_DESC_ENV_LIGHT,			//环境光强 2B
	MONITOR_DESC_ENV_CO2,			//环境CO2浓度 2B

	// 床垫相关
	MONITOR_DESC_PAD_TEMPERATURE = 0x20,	//床垫温度
	MONITOR_DESC_PAD_HUMIDITY,		//床垫湿度

    
	MONITOR_DESC_RAW = 0x30,        //原始信号
	MONITOR_DESC_BREATH_RAW, 
	MONITOR_DESC_HEART_RAW, 
} S_MONITOR_DESC_TYPE_E;

// SLEEP_INIT 只用于实时数据
typedef enum {
	SLEEP_OK = 0x00,	//一切正常
	SLEEP_INIT,		//初始化状态，约10秒时间
	SLEEP_B_STOP,		//呼吸暂停
	SLEEP_H_STOP,		//心跳暂停
	SLEEP_BODYMOVE,		//体动
	SLEEP_LEAVE,		//离床
	SLEEP_TURN_OVER,	//翻身
} S_SLEEP_STATUS_E;

typedef enum {
	DEV_NORMAL = 0x00,	//一切正常
	DEV_LOW_POWER,		//电量过低
} S_DEVICE_STATUS_TYPE_E;

typedef enum {
	NOTICE_GEBIN_REAL = 0x00,	//开始查看实时数据
	NOTICE_END_REAL,            //结束查看实时数据
	NOTICE_GEBIN_REAL_RAW,      //开始查看原始数据
	NOTICE_END_REAL_RAW,        //结束查看原始数据

	NOTICE_CLEAR_STATUS = 0x10,	//清除状态通知, Value存储状态类型 如:清除低电量状态
} S_NOTICE_TYPE_E;

typedef enum {
	LOGIN_PASSWORD = 0x00,
	LOGIN_EMAIL,
	LOGIN_MEMBER_ID,	//成员ID
} S_LONIN_TYPE_E;

typedef enum {
	H_QUERY_SUMMARY = 0x00,	//查询概要信息
	H_QUERY_RANGE,		//查询数据边界
} S_HISTORY_QUERY_TYPE_E;

typedef enum {
    H_END_NORMAL = 0x00,    //正常结束(用户手动点结束)
    H_END_AUTO,             //自动结束(用户忘记点击结束)
    H_END_ABORT,            //强制终止(关机)
    H_END_ERROR,            //错误的结束(没电了? 系统异常?)
} S_HISTORY_STOP_MODE_TYPE_E;

typedef enum {
	U_UP_SUMMARY = 0x00,	//上传升级包概要
	U_UP_DETAIL,		//上传升级包内容
} S_UPDATE_UP_TYPE_E;

typedef enum {
	PACKAGE_ACK_OK = 0x00,		//成功
	PACKAGE_ACK_INVALID_TYPE,	//帧类型错误
	PACKAGE_ACK_INVALID_LENGTH,	//帧长度异常
	PACKAGE_ACK_INVALID_CHECK_SUM,	//帧校验失败

	PACKAGE_ACK_UNKOWN = 0xFF,	//未知错误
} S_ACK_TYPE_E;

typedef enum {
	MSG_ERROR_OK = 0x00,		//OK
	MSG_ERROR_INVALID_TYPE,		//错误的消息类型
	MSG_ERROR_PARAMETER,		//参数错误(如:参数不匹配，字串异常, 结构描述中的描述类型不存在，都用该错误表示)
	MSG_ERROR_DATABASE,		//数据库错误
	MSG_ERROR_USERNAME,		//用户名错误
	MSG_ERROR_PASSWORD,		//密码错误
	MSG_ERROR_PRIVILEGE,		//无权限
	MSG_ERROR_INACTIVE,		//未激活
	MSG_ERROR_DEVICEID,		//设备ID错误
	MSG_ERROR_UNBOUND,		//未绑定
	MSG_ERROR_NOT_LOGIN,		//未登录
	MSG_ERROR_INVALID_DATA,		//历史数据错误
	MSG_ERROR_UPDATE_HW_VER,	//升级包硬件不匹配
	MSG_ERROR_UPDATE_CHECK,		//升级包校验错误
	MSG_ERROR_INVALID_SUMMARY,	//没有对应的概要信息
	MSG_ERROR_INVALID_PRODUCT_INFO,	//没有正确的出厂信息

	MSG_ERROR_UNKOWN = 0x7F,	//未知错误
} S_MSG_ERR_TYPE_E;

#pragma pack(1)

typedef struct _S_FIELD_DESC_ {	//字段描述结构
	byte Type;	//字段类型
	byte Len;	//字段所占字节数
} S_FIELD_DESC;

typedef struct _S_STRUCTURE_DESC_ {	//结构体描述
	byte Count;		//字段总数
	S_FIELD_DESC *p_Desc;	//字段描述集
} S_STRUCTURE_DESC;

typedef struct _S_PACKAGE_HEAD {		//数据帧头
	byte PreCode[SDCCP_PACK_PRECODE_LEN];	//两个字节的前导码
	ushort Length;				//数据帧长度
	byte Version;				//数据帧版本
	byte Type;				//帧类型
	byte Senquence;				//帧序号
	byte CRC8;				//帧头校验
} S_PACKAGE_HEAD;

typedef struct _S_MSG {			//网络消息
	byte Type;			//消息类型
	ushort Senquence;		//序号
	pbyte Content;			//消息正文
} S_MSG;

typedef struct _S_PACKAGE {	//数据帧结构
	S_PACKAGE_HEAD Head;	//数据帧头
	S_MSG Msg;		//消息
	u32 CRC32;		//CRC32校验码
} S_PACKAGE;

/************/
//ACK相关
typedef struct _S_PACKAGE_ACK {	// 通用 ACK 结构
	S_PACKAGE_HEAD Head;	//数据帧头
	byte AckValue;		//反馈结果
	u32 CRC32;		//CRC32校验码
} S_PACKAGE_ACK;

typedef struct _S_MSG_ERR_RESPONSE {
	byte RetCode;
} S_MSG_ERR_RESPONSE;

/************/
//控制盒状态报送相关
typedef struct _S_DEVICE_STATUS_ITEM {
	byte Type;
	byte Value;
} S_DEVICE_STATUS_ITEM;

typedef struct _S_MSG_DEVICE_STATUS_REPORT {
	byte Count;			//状态条目数
	S_DEVICE_STATUS_ITEM *Items;	//状态序列
} S_MSG_DEVICE_STATUS_REPORT;


/************/
//通知相关
typedef struct _S_NOTICE_ITEM {
	byte Type;
	byte Value;
} S_NOTICE_ITEM;

typedef struct _S_MSG_NOTICE_REPORT {	//查看事件通知结构
	byte Count;		//通知条目数
	S_NOTICE_ITEM *Items;	//通知序列
} S_MSG_NOTICE_REPORT;


/************/
//实时数据相关
/***************Lite版实时数据类型*****************/
typedef struct _S_REAL_BR_HR {	//心跳呼吸数据
	byte BreathRate;	//呼吸率
	byte HeartRate;		//心率
	byte Status;		//状态
	ushort Value;		//附加值
} S_REAL_BR_HR;
/**************************************************/

typedef struct _S_MSG_REAL_REPORT {
	u32 Time;		//实时数据对应的时间戳
	ushort Offset;  //间隔 s
	i16 Count;		//数目
	S_STRUCTURE_DESC Desc;	//结构描述, 数目 <= 0 时为空
	pbyte pRealData;	//实时数据内容序列, 数目 <= 0 时为空
} S_MSG_REAL_REPORT;

typedef struct {
	mdUINT32 ulIndex;   //当前数据起点  每次起点会自动累加 usCount
	mdUSHORT usOffset;  //间隔 ms
	mdUSHORT usCount;   //数目
	mdUSHORT *pRaw;     //实时数据内容序列, 数目 <= 0 时为空
} S_MsgRealRawReport_t;

/************/
//登录相关
typedef struct _S_MSG_LOGIN_REQUEST {
	byte DeviceID[13];	//设备ID
	u32 Timestamp;		//标准时间戳
	S_STRUCTURE_DESC Desc;	//结构描述
	pbyte pData;		//登录数据
} S_MSG_LOGIN_REQUEST;

typedef struct _S_MSG_LOGIN_RESPONSE {
	byte RetCode;		//反馈码
	u32 Timestamp;		//标准时间戳
} S_MSG_LOGIN_RESPONSE;


/************/
//注销相关
typedef struct _S_MSG_LOGOUT_REQUEST {
	u32 Timestamp;		//标准时间戳
} S_MSG_LOGOUT_REQUEST;

//与登录回复一致
typedef struct _S_MSG_LOGIN_RESPONSE S_MSG_LOGOUT_RESPONSE;

/************/
//历史数据查询相关
typedef struct _S_HISTORY_QUERY_SUMMARY {
	u32 StartTime;		//起始时间戳
	u32 EndTime;		//截止时间戳
} S_HISTORY_QUERY_SUMMARY;

typedef struct _S_HISTORY_QUERY_SUMMARY S_HISTORY_QUERY_RANGE;

typedef struct _S_HISTORY_SUMMARY {
	u32 StartTime;		//记录起点
	ushort TimeStep;	//记录间隔(秒为单位)
	i32 RecordCount;	//记录点数, 0 无数据, < 0 错误码的负值。
	byte StopMode;		//状态(参考历史数据相关状态表)
} S_HISTORY_SUMMARY;

typedef struct _S_HISTORY_RANGE {
	i32 Count;		//条目数, 0 无数据  < 0 错误码负值
	u32 FirstTime;		//头时间点, 标准时间戳
	u32 LastTime;		//尾时间点, 标准时间戳
} S_HISTORY_RANGE;

typedef struct _S_MSG_HISTORY_QUERY_REQUEST {
	byte Type;		//类型
	pbyte pQueryMsg;	//查询内容，类型不同查询结构不同
} S_MSG_HISTORY_QUERY_REQUEST;

typedef struct _S_MSG_HISTORY_QUERY_RESPONSE {
	byte RetCode;		//反馈码
	byte Type;		//类型
	i16 Count;		//序列数, = 0 表示无数据 < 0 表示错误码的负值
	pbyte pResponseMsg;	//数据组序列, 类型不同，序列元素类型则不同
} S_MSG_HISTORY_QUERY_RESPONSE;


/************/
//历史数据下载相关

/***************Lite版历史数据类型*****************/
typedef struct _S_HISTORY_BR_HR {	//心跳呼吸数据
	byte BreathRate;	//呼吸率
	byte HeartRate;		//心率
	byte Status;		//状态
	byte Value;		//附加值
} S_HISTORY_BR_HR;
/***************Lite版历史数据类型*****************/

typedef struct _S_MSG_HISTORY_DOWN_REQUEST {
	u32 StartTime;		//起始时间
	u32 StartIndex;		//起始位置
	ushort Count;		//下载条目数, 返回数目将会 <= Count
	S_STRUCTURE_DESC Desc;	//结构描述
} S_MSG_HISTORY_DOWN_REQUEST;

typedef struct _S_MSG_HISTORY_DOWN_RESPONSE {
	byte RetCode;
	u32 StartTime;		//起始时间
	u32 StartIndex;		//起始位置
	i16 Count;		//条目数, 0 无数据  < 0 错误码
	S_STRUCTURE_DESC Desc;	//结构描述(参考结构描述说明)
	pbyte pData;		//历史数据内容序列
} S_MSG_HISTORY_DOWN_RESPONSE;


/************/
//获取控制盒版本相关
typedef struct _S_VERSION_ {	//版本结构
	byte Major;		//主版本号
	byte Minor;		//子版本号
	//byte Revision;	//修正版本号(不使用)
	//ushort Build;		//编译版本号(不使用)
} S_VERSION_T;

// 无成员
//typedef struct _S_MSG_DEVICE_VERSION_REQUEST {
//} S_MSG_DEVICE_VERSION_REQUEST;

typedef struct _S_MSG_DEVICE_VERSION_RESPONSE {
	byte RetCode;
	S_VERSION_T HWVersion;	//硬件版本
	S_VERSION_T SWVersion;	//软件版本
} S_MSG_DEVICE_VERSION_RESPONSE;


/************/
//控制盒升级相关
typedef struct _S_UPDATE_UP_SUMMARY {
	S_VERSION_T HWVersion;	//硬件版本
	S_VERSION_T SWVersion;	//软件版本
	i32 Length;		//升级包长度
	u32 DesCrc32;
	u32 BinCrc32;
} S_UPDATE_UP_SUMMARY;

typedef struct _S_UPDATE_UP_DETAIL {
	u32 StartIndex;		//起始位置
	i16 Count;		//字节数, 0 无数据, <0 错误码的负值
	pbyte pData;		//升级包数据内容
} S_UPDATE_UP_DETAIL;

typedef struct _S_MSG_UPDATE_UP_REQUEST {
	byte Type;		//类型(参考升级的数据类型表)
	pbyte pUpdateData;	//数据内容，类型不同结构不同
} S_MSG_UPDATE_UP_REQUEST;

typedef struct _S_MSG_UPDATE_UP_RESPONSE {
	byte RetCode;
	i16 Count;		//成功存储的字节数, 0 无数据, <0 错误码的负值
} S_MSG_UPDATE_UP_RESPONSE;

// 设置采集状态
// 设置正在采集状态即开启一次采集, 设置未采集状态即停止一次采集
typedef struct _S_MSG_SET_SAMPLING_REQUEST {
	u32 Timestamp;		//标准时间戳, 用于记录采样开始的时间/对时
	byte Status;
} S_MSG_SET_SAMPLING_REQUEST;

// 设置采集状态反馈
typedef struct _S_MSG_SET_SAMPLING_RESPONSE {
	byte RetCode;
} S_MSG_SET_SAMPLING_RESPONSE;

// 查询采集状态, 无参数
//typedef struct _S_MSG_GET_SAMPLING_REQUEST {
//} S_MSG_GET_SAMPLING_REQUEST;

// 查询采集状态, 反馈
typedef struct _S_MSG_GET_SAMPLING_RESPONSE {
	byte RetCode;
	byte Status;
} S_MSG_GET_SAMPLING_RESPONSE;

typedef struct _S_PRODUCT_INFO {	//产品信息存储结构
	S_VERSION_T HWVersion;	//硬件版本
	S_VERSION_T SWVersion;	//软件版本
	u32 Timestamp;		//设备出厂时间(UNIX时间戳)
	byte DeviceID[13];	//设备ID
} S_PRODUCT_INFO;

// 无成员
//typedef struct _S_MSG_GET_PRODUCT_INFO_REQUEST {
//} _S_MSG_GET_PRODUCT_INFO_REQUEST;

typedef struct _S_MSG_GET_PRODUCT_INFO_RESPONSE {
	byte RetCode;
	S_PRODUCT_INFO ProductInfo;
} S_MSG_GET_PRODUCT_INFO_RESPONSE;

typedef struct _S_DEVICE_CONFIG_ITEM {
	byte Type;
	u32 Value;
} S_DEVICE_CONFIG_ITEM;

typedef struct _S_MSG_DEVICE_CONFIG_UP_REQUEST {	// 配置设置
	byte Count;             // 配置项数
	S_DEVICE_CONFIG_ITEM *Items;	// 配置项序列
} S_MSG_DEVICE_CONFIG_UP_REQUEST;

typedef struct _S_MSG_DEVICE_CONFIG_UP_RESPONSE {	// 配置设置
	byte RetCode;		//反馈码
} S_MSG_DEVICE_CONFIG_UP_RESPONSE;

typedef struct _S_MSG_DEVICE_CONFIG_DOWN_REQUEST {	// 配置查询
	byte Count;         // 配置项数
	byte *TypeList;     // 配置项序列
} S_MSG_DEVICE_CONFIG_DOWN_REQUEST;

typedef struct _S_MSG_DEVICE_CONFIG_DOWN_RESPONSE {	// 配置查询
	byte RetCode;           //反馈码
	byte Count;             // 配置项数
	S_DEVICE_CONFIG_ITEM *Items;	// 配置项序列
} S_MSG_DEVICE_CONFIG_DOWN_RESPONSE;

typedef struct _S_MSG_DEVICE_POWER_STATUS_RESPONSE {	// 电池状态查询回复
	byte RetCode;           //反馈码
	byte ChargeStatus;      //充电状态 0:未充电 1:正在充电 其他:未知状态
	byte BatteryPer;        //电量百分比, 0xFF 无效值
} S_MSG_DEVICE_POWER_STATUS_RESPONSE;

#pragma pack()

#endif

