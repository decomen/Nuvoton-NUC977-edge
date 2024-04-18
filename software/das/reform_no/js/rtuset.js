
var INVALID_RSSI		= 1000;
var INVALID_SNR			= 1000;

var WEBNET_ERR_GROUP_EXIST 	= -1001;
var WEBNET_ERR_NAME_EXIST 	= -1002;
var WEBNET_ERR_NO_MEM 		= -1003;
var WEBNET_ERR_PARAM 		= -1004;

var VAR_ATTR_NONE		= 0;
var VAR_ATTR_AI			= 1;
var VAR_ATTR_DI			= 2;
var VAR_ATTR_DO			= 3;

var VAR_DEV_TYPE0_AI			= 0x1000;

var VAR_DEV_TYPE0_TTL			= 0x1001;
var VAR_DEV_TYPE1_TTL_4DI_4DO	= 0x0000;
var VAR_DEV_TYPE1_TTL_4DO_4DI	= 0x0001;
var VAR_DEV_TYPE1_TTL_8DI		= 0x0002;
var VAR_DEV_TYPE1_TTL_8DO		= 0x0003;

var MONITOR_ENET_INDEX	= 3;
var MONITOR_GPRS_INDEX	= 5;

var CONFIG_PATH			= "media/nand/cfg";
var UPDATE_PATH			= "media/update";

var ENET_TCPIP_NUM		= 12;
var GPRS_TCPIP_NUM		= 8;
var EXT_VAR_LIMIT		= 256;
var BOARD_ZGB_UART		= 3;
var BOARD_LORA_UART		= 4;

// ---------常量表------------
var PROTO_DEV_RS1		= 0;
var PROTO_DEV_RS2		= 1;
var PROTO_DEV_NET		= 2;
var PROTO_DEV_ZIGBEE		= 3;

var PROTO_DEV_GPRS		= 4;
var PROTO_DEV_LORA		= 5;
var PROTO_DEV_RTU_SELF	= 100; 
var PROTO_DEV_RTU_SELF_MID	= 200; 

var PROTO_DEV_RS_MAX	= 1;

// proto_uart_type_e
var PROTO_MODBUS_RTU	= 0;	// modbus RTU
var PROTO_MODBUS_ASCII	= 1;	// modbus ASCII
var PROTO_DLT645		= 2;	// dlt645 2007
var PROTO_DLT645_1997	= 3;	// dlt645 1997
var PROTO_DUST			= 4;	// 粉尘协议
var PROTO_OBMODBUS_RTU	= 5;	// obmodbus RTU
var PROTO_MBUS603		= 6;	// PROTO_MBUS603
var PROTO_LUA			= 20;   // lua

// proto_tcpip_type_e
var PROTO_MODBUS_TCP	= 0;	// modbus TCP
var PROTO_CC_BJDC		= 1;	// 北京数据采集协议
var PROTO_MODBUS_RTU_OVER_TCP	= 2;	// modbus rtu over TCP
var PROTO_HJT212		= 3;	// hjt212
var PROTO_DM101			= 4;	// dm101
var PROTO_SMF			= 5;	// 赛默飞仪表
var PROTO_MQTT			= 6;	// MQTT
var PROTO_DH			= 7;	// 动环
// ---------------------------

var TCP_IP_M_NORMAL		= 0x00; // 正常通信模式(网关)
var TCP_IP_M_XFER		= 0x01; // 转发模式 

var XFER_M_GW			= 0; // 指定协议网关模式 (指定端口:支持zigbee)
var XFER_M_TRT			= 1; // 无协议透明传输 (指定端口)

var ZGB_TM_GW			= 0; // 网关模式 (采集变量采集)
var ZGB_TM_TRT			= 1; // 无协议透明传输 (指定端口)
var ZGB_TM_DTU			= 2; // 组网转发模式 (转发到串口)

var LORA_TM_GW			= 0; // 网关模式 (采集变量采集)
var LORA_TM_TRT			= 1; // 无协议透明传输 (指定端口)
var LORA_TM_DTU			= 2; // 组网转发模式 (转发到串口)

var RULE_TYPE_SYS		= 0; // 系统事件
var RULE_TYPE_CTRL		= 1; // 控制事件
var RULE_TYPE_SMS		= 2; // 短信事件
var RULE_TYPE_NONE		= 3; // 无输出

var IO_EXP_TYPE_EXP     = 0; // 常规表达式
var IO_EXP_TYPE_RULE    = 1; // 规则引用
var NET_ADAPTER_WIRED 		= 0;   	//本地网络
var NET_ADAPTER_WIRELESS 	= 1;  	//GPRS/LTE
//var GPRS_OR_NBIOT	= "GPRS/LTE/NB-IOT";

var xTcpipCfgList = new Array();
var xUartCfgList = new Array();
var xVarManageExtDataBase = new Array();
var xProtoDevList = new Array();
var xUpProtoDevList = new Array();
var xRuleList = new Array();
//脚本执行器
var xLuaList = new Array();

var type_256_key = ["Ygzdn","Zyggl","Ayggl","Byggl","Cyggl","Zwggl","Awggl","Bwggl","Cwggl","Zszgl","Aszgl","Bszgl","Cszgl","Zglys","Aglys","Bglys","Cglys","Pl","Ady","Bdy","Cdy","Adl","Bdl","Cdl","iYgzdn","iZyggl","iAyggl","iByggl","iCyggl","iZwggl","iAwggl","iBwggl","iCwggl","iZszgl","iAszgl","iBszgl","iCszgl","iZglys","iAglys","iBglys","iCglys","iPl","iAdy","iBdy","iCdy","iAdl","iBdl","iCdl"];
var type_256_desc = ["Ygzdn-有功总电能(单位KWH 精度0.01)","Zyggl-总有功功率(单位W 精度1)","Ayggl-A相有功功率(单位W 精度1)","Byggl-B相有功功率(单位W 精度1)","Cyggl-C相有功功率(单位W 精度1)","Zwggl-总无功功率(单位Var 精度1)","Awggl-A相无功功率(单位Var 精度1)","Bwggl-B相无功功率(单位Var 精度1)","Cwggl-C相无功功率(单位Var 精度1)","Zszgl-总视在功率(单位VA 精度1)","Aszgl-A相视在功率(单位VA 精度1)","Bszgl-B相视在功率(单位VA 精度1)","Cszgl-C相视在功率(单位VA 精度1)","Zglys-总功率因数(单位无)","Aglys-A相功率因数(单位无)","Bglys-B相功率因数(单位无)","Cglys-C相功率因数(单位无)","Pl-频率(单位Hz 精度0.01)","Ady-A相电压(单位V 精度0.1)","Bdy-B相电压(单位Hz 精度0.01)","Cdy-C相电压(单位Hz 精度0.01)","Adl-A相电流(单位A 精度0.01)","Bdl-B相电流(单位A 精度0.01)","Cdl-C相电流(单位A 精度0.01)","iYgzdn-有功总电能状态(0/空正常1告警 2故障)","iZyggl-总有功功率状态(0/空正常1告警 2故障)","iAyggl-A相有功功率状态(0/空正常1告警 2故障)","iByggl-B相有功功率状态(0/空正常1告警 2故障)","iCyggl-C相有功功率状态(0/空正常1告警 2故障)","iZwggl-总无功功率状态(0/空正常1告警 2故障)","iAwggl-A相无功功率状态(0/空正常1告警 2故障)","iBwggl-B相无功功率状态(0/空正常1告警 2故障)","iCwggl-C相无功功率状态(0/空正常1告警 2故障)","iZszgl-总视在功率状态(0/空正常1告警 2故障)","iAszgl-A相视在功率状态(0/空正常1告警 2故障)","iBszgl-B相视在功率状态(0/空正常1告警 2故障)","iCszgl-C相视在功率状态(0/空正常1告警 2故障)","iZglys-总功率因数状态(0/空正常1告警 2故障)","iAglys-A相功率因数状态(0/空正常1告警 2故障)","iBglys-B相功率因数状态(0/空正常1告警 2故障)","iCglys-C相功率因数状态(0/空正常1告警 2故障)","iPl-频率状态(0/空正常1告警 2故障)","iAdy-A相电压状态(0/空正常1告警 2故障)","iBdy-B相电压状态(0/空正常1告警 2故障)","iCdy-C相电压状态(0/空正常1告警 2故障)","iAdl-A相电流状态(0/空正常1告警 2故障)","iBdl-B相电流状态(0/空正常1告警 2故障)","iCdl-C相电流状态(0/空正常1告警 2故障)"];

var type_512_key = ["sd1","sd2","sd3","sd4","sd5","sd6"];
var type_512_desc = ["sd1-配电开关1(1断开 0闭合)","sd2-配电开关2(1断开 0闭合)","sd3-配电开关3(1断开 0闭合)","sd4-配电开关4(1断开 0闭合)","sd5-配电开关5(1断开 0闭合)","sd6-配电开关6(1断开 0闭合)"];

var type_1280_key = ["Tem","Hum","iTem","iHum"];
var type_1280_desc = ["Tem-温度(单位摄氏度 精度0.1)","Hum-湿度(单位% 精度0.1)","iTem-温度状态(0正常1告警 2故障)","iHum-湿度状态(0正常1告警 2故障)"];

var type_1536_key = ["Kind","Status","LineRB","LineYG","Locate","Lmd","Cable"];
var type_1536_desc = ["Kind-预留(无效)","Status-预留(无效)","LineRB-红蓝线状态","LineYG-黄绿线状态","Locate-漏水定位(单位 米 精度0.010无漏水)","Lmd-灵敏度(无效)","Cable-线缆长度(单位 米 精度0.1)"];

var type_1792_key = ["Hfkwd","Sfkwd","Ht","Lt","Qt","Ct","Di","Hdl","Ldl","ExceptV","StatusV","Hfkwd2","Sfkwd2","Ht2","Lt2","Qt2","Ct2","Di2","Hdl2","Ldl2","ExceptV2","StatusV2","Di1","Di2","Do1","Do2"];
var type_1792_desc = ["Hfkwd-回风口温度(单位 摄氏度 精度0.01)","Sfkwd-送风口温度(单位 摄氏度 精度0.01)","Ht-上限温度(单位℃ 精度1)","Lt-下限温度(单位℃ 精度1)","Qt-保留","Ct-当前设定温度(单位℃ 精度1)","Di-当前电流(单位A)","Hdl-电流上限(单位A)","Ldl-电流下限(单位mA)","ExceptV-异常状态 ","StatusV-空调状态(0关机 1制冷 2制热)","Hfkwd2-回风口温度2 (以下仅为双机有效)","Sfkwd2-送风口温度2 ","Ht2-上限温度2 ","Lt2-下限温度2 ","Qt2-保留2 ","Ct2-当前设定温度2 ","Di2-当前电流2 ","Hdl2-电流上限2 ","Ldl2-电流下限2 ","ExceptV2-异常状态2 ","StatusV2-空调状态2 ","Di1-保留 ","Di2-保留 ","Do1-保留 ","Do2-保留"];

var type_2304_key = ["Dio","D01","D02","D03","D04","D05","D06","D07","D08","D09","D10","D11","D12","D13","D14","D15","D16","D17","D18","D19","D20","D21","D22","D23","D24","D25","D26","D27","D28","D29","D30","D31","D32"];
var type_2304_desc = ["整体状态Dio(与实际模块相关)","开关量D01","开关量D02","开关量D03","开关量D04","开关量D05","开关量D06","开关量D07","开关量D08","开关量D09","开关量D10","开关量D11","开关量D12","开关量D13","开关量D14","开关量D15","开关量D16","开关量D17","开关量D18","开关量D19","开关量D20","开关量D21","开关量D22","开关量D23","开关量D24","开关量D25","开关量D26","开关量D27","开关量D28","开关量D29","开关量D30","开关量D31","开关量D32"];

function __is_gprs()
{
	return (GPRS_OR_NBIOT == "GPRS" || GPRS_OR_NBIOT == "LTE");
}

function __none_gprs()
{
	return (GPRS_OR_NBIOT == "NONE");
}

function __proto_is_master_fixed(proto_type)
{
	return (proto_type == PROTO_DLT645 || proto_type == PROTO_DLT645_1997 || proto_type == PROTO_DUST || proto_type == PROTO_MBUS603);
}

function __proto_is_modbus(dev_type, proto_type)
{
	if (dev_type <= PROTO_DEV_RS_MAX) {
		return (proto_type <= PROTO_MODBUS_ASCII);
	} else if (dev_type <= PROTO_DEV_NET) {
		return (proto_type <= PROTO_MODBUS_TCP || proto_type == PROTO_MODBUS_RTU_OVER_TCP);
	} else if (dev_type <= PROTO_DEV_ZIGBEE) {
		return (proto_type <= PROTO_MODBUS_ASCII);
	} else if (dev_type <= PROTO_DEV_GPRS) {
		return (proto_type <= PROTO_MODBUS_TCP || proto_type == PROTO_MODBUS_RTU_OVER_TCP);
	} else if (dev_type <= PROTO_DEV_LORA) {
		return (proto_type <= PROTO_MODBUS_ASCII);
	}

	return false;
}

function __proto_is_obmodbus(dev_type, proto_type)
{
	if (dev_type <= PROTO_DEV_RS_MAX) {
		return (proto_type == PROTO_OBMODBUS_RTU);
	}

	return false;
}

function __proto_is_dlt645_2007(dev_type, proto_type)
{
	return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DLT645);
}

function __proto_is_dlt645_1997(dev_type, proto_type)
{
	return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DLT645_1997);
}

function __proto_is_dlt645(dev_type, proto_type)
{
	return (__proto_is_dlt645_2007(dev_type, proto_type) || __proto_is_dlt645_1997(dev_type, proto_type));
}

function __proto_is_mbus603(dev_type, proto_type)
{
	return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_MBUS603);
}

function __proto_is_devself(dev_type)
{
	return (dev_type == PROTO_DEV_RTU_SELF);
}

function __proto_is_devself_mid(dev_type)
{
	return (dev_type == PROTO_DEV_RTU_SELF_MID);
}

function __proto_is_dust(dev_type, proto_type)
{
	return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DUST);
}

function __proto_is_smf(dev_type, proto_type)
{
	return ((dev_type == PROTO_DEV_NET || dev_type == PROTO_DEV_GPRS) && proto_type == PROTO_SMF);
}

function __proto_is_dh(dev_type, proto_type)
{
	return ((dev_type == PROTO_DEV_NET || dev_type == PROTO_DEV_GPRS) && proto_type == PROTO_DH);
}

function __format_err_code(_err)
{
	// 0: 正常, -1001: 重复添加组, -1002: 重复变量名(需修改前缀), -1003: 内存不足, -1004: 参数错误
	if (_err == 0) {
		return "成功";
	} else if (_err == WEBNET_ERR_GROUP_EXIST) {
		return "禁止重复添加设备!";
	} else if (_err == WEBNET_ERR_NAME_EXIST) {
		return "重复变量名(需修改前缀)";
	} else if (_err == WEBNET_ERR_NO_MEM) {
		return "内存不足";
	} else if (_err == WEBNET_ERR_PARAM) {
		return "参数错误";
	} else {
		return "其他错误";
	}
}

function createXMLHttpRequest() {
	var request = false;
	if(window.ActiveXObject) {
		var versions = ['Microsoft.XMLHTTP', 'MSXML.XMLHTTP', 'Microsoft.XMLHTTP', 'Msxml2.XMLHTTP.7.0', 'Msxml2.XMLHTTP.6.0', 'Msxml2.XMLHTTP.5.0', 'Msxml2.XMLHTTP.4.0', 'MSXML2.XMLHTTP.3.0', 'MSXML2.XMLHTTP'];
		for(var i=0; i<versions.length; i++) {
			try {
				request = new ActiveXObject(versions[i]);
				if(request) {
					return request;
				}
			} catch(e) {}
		}
	} else if(window.XMLHttpRequest) {
		request = new XMLHttpRequest();
	}
	return request;
}

function _tostr_(_str) { return ''+(_str!=null?_str:''); }
function _tonum_(_num) { return _num!=null?Number(_num):0; }


function getNumber( id )
{
	return Number(window.document.getElementById(id).value);
}

function getChecked( id )
{
	return window.document.getElementById(id).checked;
}

function getValue( id )
{
	return window.document.getElementById(id).value;
}

function setCheckBoxEnable(id, val){
	if(window.document.getElementById(id)){
		window.document.getElementById(id).checked = (Number(val)!=0);
	}
}

function setSelectValueByClass(_class, val){
	 $("."+_class).find("option").removeAttr("selected");
	 $("."+_class).find("option[value="+val+"]").attr("selected",true)
}
function setValueByclass(_class, val){
	$("#"+_class).val(val);
}

function setValue( id, val )
{
	if(window.document.getElementById(id)){
		window.document.getElementById(id).value = '' + val;
	}
}

function setEnable( id, enable )
{
	if(window.document.getElementById(id)){
		window.document.getElementById(id).disabled=!enable;
	}
}

function setDisplay( id, display )
{
	window.document.getElementById(id).style.display=(display?"":"none");
}

function setVisible( id, visible )
{
	window.document.getElementById(id).style.visibility=visible?"visible":"hidden";
}

function MyGetJSON(msg,url,node,data,callback)
{
	var flag = (msg != "" && msg != null);
	if(flag) Show(msg);
	if(data!=null&&data.length>0) {
		data = data.replace(/\%/g, '%25');
		data = data.replace(/\#/g, '%23');
		data = data.replace(/\&/g, '%26');
		data = data.replace(/\+/g, '%2B');
		//data = data.replace(/\\/g, '%2F');
		data = data.replace(/\=/g, '%3D');
		data = data.replace(/\?/g, '%3F');
		data = node+'='+data;
	} else {
		data = '';
	}
	$.ajax({ 
	url: url + data + "&__time="+Date.parse( new Date()),  
	async: true,
	cache: false,
	dataType: 'json', 
	//data: data, 
	success: function(data){
		if(flag) Close();
		if(data.ret != null ) {
			if( data.ret == 401 ) {
				window.location.href="http://"+window.location.host+"/login.html";
			} else if( data.ret == 402 ) {
				window.location.href="http://"+window.location.host+"/reg.html";
			}
		}
		if(callback!=null) callback(data);
	},
	timeout: 5000,
	error: function( ){
		if(flag) Close();
	}
	});
}

function MyGetJSONWithArg(msg,url,data,callback)
{
	MyGetJSON(msg,url,'arg',data,callback)
}

function onUartCfgMsChange(n)
{
	var po = getNumber('uart' + n + '_cfg_proto');
	var ms = getNumber('uart' + n + '_cfg_ms');
	
	if( ms != null && po != null && po == 0 && ms == 0 ) {
		setVisible( 'uart'+ n + '_cfg_addr_lb', true );
		setVisible( 'uart'+ n + '_cfg_addr', true );
	} else {
		setVisible( 'uart'+ n + '_cfg_addr_lb', false );
		setVisible( 'uart'+ n + '_cfg_addr', false );
	}
}

function setUartHtml(n, bd, ut, po, py, ms, ad)
{
	if( bd != null ) setValue('uart' + n + '_cfg_baud', bd); 
	if( ut != null ) setValue('uart' + n + '_cfg_mode', ut); 
	if( po != null ) setValue('uart' + n + '_cfg_proto', po); 
	if( py != null ) setValue('uart' + n + '_cfg_parity', py); 
	if( ms != null ) setValue('uart' + n + '_cfg_ms', ms); 
	if( ad != null ) setValue('uart' + n + '_cfg_addr', ad);
	
	onUartCfgMsChange(n);
}

function setUartCfg(i)
{
	var setval = {
		n:Number(i), 
		bd:getNumber('uart' + i + '_cfg_baud'), 
		ut:getNumber('uart' + i + '_cfg_mode'), 
		po:getNumber('uart' + i + '_cfg_proto'), 
		py:getNumber('uart' + i + '_cfg_parity'), 
		ms:getNumber('uart' + i + '_cfg_ms'), 
		ad:getNumber('uart' + i + '_cfg_addr')
	};
	MyGetJSONWithArg("正在设置串口参数,请稍后...","/cgi-bin/setUartCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			alert( "设置成功" );
		} else {
			alert("设置失败,请重试");
		}
	});
}

function Show(message){
	var shield = document.createElement("DIV");//产生一个背景遮罩层
	shield.id = "shield";
	shield.style.position = "absolute";
	shield.style.left = "0px";
	shield.style.top = "0px";
	shield.style.width = "100%";
	shield.style.height = ((document.documentElement.clientHeight>document.documentElement.scrollHeight)?document.documentElement.clientHeight:document.documentElement.scrollHeight)+"px";
	shield.style.background = "#333";
	shield.style.textAlign = "center";
	shield.style.zIndex = "10000";
	shield.style.filter = "alpha(opacity=0)";
	shield.style.opacity = 0;

	var alertFram = document.createElement("DIV");//产生一个提示框
	var height="40px";
	alertFram.id="alertFram";
	alertFram.style.position = "absolute";
	alertFram.style.width = "200px";
	alertFram.style.height = height;
	alertFram.style.left = "45%";
	alertFram.style.top = "50%";
	alertFram.style.background = "#fff";
	alertFram.style.textAlign = "center";
	alertFram.style.lineHeight = height;
	alertFram.style.zIndex = "100001";

   strHtml =" <div style=\"width:100%; border:#58a3cb solid 1px; text-align:center;\">";
	if (typeof(message)=="undefined"){
		strHtml+=" 正在操作, 请稍候...";
	} 
	else{
		strHtml+=message;
	}
	strHtml+=" </div>";

	alertFram.innerHTML=strHtml;
	document.body.appendChild(alertFram);
	document.body.appendChild(shield);


	var c = 0;
	
	var ad = setInterval(function(){
		doAlpha(c,shield)
	},1);//渐变效果
	document.body.onselectstart = function(){return false;}
	document.body.oncontextmenu = function(){return false;}
}
function doAlpha(c,shield){
	if (++c > 20){clearInterval(ad);return 0;}
	setOpacity(shield,c);
	}

  function setOpacity(obj,opacity){
		if(opacity>=1)opacity=opacity/100;
		try{ obj.style.opacity=opacity; }catch(e){}
		try{ 
			if(obj.filters.length>0&&obj.filters("alpha")){
			obj.filters("alpha").opacity=opacity*100;
			}else{
			obj.style.filter="alpha(opacity=\""+(opacity*100)+"\")";
			}
		}
		catch(e){}
	}

function Close(){
	var shield= window.document.getElementById("shield");
	var alertFram= window.document.getElementById("alertFram");
	if(shield!=null) {
		document.body.removeChild(shield);
	}
	if(alertFram!=null) {
		document.body.removeChild(alertFram);
	} 
	document.body.onselectstart = function(){return true};
	document.body.oncontextmenu = function(){return true};
}

function getAllUartCfg()
{
	MyGetJSONWithArg("正在获取串口配置,请稍后...","/cgi-bin/getUartCfg?", "{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			xUartCfgList = res.list.concat();
			for( var n = 0; n < res.list.length; n++ ) {
				setUartHtml( 
					n, 
					res.list[n].bd, 
					res.list[n].ut, 
					res.list[n].po, 
					res.list[n].py, 
					res.list[n].ms, 
					res.list[n].ad
				);
			}
		} else {
			alert("获取失败,请重试");
		}
	});
}

function getUartCfg(i)
{
	var setval = { n:Number(i) };
	MyGetJSONWithArg("正在获取串口配置,请稍后...","/cgi-bin/getUartCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			setUartHtml( i, res.bd, res.ut, res.po, res.py, res.ms, res.ad );
		} else {
			alert("获取失败,请重试");
		}
	});
}

function Hide_Input()
{
	var n = getNumber('Input_Format');
	setDisplay('Input_Show_Max', 3 == n);
	setDisplay('Input_Show_Min', 3 == n);
	setDisplay('Input_Show_Factor', 3 == n);
}


function refreshAllVarManageExtDataBase(index)
{
	refreshProtoDevList(xProtoDevList, "var_ext_devtype", "var_ext_pro_dev");
	refreshSearchDevList_modbus(xProtoDevList, "var_ext_search_dev");
	refreshImportDevList_modbus(xProtoDevList, "var_ext_import_dev", "var_ext_import_proto");
	refreshImportDevProtoList_modbus(xProtoDevList, 'var_ext_import_dev', 'var_ext_import_proto');
	myTableItemRemoveAll("rtu_var_ext_table");
	for( var n = 0; n < xVarManageExtDataBase.length; n++ ) {
		var _dt = xVarManageExtDataBase[n]['io'].dt;
		var _dtn = xVarManageExtDataBase[n]['io'].dtn;
		var _pt = xVarManageExtDataBase[n]['io'].pt;
		var _saddr = "----"; var _ext_addr = "--"; var _addr = "----";
		if(__proto_is_modbus(_dt, _pt) ||
			__proto_is_obmodbus(_dt, _pt)) {
			_saddr = xVarManageExtDataBase[n]['io'].sa;
			_ext_addr = xVarManageExtDataBase[n]['io'].ea;
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_dlt645(_dt, _pt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
			_saddr = xVarManageExtDataBase[n]['io'].dltad;
		} else if(__proto_is_mbus603(_dt, _pt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
			_saddr = xVarManageExtDataBase[n]['io'].mbus603ad;
		} else if(__proto_is_devself(_dt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_devself_mid(_dt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_dust(_dt, _pt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_smf(_dt, _pt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_dh(_dt, _pt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
			_saddr = xVarManageExtDataBase[n]['io'].dhsid;
		}
		varExtTableAddItem( 
			xVarManageExtDataBase[n].en, 
			xVarManageExtDataBase[n].gp, 
			xVarManageExtDataBase[n].na, 
			xVarManageExtDataBase[n].al, 
			varExtGetVarTypeName(xVarManageExtDataBase[n]['io'].ovt,xVarManageExtDataBase[n]['io'].ovs), 
			xVarManageExtDataBase[n]['io'].va, 
			inProtoDevList(xProtoDevList, _dt, _dtn, _pt) ? varExtGetProtoName(_dt, _dtn, _pt):"---", 
			_saddr, _ext_addr, _addr, 
			xVarManageExtDataBase[n]['storage'].se
		);
	}
	var table = window.document.getElementById("rtu_var_ext_table");
	onExtTableItemClick(table, table.rows[index]);
}

function getAllVarManageExtDataBase(_index)
{
	MyGetJSONWithArg("正在获取采集变量表,请稍后...","/cgi-bin/getVarManageExtData?", "{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			xVarManageExtDataBase = res.list.concat();
			xProtoDevList = res.protolist;
			if (_index > (xVarManageExtDataBase.length - 1)) _index = (xVarManageExtDataBase.length - 1);
			refreshAllVarManageExtDataBase(_index);
		} else {
			alert("获取失败,请重试");
		}
	});
}

function getAllVarManageExtDataVals()
{
	MyGetJSONWithArg("","/cgi-bin/getVarManageExtDataVals?", "{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			var vals = res.list;
			var table = window.document.getElementById('rtu_var_ext_table');
			var rowNum = table.rows.length;
			for(var i=0;i<rowNum;i++) {
				table.rows[i].cells[6].innerHTML = vals[i];
			}
		}
	});
}

function getVarExtInfo()
{
	var obj = window.document.getElementById("var_ext_id");
	var id = -1;
	if( obj != null && obj.value.length > 0 ) {
		id = Number(obj.value);
	}
	
	if( id >= 0 && id < EXT_VAR_LIMIT ) {
		var setval = { n:Number(id) };
		
		MyGetJSONWithArg("正在设置采集变量信息,请稍后...","/cgi-bin/getVarManageExtData?", JSON.stringify(setval), function (res) {
			if( res != null && 0 == res.ret ) {
				if( xVarManageExtDataBase[id] != null ) {
					xVarManageExtDataBase[id] = res;
				}
				var table = window.document.getElementById("rtu_var_ext_table");
				onExtTableItemClick(table, table.rows[id]);
			} else {
				alert("获取失败,请重试");
			}
		});
	} else {
		alert("请先在列表中，选择要修改的选项，再进行读取");
	}
}

function myTableItemRemoveAll(id)
{
	var table = window.document.getElementById(id);
	var rowNum = table.rows.length;
	if(rowNum > 0) {
		for(i=0;i<rowNum;i++) {
			table.deleteRow(i);
			rowNum = rowNum-1;
			i = i-1;
		}
	}
}

var var_ext_select_item_index = -1;

function onExtTableItemClick(tb,row)
{
	if( row != null && row.rowIndex != null && row.rowIndex >= 0 ) {
				
		for (var i = 0; i < tb.rows.length; i++) {
			if( xVarManageExtDataBase[i].en > 0 ) {
				tb.rows[i].style.background="#FFFFFF";
			} else {
				tb.rows[i].style.background="#F0F0F0";
			}
		}
		row.style.background="#E5E5E5";
		
		var _rowIndex = row.rowIndex;//去掉表头
		setValue('var_ext_id', _rowIndex);//编号
		setCheckBoxEnable('var_ext_enable',xVarManageExtDataBase[_rowIndex].en);//启用
		
		//setValue('var_ext_enable', xVarManageExtDataBase[_rowIndex].en);
		setValue('var_ext_name0', xVarManageExtDataBase[_rowIndex].na);
		setValue('var_ext_name1', xVarManageExtDataBase[_rowIndex].na);
		setValue('var_ext_alias0', xVarManageExtDataBase[_rowIndex].al);
		setValue('var_ext_alias1', xVarManageExtDataBase[_rowIndex].al);
				
		//下拉框赋值方法
		//setSelectValueByClass('var_ext_vartype', xVarManageExtDataBase[_rowIndex]['io'].vt);
		setValue('var_ext_vartype0', xVarManageExtDataBase[_rowIndex]['io'].ovt);
		setValue('var_ext_vartype1', xVarManageExtDataBase[_rowIndex]['io'].vt);
		//setValue('var_ext_vartype', xVarManageExtDataBase[_rowIndex]['io'].vt);
		setValue('var_ext_vartype_rule', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vrl));
		setValue('var_ext_out_vartype', xVarManageExtDataBase[_rowIndex]['io'].ovt);
		setValue('var_ext_out_vartype_rule', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].ovrl));
		setValue('var_ext_vmax', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vma));
		setValue('var_ext_vmin', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vmi));
		setValue('var_ext_vinit', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vii));
		setValue('var_ext_vratio', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vrt));
				
		if (xVarManageExtDataBase[_rowIndex].gp != null && xVarManageExtDataBase[_rowIndex].gp.length > 0) {
			setDisplay('btn_del_group', true);
		} else {
			setDisplay('btn_del_group', false);
		}
		
		setCheckBoxEnable('var_ext_dev_rtu_self', PROTO_DEV_RTU_SELF == xVarManageExtDataBase[_rowIndex]['io'].dt || PROTO_DEV_RTU_SELF_MID == xVarManageExtDataBase[_rowIndex]['io'].dt);
		if(getChecked('var_ext_dev_rtu_self')) {
			if (PROTO_DEV_RTU_SELF == xVarManageExtDataBase[_rowIndex]['io'].dt) {
				setValue('var_ext_dev_rtu_self_type', '0');
			} else if (PROTO_DEV_RTU_SELF_MID == xVarManageExtDataBase[_rowIndex]['io'].dt) {
				setValue('var_ext_dev_rtu_self_type', '1');
			}
			setValue('var_ext_dev_rtu_self_list', xVarManageExtDataBase[_rowIndex]['io'].dtn);
			ext_oncheck_self_dev();
		}
		setValue('var_ext_devtype', xVarManageExtDataBase[_rowIndex]['io'].dt + "|" + xVarManageExtDataBase[_rowIndex]['io'].dtn + "|" + xVarManageExtDataBase[_rowIndex]['io'].pt);
		setValue('var_ext_pro_dev', xVarManageExtDataBase[_rowIndex]['io'].dt + "|" + xVarManageExtDataBase[_rowIndex]['io'].dtn);
		setValue('var_ext_pro_type', xVarManageExtDataBase[_rowIndex]['io'].pt);
		setValue('var_ext_addr', xVarManageExtDataBase[_rowIndex]['io'].ad);
		setValue('var_err_op', xVarManageExtDataBase[_rowIndex]['io'].eop);
		setValue('var_err_cnt', xVarManageExtDataBase[_rowIndex]['io'].ecnt);

		if(__proto_is_modbus(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt) ||
			__proto_is_obmodbus(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_sync_faddr', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].sfa));
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].sa);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].sa);
			if (__proto_is_modbus(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
				setValue('var_modbus_op', xVarManageExtDataBase[_rowIndex]['io'].mbop);
			} else {
				setValue('var_obmodbus_op', xVarManageExtDataBase[_rowIndex]['io'].mbop);
			}
			setValue('var_ext_extaddr', xVarManageExtDataBase[_rowIndex]['io'].ea);
			setValue('var_ext_extaddrofs', xVarManageExtDataBase[_rowIndex]['io'].ao);
		} else if(__proto_is_dlt645_2007(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_dlt546_op', '0x' + numToString(xVarManageExtDataBase[_rowIndex]['io'].dltop, 16, 8));
		} else if(__proto_is_dlt645_1997(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_dlt546_1997_op', '0x' + numToString(xVarManageExtDataBase[_rowIndex]['io'].dltop, 16, 4));
		} else if(__proto_is_mbus603(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].mbus603ad);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].mbus603ad);
			setValue('var_ext_mbus603_op', xVarManageExtDataBase[_rowIndex]['io'].mbus603op);
		} else if(__proto_is_dust(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_dust_op', xVarManageExtDataBase[_rowIndex]['io'].dustop);
		} else if(__proto_is_smf(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_smf_op', xVarManageExtDataBase[_rowIndex]['io'].smfop);
		} else if(__proto_is_dh(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].dhsid);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].dhsid);
			setValue('var_ext_dh_type', xVarManageExtDataBase[_rowIndex]['io'].dhtype);
			var_dh_type_change();
			setValue('var_ext_dh_key', xVarManageExtDataBase[_rowIndex]['io'].dhkey);
		}
		
		setValue('var_ext_varrw', xVarManageExtDataBase[_rowIndex]['io'].rw);
		setCheckBoxEnable('var_ext_storage_en0', xVarManageExtDataBase[_rowIndex]['storage'].se);//数据存盘
		setCheckBoxEnable('var_ext_storage_en1', xVarManageExtDataBase[_rowIndex]['storage'].se);//数据存盘
		//chen qq,有多个相同节点存在时，使用class赋值
		//setSelectValueByClass('var_ext_storage_en', xVarManageExtDataBase[_rowIndex]['storage'].se);//数据存盘
		setValue('var_ext_storage_step', xVarManageExtDataBase[_rowIndex]['storage'].ss);//存盘间隔
		
		var _exp_t = xVarManageExtDataBase[_rowIndex]['io'].exp_t;
		setValue('var_ext_exp_type', _exp_t);
		
		if (_exp_t == IO_EXP_TYPE_EXP) {
			setValue('var_ext_exp', xVarManageExtDataBase[_rowIndex]['io']['exp']);
			var_ext_exp_type_change(null);
		} else if (_exp_t == IO_EXP_TYPE_RULE) {
			var _rule = xVarManageExtDataBase[_rowIndex]['io'].rule;
			rule_create_select_list('var_ext_exp_rule');
			var_ext_exp_type_change(_rule.name);
			var_ext_exp_rule_string_toparam('rule_param_in', _rule.p_in);
			var_ext_exp_rule_string_toparam('rule_param_out', _rule.p_out);
			var_ext_rule_name_bak = _rule.name;
			var_ext_rule_p_in_bak = _rule.p_in;
			var_ext_rule_p_out_bak = _rule.p_out;
		}

		switch(xVarManageExtDataBase[_rowIndex].attr) {
		case VAR_ATTR_NONE:
		case VAR_ATTR_DI:
			setDisplay("dataAttrtab4_head", false);
			setDisplay("dataAttrtab5_head", false);
			break;
		case VAR_ATTR_AI:
			setDisplay("dataAttrtab4_head", true);
			setDisplay("dataAttrtab5_head", false);
			setValue('Input_Range_Engineering', '');
			setValue('Input_Range_Electrical', '');
			setValue('Input_Range', xVarManageExtDataBase[_rowIndex]['cfg'].range);
			setValue('Input_Format', xVarManageExtDataBase[_rowIndex]['cfg'].unit);
			setValue('Input_Range_Min', xVarManageExtDataBase[_rowIndex]['cfg'].rmin);
			setValue('Input_Range_Max', xVarManageExtDataBase[_rowIndex]['cfg'].rmax);
			setValue('Input_Correction_Factor', xVarManageExtDataBase[_rowIndex]['cfg'].fact);
			Hide_Input();
			break;
		case VAR_ATTR_DO:
			setDisplay("dataAttrtab4_head", false);
			setDisplay("dataAttrtab5_head", true);
			setValue("Output_State", 0==Number(xVarManageExtDataBase[_rowIndex]['io'].va)?"低":"高");
			break;
		}

		onVarExtVartypeChange("var_ext_vartype0", false );
		onVarExtVartypeChange("var_ext_vartype1", false );
	}
}

function onExtTableItemDbClick(tb,row)
{
	var_ext_add_flag = false;
	var_ext_select_item_index = row.rowIndex;
	onExtTableItemClick(tb,row);
	$(".J_data_tab li").removeClass("selected");
	$("#dataAttrtab1_head").addClass("selected");
	$(".data_tab_content").addClass("hide");
	$("#dataAttrtab1").removeClass("hide");
	showDialog('data_dialog');
	ext_onchange_proto(false);
	ext_oncheck_self_dev();
	onExtTableItemClick(tb,row);
}

function varExtGetRW(rw)
{
	switch(Number(rw)) {
	case 0: return "只读";
	case 1: return "只写";
	case 2: return "读写";
	}
	
	return "未知";
}

function varExtGetProtoDev(dev,n)
{
	var str = "";
	dev = Number(dev);
	n = Number(n);
	switch(dev) {
	case PROTO_DEV_RS1: case PROTO_DEV_RS2: {
		return "COM"+(dev+1);
	}
	case PROTO_DEV_NET: {
		return "Net" + (n+1);
	}
	case PROTO_DEV_LORA: {
		return "LoRa";
	}
	case PROTO_DEV_GPRS: {
		return GPRS_OR_NBIOT + (n+1);
	}
	case PROTO_DEV_RTU_SELF: {
		return "";
	}
	case PROTO_DEV_RTU_SELF_MID: {
		return "中间变量";
	}
	}
	
	return "";
}

function varExtGetProto(dev,proto)
{
	var str = "";
	dev = Number(dev);
	proto = Number(proto);
	switch(dev) {
	case PROTO_DEV_RS1: case PROTO_DEV_RS2: {
		switch(proto) {
		case PROTO_MODBUS_RTU: return "Modbus_RTU";
		case PROTO_MODBUS_ASCII: return "Modbus_ASCII";
		case PROTO_DLT645: return "DLT645-2007";
		case PROTO_DLT645_1997: return "DLT645-1997";
		case PROTO_DUST: return "粉尘浓度测量仪协议";
		case PROTO_MBUS603: return "Mbus-603";
		case PROTO_OBMODBUS_RTU: return "Modbus_RTU(单字节寄存器)";
		case PROTO_LUA: return "Lua协议";
		}
		break;
	}
	case PROTO_DEV_NET: {
		switch(proto) {
		case PROTO_MODBUS_TCP: return "Modbus_TCP";
		case PROTO_CC_BJDC: return "大型公建通讯协议";
		case PROTO_MODBUS_RTU_OVER_TCP: return "Modbus_RTU_Over_TCP";
		case PROTO_HJT212: return "HJ/T212";
		case PROTO_DM101: return "DM101";
		case PROTO_SMF: return "赛默飞仪表";
		case PROTO_MQTT: return "MQTT";
		case PROTO_DH: return "动环";
		}
		break;
	}
	case PROTO_DEV_LORA: {
		switch(proto) {
		case PROTO_MODBUS_RTU: return "Modbus_RTU";
		case PROTO_MODBUS_ASCII: return "Modbus_ASCII";
		}
		break;
	}
	case PROTO_DEV_GPRS: {
		switch(proto) {
		case PROTO_MODBUS_TCP: return "Modbus_TCP";
		case PROTO_MODBUS_RTU_OVER_TCP: return "Modbus_RTU_Over_TCP";
		case PROTO_HJT212: return "HJ/T212";
		case PROTO_DM101: return "DM101";
		case PROTO_MQTT: return "MQTT";
		}
		break;
	}
	}
	
	return "";
}

function varExtGetProtoName(dev,n,proto)
{
	if(Number(dev) != PROTO_DEV_RTU_SELF && Number(dev) != PROTO_DEV_RTU_SELF_MID) {
		var str = varExtGetProtoDev(dev,n) + "_" +  varExtGetProto(dev,proto);
		if(str.length > 0) return str;
	} else {
		var str = varExtGetProtoDev(dev,n);
		if(str.length > 0) return str;
	}
	return "ERROR";
}

function varExtGetUpProtoName(dev,n,proto)
{
	if(Number(dev) != PROTO_DEV_RTU_SELF && Number(dev) != PROTO_DEV_RTU_SELF_MID) {
		var str = varExtGetProtoDev(dev, n) + "_" +  varExtGetProto(dev,proto);
		if(str.length > 0) return str;
	} else {
		var str = varExtGetProtoDev(dev,n);
		if(str.length > 0) return str;
	}
	return "ERROR";
}

function varExtGetVarTypeName(type,len)
{
	switch(Number(type)){
	case 0: return "BIT";
	case 1: return "INT8";
	case 2: return "UINT8";
	case 3: return "INT16";
	case 4: return "UINT16";
	case 5: return "INT32";
	case 6: return "UINT32";
	case 7: return "FLOAT";
	case 8: return "DOUBLE";
	case 9: return "ARRAY("+len+")";
	default: return "ERROR";
	}
}

function varExtGetVarTypeVS(type)
{
	switch(Number(type)){
	case 0: return 1;
	case 1: return 1;
	case 2: return 1;
	case 3: return 2;
	case 4: return 2;
	case 5: return 4;
	case 6: return 4;
	case 7: return 4;
	case 8: return 8;
	case 9: return 0;
	default: return 0;
	}
}

function varExtTableAddItem(enable,group,name,alias,valtype,val,protodev,slaveaddr,extaddr,addr,storage)
{
	var index = 0;
	var table = window.document.getElementById("rtu_var_ext_table");
	var row = table.insertRow(table.rows.length);
	row.style.height="25px";
	row.onclick = function(){ onExtTableItemClick( table, row ); };
	row.ondblclick = function(){ onExtTableItemDbClick( table, row ); };
	var obj = row.insertCell(index++);
	obj.innerHTML = enable!=0?"启用":"未启用";
	obj = row.insertCell(index++);
	obj.innerHTML = table.rows.length-1;
	obj = row.insertCell(index++);
	obj.innerHTML = group;
	obj = row.insertCell(index++);
	obj.innerHTML = name;
	obj = row.insertCell(index++);
	obj.innerHTML = alias;
	obj = row.insertCell(index++);
	obj.innerHTML = valtype;
	obj = row.insertCell(index++);
	obj.innerHTML = val;
	obj = row.insertCell(index++);
	obj.innerHTML = enable!=0?protodev:'--';
	obj = row.insertCell(index++);
	obj.innerHTML = slaveaddr;
	obj = row.insertCell(index++);
	obj.innerHTML = extaddr;
	obj = row.insertCell(index++);
	obj.innerHTML = addr;
	obj = row.insertCell(index++);
	obj.innerHTML = Number(storage)!=0?"是":"否";
}

function refresh_type_rule(_id,_n)
{
	var _rule = getValue(_id);
	var selectobj = window.document.getElementById(_id);
	if(selectobj==null) return ;
	selectobj.options.length = 0;
	setDisplay(_id, true);
	if( _n < 5 || _n == 9 || getChecked('var_ext_dev_rtu_self')) {
		setDisplay(_id, false);
		selectobj.value = 0; 
	} else if( _n < 8 ) {
		selectobj.options.add(new Option('AB CD', '0'));
		selectobj.options.add(new Option('CD AB', '1'));
		selectobj.options.add(new Option('BA DC', '2'));
		selectobj.options.add(new Option('DC BA', '3'));
	} else if( _n == 8 ) {
		selectobj.options.add(new Option('AB CD EF GH', '0'));
		selectobj.options.add(new Option('GH EF CD AB', '1'));
		selectobj.options.add(new Option('BA DC FE HG', '2'));
		selectobj.options.add(new Option('HG FE DC BA', '3'));
	}
	selectobj.value = ''+_rule;
}

function onVarExtVartypeChange(_id,_change)
{
	var obj = window.document.getElementById(_id);
	if (_id == 'var_ext_vartype1') {
		refresh_type_rule('var_ext_vartype_rule', obj.selectedIndex);
		if(_change) setValue("var_ext_vartype_rule", '0' );
	} else {
		//alert("getValue(var_ext_out_vartype_rule)" + getValue('var_ext_out_vartype_rule'));
		refresh_type_rule('var_ext_out_vartype_rule', obj.selectedIndex);
		if(_change) setValue("var_ext_out_vartype_rule", '0' );
	}
}

function applyVarExtInfo(a,b)
{
	setValue('var_ext_name'+a, getValue('var_ext_name'+b));
	setValue('var_ext_alias'+a, getValue('var_ext_alias'+b));
	if (a == 0) {
		setValue('var_ext_vartype'+a, getValue('var_ext_out_vartype'));
	} else {
		setValue('var_ext_out_vartype', getValue('var_ext_vartype'+b));
	}
	setValue('var_ext_slaveaddr'+a, getValue('var_ext_slaveaddr'+b));
	setCheckBoxEnable('var_ext_storage_en'+a, getChecked('var_ext_storage_en'+b));
	onVarExtVartypeChange("var_ext_vartype0", false );
	onVarExtVartypeChange("var_ext_vartype1", false );
	if( Number(a)==0 ) {
		if(getChecked('var_ext_dev_rtu_self')) {
			setValue('var_ext_devtype', '100|'+getNumber('var_ext_dev_rtu_self_list')+'|0');
		} else {
			setValue('var_ext_devtype', getValue('var_ext_pro_dev')+'|'+getValue('var_ext_pro_type'));
		}
	}
}

function getVarExtMapAddr()
{
	var _max_addr = 0;
	if( xVarManageExtDataBase != null ) {
		for( var n = 0; n < xVarManageExtDataBase.length; n++ ) {
			var _io = xVarManageExtDataBase[n]['io'];
			if( _io != null && _io.ad != null && _io.ovs != null ) {
				if( Number(_io.ad) >= _max_addr ) {
					_max_addr = Number(_io.ad)+(Number(_io.ovs)/2);
				}
			}
		}
	}
	return (_max_addr<1024?1024:_max_addr);
}

var var_ext_add_flag = false;
var var_ext_rule_name_bak = "";
var var_ext_rule_p_in_bak = "";
var var_ext_rule_p_out_bak = "";

function var_ext_add_init()
{
	var_ext_add_flag = true;
}

function addVarExtInfo()
{
	if( xVarManageExtDataBase != null ) {
		setValue('var_ext_id', xVarManageExtDataBase.length );
	} else {
		setValue('var_ext_id', "0" );
	}
	var _id = getNumber('var_ext_id');
	setCheckBoxEnable('var_ext_enable',1);
	setValue('var_ext_name0', "VR"+(1+_id));
	setValue('var_ext_name1', "VR"+(1+_id));
	setValue('var_ext_alias0', "");
	setValue('var_ext_alias1', "");
	setValue('var_ext_vartype0', 4);
	setValue('var_ext_out_vartype', 4);
	setValue('var_ext_vartype1', 4);
	
	setValue('var_ext_vmax', "");
	setValue('var_ext_vmin', "");
	setValue('var_ext_vinit', "");
	setValue('var_ext_vratio', "");
	
	setValue('var_ext_devtype', "");
	setValue('var_ext_pro_dev', "");
	setValue('var_ext_pro_type', "");
	setValue('var_ext_sync_faddr', 0);
	setValue('var_err_op', 0);
	setValue('var_err_cnt', 0);
	
	setValue('var_ext_addr', getVarExtMapAddr());
	setValue('var_ext_slaveaddr0', 5);
	setValue('var_ext_slaveaddr1', 5);
	setValue('var_modbus_op', 3);
	setValue('var_ext_extaddr', 0);
	setValue('var_ext_extaddrofs', 0);
	setValue('var_ext_varrw', 0);
	setCheckBoxEnable('var_ext_storage_en0', 0);
	setCheckBoxEnable('var_ext_storage_en1', 0);
	setValue('var_ext_storage_step', 5);
	setValue('var_ext_exp', "");
	
	$(".J_data_tab li").removeClass("selected");
	$("#dataAttrtab1_head").addClass("selected");
	$(".data_tab_content").addClass("hide");
	$("#dataAttrtab1").removeClass("hide");
	
	setDisplay("dataAttrtab4_head", false);
	setDisplay("dataAttrtab5_head", false);
}

function checkVarExtName(_name, _n)
{
	if( xVarManageExtDataBase == null || _n >= xVarManageExtDataBase.length ) {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].na != null && xVarManageExtDataBase[n].na == _name ) {
				return n;
			}
		}
	} else {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].na != null && n != _n && xVarManageExtDataBase[n].na == _name ) {
				return n;
			}
		}
	}
	return -1;
}

function checkVarAddr(_addr, _n, _sz)
{
	var _a = _addr;
	var _b = _addr + _sz - 1;
	for(var n = 0; n < xVarManageExtDataBase.length; n++) {
		if (n != _n) {
			var _c = xVarManageExtDataBase[n]['io'].ad;
			var _d = xVarManageExtDataBase[n]['io'].ad + Math.round(xVarManageExtDataBase[n]['io'].ovs / 2) - 1;
			if (_a == _c || _a ==_d || _b == _c || _b == _d) {
				return n;
			} else if (_a < _c && _c < _b && _b < _d) {
				return n;
			} else if (_a < _c && _d < _b) {
				return n;
			} else if (_c < _a && _a < _d && _d < _b) {
				return n;
			} else if (_c < _a && _b < _d) {
				return n;
			} 
		}
	}
	return -1;
}

function checkVarExtAlias(_alias, _n)
{
	if( xVarManageExtDataBase == null || _n >= xVarManageExtDataBase.length ) {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].na != null && xVarManageExtDataBase[n].al == _alias && _alias.length > 0 ) {
				return n;
			}
		}
	} else {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].al != null && n != _n && xVarManageExtDataBase[n].al == _alias && _alias.length > 0 ) {
				return n;
			}
		}
	}
	return -1;
}

function setVarExtInfo()
{
	var obj = window.document.getElementById("var_ext_id");
	var id = -1;
	if( obj != null && obj.value.length > 0 ) {
		id = Number(obj.value);
	}
		
	if( id >= 0 && id < EXT_VAR_LIMIT ) {
		
		onVarExtVartypeChange("var_ext_vartype0", false );
		onVarExtVartypeChange("var_ext_vartype1", false );
		
		var addr = getNumber('var_ext_addr');

		if( addr < 1024 || addr > 10240 ) {
			alert("内部映射地址范围(1024->10240), 请修改后重新保存");
			return -1;
		}
		
		var err_cnt = getNumber('var_err_cnt');

		if( err_cnt > 255 ) {
			alert("错误次数范围(0->255), 请修改后重新保存");
			return -1;
		}
		
		var _var_ext_name = getValue('var_ext_name0');
		var _var_ext_alias = getValue('var_ext_alias0');
		if (chk(_var_ext_name, "采集变量名称")) return -1;
		
		if(getChecked('var_ext_dev_rtu_self')) {
			var self_type = getNumber('var_ext_dev_rtu_self_type');
			if (0 == self_type) {
				setValue('var_ext_vartype0', 7);
				setValue('var_ext_vartype1', 7);
				setValue('var_ext_out_vartype', 7);
			} else if (1 == self_type) {
				setValue('var_ext_vartype0', 8);
				setValue('var_ext_vartype1', 8);
				setValue('var_ext_out_vartype', 8);
			}
		}
		
		var proto = getValue("var_ext_devtype").split("|");
		var _dt = 0;
		if (getChecked('var_ext_dev_rtu_self')) {
			var self_type = getNumber('var_ext_dev_rtu_self_type');
			if (0 == self_type) {
				_dt = PROTO_DEV_RTU_SELF;
			} else if (1 == self_type) {
				_dt = PROTO_DEV_RTU_SELF_MID;
			}
		} else {
			_dt = Number(proto[0]);
		}

		var _exp_type = getNumber('var_ext_exp_type');
		
		var _io = {
			rw:getNumber('var_ext_varrw'),
			vt:getNumber('var_ext_vartype1'), 
			vrl:getNumber('var_ext_vartype_rule'), 
			ovt:getNumber('var_ext_out_vartype'), 
			ovrl:getNumber('var_ext_out_vartype_rule'), 
			dt:_dt, 
			dtn:getChecked('var_ext_dev_rtu_self')?getNumber('var_ext_dev_rtu_self_list'):Number(proto[1]), 
			pt:getChecked('var_ext_dev_rtu_self')?0:Number(proto[2]), 
			ad:getNumber('var_ext_addr'),
			'exp_t':_exp_type,
			'eop':getNumber('var_err_op'),
			'ecnt':getNumber('var_err_cnt')
		};
		
		_io['vs'] = varExtGetVarTypeVS(_io.vt);
		_io['ovs'] = varExtGetVarTypeVS(_io.ovt);
    
		if (_exp_type == IO_EXP_TYPE_EXP) {
			_io['exp'] = getValue('var_ext_exp');
		} else {
			var _rule_name = getValue('var_ext_exp_rule');
			if (_rule_name.length <= 0) {
				alert("使用规则时需要选择一个存在的规则!");
				return -1;
			}
			var _rule = {
				name:getValue('var_ext_exp_rule'),
				p_in:var_ext_exp_rule_param_tostring('rule_param_in'),
				p_out:var_ext_exp_rule_param_tostring('rule_param_out')
			}
			
			_io['rule'] = _rule;
		}
		
		if(__proto_is_modbus(_io.dt, _io.pt) ||
			__proto_is_obmodbus(_io.dt, _io.pt)) {
			
			var slaveaddr = getNumber('var_ext_slaveaddr0');
		
			if( slaveaddr < 0 || slaveaddr > 255 ) {
				alert("Modbus从机地址范围(0->255), 请修改后重新保存");
				return -1;
			}
			
			_io['sa'] = slaveaddr;
			_io['ea'] = getNumber('var_ext_extaddr');
			_io['mbop'] = __proto_is_modbus(_io.dt, _io.pt) ? getNumber('var_modbus_op') : getNumber('var_obmodbus_op');
			_io['ao'] = getNumber('var_ext_extaddrofs');
			_io['sfa'] = getNumber('var_ext_sync_faddr');
			
			var _var_ext_vmax = getValue('var_ext_vmax');
			var _var_ext_vmin = getValue('var_ext_vmin');
			var _var_ext_vinit = getValue('var_ext_vinit');
			var _var_ext_vratio = getValue('var_ext_vratio');
			if( _var_ext_vmax != null && _var_ext_vmax.length > 0 ) {
				_io['vma'] = Number(_var_ext_vmax);
			}
			if( _var_ext_vmin != null && _var_ext_vmin.length > 0 ) {
				_io['vmi'] = Number(_var_ext_vmin);
			}
			if( _var_ext_vinit != null && _var_ext_vinit.length > 0 ) {
				_io['vii'] = Number(_var_ext_vinit);
			}
			if( _var_ext_vratio != null && _var_ext_vratio.length > 0 ) {
				_io['vrt'] = Number(_var_ext_vratio);
			}
			
		} else if(__proto_is_dlt645_2007(_io.dt, _io.pt)) {
			var dltaddr = getValue('var_ext_slaveaddr0');
		
			if( dltaddr.length != 12 ) {
				alert("DLT645地址长度为12, 请修改后重新保存");
				return -1;
			}
			
			_io['dltad'] = dltaddr;
			_io['dltop'] = parseInt(getValue('var_ext_dlt546_op'),16);
		} else if(__proto_is_dlt645_1997(_io.dt, _io.pt)) {
			var dltaddr = getValue('var_ext_slaveaddr0');
		
			if( dltaddr.length != 12 ) {
				alert("DLT645地址长度为12, 请修改后重新保存");
				return -1;
			}
			
			_io['dltad'] = dltaddr;
			_io['dltop'] = parseInt(getValue('var_ext_dlt546_1997_op'),16);
		} else if(__proto_is_mbus603(_io.dt, _io.pt)) {
			var mbus603addr = getNumber('var_ext_slaveaddr0');
		
			if( mbus603addr > 255 ) {
				alert("mbus603地址应 < 255, 请修改后重新保存");
				return -1;
			}
			
			_io['mbus603ad'] = mbus603addr;
			_io['mbus603op'] = parseInt(getValue('var_ext_mbus603_op'), 10);
		} else if(__proto_is_dust(_io.dt, _io.pt)) {
			_io['dustop'] = getNumber('var_ext_dust_op');
		} else if(__proto_is_smf(_io.dt, _io.pt)) {
			_io['smfop'] = getNumber('var_ext_smf_op');
		} else if(__proto_is_dh(_io.dt, _io.pt)) {
			_io['dhsid'] = getNumber('var_ext_slaveaddr0');
			_io['dhtype'] = getNumber('var_ext_dh_type');
			_io['dhkey'] = getValue('var_ext_dh_key');
		}
		
		//var _alarm = {
		//	en:getNumber('var_ext_enable'), 
		//};
		var _storage = {
			se:getChecked('var_ext_storage_en0')?1:0,
			ss:getNumber('var_ext_storage_step')
		};
		
		var _cfg = { };
		if( xVarManageExtDataBase != null && id < xVarManageExtDataBase.length ) {
			switch(xVarManageExtDataBase[id].attr) {
			case VAR_ATTR_AI: {
				//数据格式
				var Input_Format = getValue("Input_Format");
				//量程设置
				var Input_Range = getValue("Input_Range");
				//最大量程
				var Input_Range_Max = getValue("Input_Range_Max");
				//最小量程
				var Input_Range_Min = getValue("Input_Range_Min");
				//修正系数
				var Input_Correction_Factor = getValue("Input_Correction_Factor");

				if (chk(Input_Format, "AI数据格式")) return;
				if (chk(Input_Range, "AI量程设置")) return;

				if ( 3 == Input_Format ) {
					if (chk(Input_Range_Max, "AI最大量程")) return;
					if (chk(Input_Range_Min, "AI最小量程")) return;
					if (chk(Input_Correction_Factor, "AI修正系数")) return;
				}

				_cfg.unit = Number(Input_Format);
				_cfg.range = Number(Input_Range);
				_cfg.rmax = Number(Input_Range_Max);
				_cfg.rmin = Number(Input_Range_Min);
				_cfg.fact = Number(Input_Correction_Factor);
				break;
			}
			}
		}
		
		var setval = {
			n:id, 
			en:getChecked('var_ext_enable')?1:0, 
			na:getValue('var_ext_name0'), 
			al:getValue('var_ext_alias0'), 
			attr:VAR_ATTR_NONE,
			'io':_io, 
			//'alarm':_alarm, 
			'storage':_storage,
			'cfg':_cfg
		};
		
		if( checkVarExtName(setval.na, id) >= 0 ) {
			alert("变量名重复，请检查！");
			return -1;
		}
		if( checkVarExtAlias(setval.al, id) >= 0 ) {
			alert("别名重复，请检查！");
			return -1;
		}
		
		var __index = checkVarAddr(addr, id, Math.round(_io.ovs / 2));
		if( __index >= 0 ) {
			alert("映射地址已被变量["+ xVarManageExtDataBase[__index].na +"]使用，请检查！\r\n**注意：[" + $('#var_ext_vartype0 option:selected').text() + "]占用" + Math.round(_io.ovs / 2) + "个寄存器**");
			return -1;
		}
		
		if( xVarManageExtDataBase != null && id < xVarManageExtDataBase.length ) {
			xVarManageExtDataBase[id].en=setval.en;
			xVarManageExtDataBase[id].na=setval.na;
			xVarManageExtDataBase[id].al=setval.al;
			
			setval.attr = xVarManageExtDataBase[id].attr;
			
			if(xVarManageExtDataBase[id]['io']==null) xVarManageExtDataBase[id]['io'] = { };
			xVarManageExtDataBase[id]['io'].rw=_io.rw;
			xVarManageExtDataBase[id]['io'].vt=_io.vt;
			xVarManageExtDataBase[id]['io'].vs=_io.vs;
			xVarManageExtDataBase[id]['io'].vrl=_io.vrl;
			xVarManageExtDataBase[id]['io'].ovt=_io.ovt;
			xVarManageExtDataBase[id]['io'].ovs=_io.ovs;
			xVarManageExtDataBase[id]['io'].ovrl=_io.ovrl;
			xVarManageExtDataBase[id]['io'].dt=_io.dt;
			xVarManageExtDataBase[id]['io'].dtn=_io.dtn;
			xVarManageExtDataBase[id]['io'].pt=_io.pt;
			xVarManageExtDataBase[id]['io'].ad=_io.ad;
			if(__proto_is_modbus(_io.dt, _io.pt) ||
				__proto_is_obmodbus(_io.dt, _io.pt)) {
				xVarManageExtDataBase[id]['io'].sa=_io.sa;
				xVarManageExtDataBase[id]['io'].ea=_io.ea;
				xVarManageExtDataBase[id]['io'].ao=_io.ao;
				xVarManageExtDataBase[id]['io'].sfa=_io.sfa;
			} else if(__proto_is_dlt645(_io.dt, _io.pt)) {
				xVarManageExtDataBase[id]['io'].dltad=_io.dltad;
				xVarManageExtDataBase[id]['io'].dltop=_io.dltop;
			} else if(__proto_is_mbus603(_io.dt, _io.pt)) {
				xVarManageExtDataBase[id]['io'].mbus603ad=_io.mbus603ad;
				xVarManageExtDataBase[id]['io'].mbus603op=_io.mbus603op;
			}
			xVarManageExtDataBase[id]['io'].exp=_io.exp;
			
			if( _io.vma != null) xVarManageExtDataBase[id]['io'].vma=_io.vma;
			if( _io.vmi != null) xVarManageExtDataBase[id]['io'].vmi=_io.vmi;
			if( _io.vii != null) xVarManageExtDataBase[id]['io'].vii=_io.vii;
			if( _io.vrt != null) xVarManageExtDataBase[id]['io'].vrt=_io.vrt;
			
			xVarManageExtDataBase[id]['storage'].se=_storage.se;
			xVarManageExtDataBase[id]['storage'].ss=_storage.ss;
			
			switch(xVarManageExtDataBase[id].attr) {
			case VAR_ATTR_AI:
				xVarManageExtDataBase[id]['cfg'].range = _cfg.range;
				xVarManageExtDataBase[id]['cfg'].unit = _cfg.unit;
				xVarManageExtDataBase[id]['cfg'].rmin = _cfg.rmin;
				xVarManageExtDataBase[id]['cfg'].rmax = _cfg.rmax;
				xVarManageExtDataBase[id]['cfg'].fact = _cfg.fact;
				break;
			}
		}
		
		MyGetJSONWithArg("正在设置采集变量信息,请稍后...","/cgi-bin/setVarManageExtData?", JSON.stringify(setval), function (res) {
			if( res != null && 0 == res.ret ) {
				getAllVarManageExtDataBase(0);
				refreshAllVarManageExtDataBase(id);
				alert( "设置成功" );
			} else {
				alert("设置失败,请重试");
			}
		});
	} else {
		alert("请先在列表中，选择要修改的选项，再进行设置");
	}
	
	return 0;
}

function ext_onchange_proto(_resize)
{
	var _show_div_id = 'ext_dlt546_1997_div';
	setDisplay('ext_modbus_div', false);
	setDisplay('ext_dlt546_2007_div', false);
	setDisplay('ext_dlt546_1997_div', false);
	setDisplay('ext_mbus603_div', false);
	setDisplay('ext_dust_div', false);
	setDisplay('ext_smf_div', false);
	setDisplay('ext_dh_div', false);
	setDisplay(_show_div_id, true);
	var protoDevList = window.document.getElementById('var_ext_pro_dev');
	var protoTypeList = window.document.getElementById('var_ext_pro_type');
	if(protoTypeList.options.length > 0 && protoDevList.options.length > 0 ) {
		var devary = protoDevList.value.split("|");
		if(devary.length==2) {
			var _dev = Number(devary[0]);
			var _po = Number(protoTypeList.value);
			if(__proto_is_modbus(_dev,_po) ||
				__proto_is_obmodbus(_dev,_po)) {
				setDisplay('ext_modbus_div', true);
				if (__proto_is_modbus(_dev,_po)) {
					setDisplay('var_modbus_op', true);
					setDisplay('var_obmodbus_op', false);
				} else {
					setDisplay('var_modbus_op', false);
					setDisplay('var_obmodbus_op', true);
				}
				if(_show_div_id != 'ext_modbus_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 4);
					setValue('var_ext_vartype1', 4);
					setValue('var_ext_out_vartype', 4);
				}
			} else if(__proto_is_dlt645_2007(_dev,_po)) {
				setDisplay('ext_dlt546_2007_div', true);
				if(_show_div_id != 'ext_dlt546_2007_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_out_vartype', 7);
				}
			} else if(__proto_is_dlt645_1997(_dev,_po)) {
				setDisplay('ext_dlt546_1997_div', true);
				if(_show_div_id != 'ext_dlt546_1997_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_out_vartype', 7);
				}
			} else if(__proto_is_mbus603(_dev,_po)) {
				setDisplay('ext_mbus603_div', true);
				if(_show_div_id != 'ext_mbus603_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_out_vartype', 7);
				}
			} else if(__proto_is_dust(_dev,_po)) {
				setDisplay('ext_dust_div', true);
				if(_show_div_id != 'ext_dust_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_out_vartype', 7);
				}
			} else if(__proto_is_smf(_dev,_po)) {
				setDisplay('ext_smf_div', true);
				if(_show_div_id != 'ext_smf_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_out_vartype', 7);
				}
			} else if(__proto_is_dh(_dev,_po)) {
				setDisplay('ext_dh_div', true);
				if(_show_div_id != 'ext_dh_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_out_vartype', 7);
				}
			}
		}
	}
	var_dh_type_change();
}

function ext_oncheck_self_dev()
{
	setDisplay('ext_err_op_div', true);
	if(getChecked('var_ext_dev_rtu_self')) {
		setDisplay('ext_modbus_div', false);
		setDisplay('ext_dlt546_2007_div', false);
		setDisplay('ext_dlt546_1997_div', false);
		setDisplay('ext_mbus603_div', false);
		setDisplay('ext_dust_div', false);
		setDisplay('ext_not_rtu_self_div', false);
		setDisplay('ext_not_rtu_self_div1', false);
		setDisplay('var_ext_dev_rtu_self_list_ul', true);
		setValue('var_ext_vartype0', 7);
		setValue('var_ext_vartype1', 7);
		setValue('var_ext_out_vartype', 7);
		setEnable('var_ext_vartype0', false);
		setEnable('var_ext_vartype1', false);
		setEnable('var_ext_out_vartype', false);
		setDisplay('ext_dh_div', false);
		ext_onchange_dev_rtu_self_type();
	} else {
		setDisplay('var_ext_dev_rtu_self_list_ul', false);
		setDisplay('ext_not_rtu_self_div', true);
		setDisplay('ext_not_rtu_self_div1', true);
		setEnable('var_ext_vartype0', true);
		setEnable('var_ext_vartype1', true);
		setEnable('var_ext_out_vartype', true);
		refreshOneProtoDevList(false,'var_ext_pro_dev','var_ext_pro_type');
		ext_onchange_proto(true);
	}
}

function ext_onchange_dev_rtu_self_type()
{
	var self_type = getNumber('var_ext_dev_rtu_self_type');
	setDisplay('var_ext_dev_rtu_self_list', false);
	setDisplay('ext_err_op_div', true);
	if(self_type == 0) {
		setDisplay('var_ext_dev_rtu_self_list', true);
		setValue('var_ext_vartype0', 7);
		setValue('var_ext_vartype1', 7);
		setValue('var_ext_out_vartype', 7);
	} else if(self_type == 1) {
		setDisplay('ext_err_op_div', false);
		setValue('var_ext_vartype0', 8);
		setValue('var_ext_vartype1', 8);
		setValue('var_ext_out_vartype', 8);
	}
}

// 该函数关联性比较强
function refreshOneProtoDevList(isup,prodevid,protypeid)
{
	var protoTypeList = window.document.getElementById(protypeid);
	var n = 0;
	var _dev = getValue(prodevid).split("|");
	if( _dev.length == 2 && isup != null ) {
		var dev = Number(_dev[0]);
		var dev_n = Number(_dev[1]);
		var res = isup?xUpProtoDevList:xProtoDevList;
		protoTypeList.options.length = 0;
		if( res.rs != null ) {
			for( var n = 0; n < res.rs.length; n++ ) {
				if( dev == res.rs[n].id) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.rs[n].id, res.rs[n].po), ""+res.rs[n].po ) 
					);
				}
			}
		}
		if( res.net != null ) {
			for( var n = 0; n < res.net.length; n++ ) {
				if( dev == res.net[n].id && dev_n == res.net[n].idn) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.net[n].id, res.net[n].po), ""+res.net[n].po ) 
					);
				}
			}
		}
		if( res.zigbee != null ) {
			for( var n = 0; n < res.zigbee.length; n++ ) {
				if( dev == res.zigbee[n].id ) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.zigbee[n].id, res.zigbee[n].po), ""+res.zigbee[n].po ) 
					);
				}
			}
		}
		if( res.lora != null ) {
			for( var n = 0; n < res.lora.length; n++ ) {
				if( dev == res.lora[n].id ) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.lora[n].id, res.lora[n].po), ""+res.lora[n].po ) 
					);
				}
			}
		}
		if( res.gprs != null ) {
			for( var n = 0; n < res.gprs.length; n++ ) {
				if( dev == res.gprs[n].id && dev_n == res.gprs[n].idn ) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.gprs[n].id, res.gprs[n].po), ""+res.gprs[n].po ) 
					);
				}
			}
		}
	}
}

function inProtoDevList(res, _id, _idn, _po)
{
	if(PROTO_DEV_RTU_SELF==Number(_id)) return true;
	if(PROTO_DEV_RTU_SELF_MID==Number(_id)) return true;
	if( res.rs != null ) {
		for( var n = 0; n < res.rs.length; n++ ) {
			if(res.rs[n].id == _id && res.rs[n].po == _po) return true;
		}
	}
	if( res.net != null ) {
		for( var n = 0; n < res.net.length; n++ ) {
			if(res.net[n].id == _id && res.net[n].idn == _idn && res.net[n].po == _po) return true;
		}
	}
	if( res.zigbee != null ) {
		for( var n = 0; n < res.zigbee.length; n++ ) {
			if(res.zigbee[n].id == _id && res.zigbee[n].po == _po) return true;
		}
	}
	if( res.lora != null ) {
		for( var n = 0; n < res.lora.length; n++ ) {
			if(res.lora[n].id == _id && res.lora[n].po == _po) return true;
		}
	}
	if( res.gprs != null ) {
		for( var n = 0; n < res.gprs.length; n++ ) {
			if(res.gprs[n].id == _id && res.gprs[n].po == _po) return true;
		}
	}
}

function refreshProtoDevList(res, devtypeid, prodevid)
{
	var protoDevList = window.document.getElementById(devtypeid);
	var devNameList = window.document.getElementById(prodevid);
	if( protoDevList != null ) protoDevList.options.length = 0;
	if( devNameList != null ) devNameList.options.length = 0;
	if( res.rs != null ) {
		/*for( var n = 0; n < 8; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( PROTO_DEV_RTU_SELF, n, 0 ), 
						PROTO_DEV_RTU_SELF + "|" + n + "|" + 0
					)
				);
			}
		}*/
		if( protoDevList != null ) {
			protoDevList.options.add(
				new Option(
					varExtGetProtoName( PROTO_DEV_RTU_SELF_MID, 0, 0 ), 
					PROTO_DEV_RTU_SELF_MID + "|0|0"
				)
			);
		}
		for( var n = 0; n < res.rs.length; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( res.rs[n].id, res.rs[n].idn, res.rs[n].po ), 
						res.rs[n].id + "|" + res.rs[n].idn + "|" + res.rs[n].po
					)
				);
			}
			if( devNameList != null ) {
				devNameList.options.add(
					new Option(
						varExtGetProtoDev( res.rs[n].id, res.rs[n].idn ), 
						res.rs[n].id + "|" + res.rs[n].idn
					)
				);
			}
		}
	}
	if( res.net != null ) {
		for( var n = 0; n < res.net.length; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( res.net[n].id, res.net[n].idn, res.net[n].po ), 
						res.net[n].id + "|" + res.net[n].idn + "|" + res.net[n].po
					)
				);
			}
			if( devNameList != null ) {
				devNameList.options.add(
					new Option(
						varExtGetProtoDev( res.net[n].id, res.net[n].idn ), 
						res.net[n].id + "|" + res.net[n].idn
					)
				);
			}
		}
	}
	if( res.zigbee != null ) {
		for( var n = 0; n < res.zigbee.length; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( res.zigbee[n].id, res.zigbee[n].idn, res.zigbee[n].po), 
						res.zigbee[n].id + "|" + res.zigbee[n].idn + "|" + res.zigbee[n].po
					)
				);
			}
			if( devNameList != null ) {
				devNameList.options.add(
					new Option(
						varExtGetProtoDev( res.zigbee[n].id, res.zigbee[n].idn ), 
						res.zigbee[n].id + "|" + res.zigbee[n].idn
					)
				);
			}
		}
	}
	if( res.lora != null ) {
		for( var n = 0; n < res.lora.length; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( res.lora[n].id, res.lora[n].idn, res.lora[n].po), 
						res.lora[n].id + "|" + res.lora[n].idn + "|" + res.lora[n].po
					)
				);
			}
			if( devNameList != null ) {
				devNameList.options.add(
					new Option(
						varExtGetProtoDev( res.lora[n].id, res.lora[n].idn ), 
						res.lora[n].id + "|" + res.lora[n].idn
					)
				);
			}
		}
	}
	if( res.gprs != null ) {
		
		if ("upload_data_pro_dev" == prodevid) {
			for( var n = 0; n < res.gprs.length; n++ ) {
				if( protoDevList != null ) {
					protoDevList.options.add(
						new Option(
							varExtGetProtoName( res.gprs[n].id, res.gprs[n].idn, res.gprs[n].po), 
							res.gprs[n].id + "|" + res.gprs[n].idn + "|" + res.gprs[n].po
						)
					);
				}
				if( devNameList != null ) {
					devNameList.options.add(
						new Option(
							varExtGetProtoDev( res.gprs[n].id, res.gprs[n].idn) , 
							res.gprs[n].id + "|" + res.gprs[n].idn
						)
					);
				}
			}
		} else {
			for( var n = 0; n < res.gprs.length; n++ ) {
				if( protoDevList != null ) {
					protoDevList.options.add(
						new Option(
							varExtGetProtoName( res.gprs[n].id, res.gprs[n].idn, res.gprs[n].po), 
							res.gprs[n].id + "|" + res.gprs[n].idn + "|" + res.gprs[n].po
						)
					);
				}
				if( devNameList != null ) {
					devNameList.options.add(
						new Option(
							varExtGetProtoDev( res.gprs[n].id, res.gprs[n].idn ), 
							res.gprs[n].id + "|" + res.gprs[n].idn
						)
					);
				}
			}
		}
	}
}

function refreshSearchDevList_modbus(res, dev_list_id)
{
	var dev_list = window.document.getElementById(dev_list_id);
	if( dev_list != null ) dev_list.options.length = 0;
	if( res.rs != null ) {
		for( var n = 0; n < res.rs.length; n++ ) {
			if( dev_list != null && __proto_is_modbus(res.rs[n].id, res.rs[n].po)) {
				dev_list.options.add(
					new Option(
						varExtGetProtoDev( res.rs[n].id, res.rs[n].idn ), 
						res.rs[n].id + "|" + res.rs[n].idn
					)
				);
			}
		}
	}
	if( res.lora != null ) {
		for( var n = 0; n < res.lora.length; n++ ) {
			if( dev_list != null && __proto_is_modbus(res.lora[n].id, res.lora[n].po)) {
				dev_list.options.add(
					new Option(
						varExtGetProtoDev( res.lora[n].id, res.lora[n].idn ), 
						res.lora[n].id + "|" + res.lora[n].idn
					)
				);
			}
		}
	}
}

function refreshImportDevList_modbus(res, dev_list_id)
{
	var dev_list = window.document.getElementById(dev_list_id);
	if( dev_list != null ) dev_list.options.length = 0;
	if( res.rs != null ) {
		for( var n = 0; n < res.rs.length; n++ ) {
			if (dev_list != null && __proto_is_modbus(res.rs[n].id, res.rs[n].po)) {
				dev_list.options.add(
					new Option(
						varExtGetProtoDev( res.rs[n].id, res.rs[n].idn ), 
						res.rs[n].id + "|" + res.rs[n].idn
					)
				);
			}
		}
	}
	if( res.net != null ) {
		for( var n = 0; n < res.net.length; n++ ) {
			if (dev_list != null && __proto_is_modbus(res.net[n].id, res.net[n].po)) {
				dev_list.options.add(
					new Option(
						varExtGetProtoDev( res.net[n].id, res.net[n].idn ), 
						res.net[n].id + "|" + res.net[n].idn
					)
				);
			}
		}
	}
	if( res.zigbee != null ) {
		for( var n = 0; n < res.zigbee.length; n++ ) {
			if (dev_list != null && __proto_is_modbus(res.zigbee[n].id, res.zigbee[n].po)) {
				dev_list.options.add(
					new Option(
						varExtGetProtoDev( res.zigbee[n].id, res.zigbee[n].idn ), 
						res.zigbee[n].id + "|" + res.zigbee[n].idn
					)
				);
			}
		}
	}
	if( res.lora != null ) {
		for( var n = 0; n < res.lora.length; n++ ) {
			if (dev_list != null && __proto_is_modbus(res.lora[n].id, res.lora[n].po)) {
				dev_list.options.add(
					new Option(
						varExtGetProtoDev( res.lora[n].id, res.lora[n].idn ), 
						res.lora[n].id + "|" + res.lora[n].idn
					)
				);
			}
		}
	}
	if( res.gprs != null ) {
		for( var n = 0; n < res.gprs.length; n++ ) {
			if (dev_list != null && __proto_is_modbus(res.gprs[n].id, res.gprs[n].po)) {
				dev_list.options.add(
					new Option(
						varExtGetProtoDev( res.gprs[n].id, res.gprs[n].idn ), 
						res.gprs[n].id + "|" + res.gprs[n].idn
					)
				);
			}
		}
	}
}

function refreshImportDevProtoList_modbus(res, dev_list_id, proto_list_id)
{
	var proto_list = window.document.getElementById(proto_list_id);
	var n = 0;
	var _dev = getValue(dev_list_id).split("|");
	if( _dev.length == 2) {
		var dev = Number(_dev[0]);
		var dev_n = Number(_dev[1]);
		proto_list.options.length = 0;
		if( res.rs != null ) {
			for( var n = 0; n < res.rs.length; n++ ) {
				if (dev == res.rs[n].id && __proto_is_modbus(res.rs[n].id, res.rs[n].po)) {
					proto_list.options.add( 
						new Option( varExtGetProto(res.rs[n].id, res.rs[n].po), ""+res.rs[n].po ) 
					);
				}
			}
		}
		if( res.net != null ) {
			for (var n = 0; n < res.net.length; n++ ) {
				if( dev == res.net[n].id && dev_n == res.net[n].idn && __proto_is_modbus(res.net[n].id, res.net[n].po)) {
					proto_list.options.add( 
						new Option( varExtGetProto(res.net[n].id, res.net[n].po), ""+res.net[n].po ) 
					);
				}
			}
		}
		if( res.zigbee != null ) {
			for (var n = 0; n < res.zigbee.length; n++ ) {
				if( dev == res.zigbee[n].id && __proto_is_modbus(res.zigbee[n].id, res.zigbee[n].po)) {
					proto_list.options.add( 
						new Option( varExtGetProto(res.zigbee[n].id, res.zigbee[n].po), ""+res.zigbee[n].po ) 
					);
				}
			}
		}
		if( res.lora != null ) {
			for (var n = 0; n < res.lora.length; n++ ) {
				if( dev == res.lora[n].id && __proto_is_modbus(res.lora[n].id, res.lora[n].po)) {
					proto_list.options.add( 
						new Option( varExtGetProto(res.lora[n].id, res.lora[n].po), ""+res.lora[n].po ) 
					);
				}
			}
		}
		if( res.gprs != null ) {
			for( var n = 0; n < res.gprs.length; n++ ) {
				if (dev == res.gprs[n].id && dev_n == res.gprs[n].idn && __proto_is_modbus(res.gprs[n].id, res.gprs[n].po)) {
					proto_list.options.add( 
						new Option( varExtGetProto(res.gprs[n].id, res.gprs[n].po), ""+res.gprs[n].po ) 
					);
				}
			}
		}
	}
}

function devTimeSync()
{
	var myDate = new Date();
	var setval = {
		ye:myDate.getFullYear(), 
		mo:(myDate.getMonth()+1), 
		da:myDate.getDate(), 
		dh:myDate.getHours(), 
		hm:myDate.getMinutes(), 
		ms:myDate.getSeconds()
	};
	
	MyGetJSONWithArg("正在对时,请稍后...","/cgi-bin/setTime?", JSON.stringify(setval));
}

//======================================================页面js函数==================================================
//每次选择菜单时，都会填写【功能名称】，待调用ak47时使用；
function bullet(name)
{
	document.getElementById("txt_refresh_name").value = name;
}

function bullet_clear()
{
	document.getElementById("txt_refresh_name").value = "";
}

function bullet_add(name) 
{
	document.getElementById("txt_refresh_name").value = name;
}


function msg()
{
	alert('应用成功！');
}

function chk(obj,name)
{
	if (obj == "" || obj == "undefined")
	{
		alert("【" + name + "】不能为空！");
		return true;
	}
	else
	{
		return false;
	}
}

function chk2(obj1, obj2, name)
{
	if ((obj1 == "" || obj1 == "undefined") && (obj2 == "" || obj2 == "undefined"))
	{
		alert("【" + name + "】不能为空！");
		return true;
	}
	else
	{
		return false;
	}
}

var v = 0;
var bakname = "";
//自动刷新时，调用该函数；
function ak47()
{
	var name = document.getElementById("txt_refresh_name").value;
	/*if( bakname != name ) {
		bakname = name;
		bullet_clear();
	}*/
	switch (name)
	{
		case "1001":
			{
				//设备基本信息
				Show_Basic_Information();
			}
			break;
		case "1002":
			{
				//系统资源
				Show_System_Resource();
			}
			break;
		case "1003":
			{
				Show_All_NetInfo();
			}
			break;
		case "1004":
			{
				//已连接LoRa无线节点
				Show_LoRa_Information();
			}
			break;
		case "1005":
			{
				//基本设置
				Show_Time_Synchronization();
				Show_Storage_Cfg();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1006":
			{
				//日志
				Show_Log();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "10016":
			{
				
				show_dev_logs_list();
				monitor_get_cfg();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1007":
			{
				//管理权限
				Show_Jurisdiction();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1008":
			{
				//远程传输设置
				Show_Remote_Transmission();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1009":
			{
				//本地网络配置
				Show_Local_Network_Configuration();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1010":
			{
				//LoRa配置
				Show_LoRa();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1012":
			{
				//工作模式及中心配置
				Show_GPRS_Transmission();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1013":
			{
				//GPRS工作参数配置
				Show_GPRS_Config();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1014":
			{
				//无线参数
				Show_GPRS_Wireless();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1015":
			{
				//串口配置
				Show_Serial_Port();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1021":
			{
				//采集变量
				getAllVarManageExtDataBase(0);
				get_all_rules(null);
				//跳转
				bullet_add("9021");
			}
			break;
		case "9021":
			{
				getAllVarManageExtDataVals();
				getVarManageAiValue();
				break;
			}
		case "1022":
			{
				//用户变量
				Show_Defin();
				//仅刷新一次；
				bullet_clear();
			}
			break;

		case "1023":
			{
				//动作
			}
			break;
			
		case "1024":
			{
				Show_Xfer_Net_Cfg();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1029":
			{
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1030"://上传数据配置
			{
				Show_upload_cfg();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1039":
			{
				get_all_rules();
				//仅刷新一次；
				bullet_clear();
			}
		case "1050"://网络适配器配置
			{
				Show_Network_Information();
				//仅刷新一次；
				bullet_clear();
			}
			break;
						
		default: break;
	}

}

function goreg()
{
	window.location.href="http://"+window.location.host+"/reg.html";
}

//设备基本信息
function Show_Basic_Information()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("","/cgi-bin/getDevInfo?","", function (info)
	{
		Page_Basic_Information(info)
	});
}

function Page_Basic_Information( info )
{	
	if( info.reg == 1 ) {
		window.document.getElementById("rtu_reg_status").innerHTML = "已激活";
		setVisible('btn_goreg', false);
	} else {
		var t = info.tta-info.ttt;
		if( t > 0 ) {
			var d = parseInt(t/(24*60*60));
			t-=(24*60*60*d);
			var h = parseInt(t/(60*60));
			t-=60*60*h;
			var m = parseInt(t/60);
			window.document.getElementById("rtu_reg_status").innerHTML = "试用剩余时间 " + d + "天" + h + "时" + m + "分";
		} else {
			window.document.getElementById("rtu_reg_status").innerHTML = "试用结束，请激活！";
		}
		setVisible('btn_goreg', true);
	}
	//设备ID
	document.getElementById("rtu_dev_id").innerHTML = info.id;
	//设备序列号
	document.getElementById("rtu_dev_sn").innerHTML = info.sn;
	//硬件版本
	document.getElementById("rtu_dev_hw_ver").innerHTML = info.hw;
	//固件版本
	document.getElementById("rtu_dev_sw_ver").innerHTML = info.sw;
	//产品序列号
	document.getElementById("rtu_dev_oem").innerHTML = info.om;
	//LoRa 固件版本
	document.getElementById("rtu_dev_lora_ver").innerHTML = info.loraver;
	//MAC 地址
	document.getElementById("rtu_dev_mac").innerHTML = info.mc;
	//系统时间
	document.getElementById("rtu_dev_time").innerHTML = info.dt;
	//系统信息
	document.getElementById("rtu_dev_desc").innerHTML = info.desc;
	//模块信息
	document.getElementById("rtu_das_module_desc").innerHTML = info.das_desc;
	//运行时间
	document.getElementById("rtu_run_time").innerHTML = info.rt;
	
	var date = new Date(info.dt);
	var nowdate = new Date();
	var datediff = nowdate.getTime()-date.getTime();
	if( datediff > 10000 ) {
		devTimeSync();
	}
}

//系统资源
function Show_System_Resource()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_System_Resource("注：此处传入一个对象");
	MyGetJSONWithArg("","/cgi-bin/getCpuUsage?","", function (info)
	{
		Page_System_Resource(info)
	});
}

//cpu使用率和内存占用有% 可用进度条 内存剩余，存储ROW，SD卡没有进度条
function Page_System_Resource(info)
{
	//CPU使用率
	document.getElementById("sys_cpu_rate").innerHTML = Number(info.cpu) + "%";
	$("#sys_cpu_rate").css('width',Number(info.cpu) + "%");
	//内存占用
	if( Number(info.ms) > 0 ) {
		document.getElementById("sys_memory_use").innerHTML = Math.round(100*Number(info.mu)/Number(info.ms)) + "%";
		$("#sys_memory_use").css('width',Math.round(100*Number(info.mu)/Number(info.ms)) + "%");
	} else {
		document.getElementById("sys_memory_use").innerHTML = "--";
		$("#sys_memory_use").css('width',"0%");
	}
	//内存剩余
	document.getElementById("sys_memory_free").innerHTML = "Free : " + 
				((Number(info.ms)-Number(info.mu))/1024).toFixed(2) + " MB  [Total : " + 
				(Number(info.ms)/1024).toFixed(2) + " MB, Used : " +  
				(Number(info.mu)/1024).toFixed(2) + " MB, Max Used : " +  (Number(info.ma)/1024).toFixed(2) + " MB]";

	//存储ROM
	document.getElementById("sys_store_rom").innerHTML = ((Number(info.fs)-Number(info.fu))/1024).toFixed(2) + " MB";
}

//网络信息
function Show_Network_Information()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("","/cgi-bin/getNetInfo?","", function (info)
	{
		Page_Network_Information(info)
	});
}

function Page_Network_Information(info)
{
	var prefix = 'sys_net';
	//类型: dhcp
	document.getElementById(prefix + '_dhcp').innerHTML = Number(info.dh)!=0?"开":"关";
	//MAC地址
	document.getElementById(prefix + '_mac').innerHTML = info.mac;
	//地址
	document.getElementById(prefix + '_address').innerHTML = info.ip;
	//给调试监听添加IP地址
	//$('#txtWebSocketAddress').val(info.ip);
	//子网掩码
	document.getElementById(prefix + '_mask').innerHTML = info.mask;
	//网关
	document.getElementById(prefix + '_gate').innerHTML = info.gw;
	//DNS 1
	document.getElementById(prefix + '_dns_1').innerHTML = info.d1;
	//DNS 2
	document.getElementById(prefix + '_dns_2').innerHTML = info.d2;
	//已连接
	document.getElementById(prefix + '_link').innerHTML = info.lk;
	
	if (!info.link) {
		setDisplay('net_no_link', true);
		setEnable('net_adapter', false);
		setValue('net_adapter', '1');
		document.getElementById(prefix + '_link').innerHTML = "网线未连接/异常";
	} else {
		setDisplay('net_no_link', false);
		setEnable('net_adapter', true);
		setValue('net_adapter', info.adpt);
	}
	document.getElementById(prefix + '_info_title').innerHTML = "有线网络" + (info.adpt == NET_ADAPTER_WIRED ? "【当前网络】" : "");

}

//已连接LoRa无线节点
function Show_LoRa_Information()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("","/cgi-bin/getLoRaList?","", function (info)
	{
		Page_LoRa_Information(info)
	});
}

function loraNodeMode(mode)
{
	if( mode != null ) {
		switch(Number(mode)) {
		case 0:
			return "终端";
		case 1:
			return "中心节点";
		}
	}
	return "未知";
}

function loraNodeTableAddItem(mode,type0,type1,id,netid,on,rssi,rssi_db,snr,adlst,upt,offt)
{
	var index = 0;
	var table = window.document.getElementById("lora_node_table");
	var row = table.insertRow(table.rows.length);
	var obj = row.insertCell(index++);
	obj.innerHTML = loraNodeMode(mode);
	obj = row.insertCell(index++);
	obj.innerHTML = var_ext_dev_type0_format(type0);
	obj = row.insertCell(index++);
	obj.innerHTML = id;
	obj = row.insertCell(index++);
	obj.innerHTML = netid;
	obj = row.insertCell(index++);
	obj.innerHTML = Number(on)!=0?"在线":"下线";
	obj = row.insertCell(index++);
	if(Number(on)!=0 && rssi!=null && Number(rssi)!=0) {
		if (rssi_db == INVALID_RSSI) {
			obj.innerHTML = Number(rssi) + "%";
		} else {
			obj.innerHTML = Number(rssi) + "% (" + snr + "," + rssi_db + "dB)";
		}
	} else {
		obj.innerHTML = "未知";
	}
	obj = row.insertCell(index++);
	obj.innerHTML = adlst;
	obj = row.insertCell(index++);
	obj.innerHTML = upt;
	obj = row.insertCell(index++);
	obj.innerHTML = offt;
}

function loraNodeTableItemRemoveAll()
{
	var table = window.document.getElementById("lora_node_table");
	var rowNum = table.rows.length;
	if(rowNum > 0) {
		for(i=0;i<rowNum;i++) {
			table.deleteRow(i);
			rowNum = rowNum-1;
			i = i-1;
		}
	}
}

function Page_LoRa_Information(info)
{
	if(info.list != null ) {
		loraNodeTableItemRemoveAll();
		for( var n = 0; n < info.list.length; n++ ) {
			loraNodeTableAddItem( 
				info.list[n].mode, 
				info.list[n].type0, 
				info.list[n].type1, 
				info.list[n].id, 
				info.list[n].netid, 
				info.list[n].on, 
				info.list[n].rssi, 
				info.list[n].rssi_db, 
				info.list[n].snr, 
				info.list[n].adlst, 
				info.list[n].upt, 
				info.list[n].offt
			);
		}
	}
}

//GPRS网络信息
function Show_gprs_network()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_gprs_network("")
	MyGetJSONWithArg("","/cgi-bin/getGPRSState?","", function (info)
	{
		Page_gprs_network(info);
	});
}

function Page_gprs_network(info)
{
	var reg = (info.creg!=null?Number(info.creg):-1);
	var _pwr = (info.pwr!=null?(Number(info.pwr)!=0):1);
	//开机状态
	document.getElementById("gprs_network_pwr").innerHTML = _pwr?"开机":"关机";
	if(_pwr) {
		//运营商名称
		document.getElementById("gprs_network_name").innerHTML = info.alphan;
		document.getElementById("gprs_network_type").innerHTML = GPRS_OR_NBIOT;
		document.getElementById("gprs_network_ip").innerHTML = info.ip;
		//网络注册状态
		if( reg <= 0 || reg == 2 || reg == 3 || reg == 4 ) {
			document.getElementById("gprs_network_creg").innerHTML = "未注册";
		} else if( reg == 1 ) {
			document.getElementById("gprs_network_creg").innerHTML = "本地网络";
		} else if( reg == 5 ) {
			document.getElementById("gprs_network_creg").innerHTML = "漫游网络";
		} else if( reg == 8 ) {
			document.getElementById("gprs_network_creg").innerHTML = "紧急呼叫";
		} else {
			document.getElementById("gprs_network_creg").innerHTML = "未知状态("+reg+")";
		}
		//信号质量
		document.getElementById("gprs_network_csq").innerHTML = info.csq;
		//小区信息
		//document.getElementById("gprs_network_area").innerHTML = info.area;
		
		document.getElementById("gprs_net_info_title").innerHTML = GPRS_OR_NBIOT + "网络" + (info.adpt == NET_ADAPTER_WIRELESS ? "【当前网络】" : "");
	} else {
		document.getElementById("gprs_network_name").innerHTML = "--";
		document.getElementById("gprs_network_type").innerHTML = "--";
		document.getElementById("gprs_network_ip").innerHTML = "--";
		document.getElementById("gprs_network_creg").innerHTML = "--";
		document.getElementById("gprs_network_csq").innerHTML = "--";
		//document.getElementById("gprs_network_area").innerHTML = "--";
		document.getElementById("gprs_net_info_title").innerHTML = GPRS_OR_NBIOT + "网络(关机)";
	}
}

function Show_All_NetInfo()
{
	MyGetJSONWithArg("","/cgi-bin/getAllNetInfo?","", function (info)
	{
		if( info.tcpipinfo != null ) {
			Page_Root_GPRS(info.tcpipinfo);
			Page_Root_Network(info.tcpipinfo);
		}
		if( info.gprsinfo != null ) {
			Page_gprs_network(info.gprsinfo);
		}
		if( info.netinfo != null ) {
			Page_Network_Information(info.netinfo);
		}
	});
}

//GPRS 与 以太网 连接状态
function Show_Root_GPRS_Network()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Root_GPRS("12345");
	MyGetJSONWithArg("","/cgi-bin/getTcpipState?","", function (info)
	{
		Page_Root_GPRS(info.states);
		Page_Root_Network(info.states);
	});
}

function Page_Root_GPRS(obj)
{
	var str = "<table style='width:96%' class='list_info_table'>";
	str += "<tr><th>序号</th><th>组号</th><th>远程IP</th><th>远程端口</th><th>本地IP</th><th>本地端口</th><th>状态</th><th>已连接:（时长）</th></tr>";

	try
	{
		var index = 1;
		for (var n = ENET_TCPIP_NUM; n < ENET_TCPIP_NUM + GPRS_TCPIP_NUM; n++)
		{
			var m = obj[n].list.length;
			for(var k = 0; k < m; k++){
			    var item = obj[n].list[k];
				str += "<tr>";
				str += "<td>" + index + "</td>";
				str += "<td>" + ((n - ENET_TCPIP_NUM) + 1) + "组</td>";
				if (item.st == 2) {
					str += "<td>监听所有地址</td>";
					str += "<td>--</td>";
				} else {
					str += "<td>" + item.rip + "</td>";
					str += "<td>" + item.rpt + "</td>";
				}
				str += "<td>" + item.lip + "</td>";
				str += "<td>" + item.lpt + "</td>";
				switch(item.st) {
				case 0:
					str += "<td>等待</td>";
					break;
				case 1:
					str += "<td>已连接</td>";
					break;
				case 2:
					str += "<td>监听中</td>";
					break;
				case 3:
					str += "<td>正在连接</td>";
					break;
				}
				if( 1 == item.st ) {
					str += "<td>" + (item.tn-item.tc) + " 秒</td>";
				} else {
					str += "<td>--</td>";
				}
				str += "</tr>";
				index++;
			}
		}
	}
	catch (e)
	{
	}

	str += "</table>";

	document.getElementById("div_gprs").innerHTML = str;

}


//GPRS连接状态
//function Show_Root_Network()
//{
//	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
//	Page_Root_Network("12345");
//}

function Page_Root_Network(obj)
{
	var str = "<table style='width:96%' class='list_info_table'>";
	str += "<tr><th style=''>序号</th><th style=''>组号</th><th style=''>远程IP</th><th style=''>远程端口</th><th style=''>本地IP</th><th style=''>本地端口</th><th >状态</th><th>已连接:（时长）</td></tr>";

	try
	{
		var index = 1;
		for (var n = 0; n < ENET_TCPIP_NUM; n++)
		{
			var m = obj[n].list.length;
			for(var k = 0; k < m; k++){
			    var item = obj[n].list[k];
				str += "<tr>";
				str += "<td>" + index + "</td>";
				str += "<td>" + (n + 1) + "组</td>";
				if (item.st == 2) {
					str += "<td>监听所有地址</td>";
					str += "<td>--</td>";
				} else {
					str += "<td>" + item.rip + "</td>";
					str += "<td>" + item.rpt + "</td>";
				}
				str += "<td>" + item.lip + "</td>";
				str += "<td>" + item.lpt + "</td>";
				switch(item.st) {
				case 0:
					str += "<td>等待</td>";
					break;
				case 1:
					str += "<td>已连接</td>";
					break;
				case 2:
					str += "<td>监听中</td>";
					break;
				case 3:
					str += "<td>正在连接</td>";
					break;
				}
				if( 1 == item.st ) {
					str += "<td>" + (item.tn-item.tc) + " 秒</td>";
				} else {
					str += "<td>--</td>";
				}
				str += "</tr>";
				index++;
			}
		}
	}
	catch (e)
	{
	}

	str += "</table>";

	document.getElementById("div_network").innerHTML = str;

}

//时间同步
function Show_Time_Synchronization()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；	
	MyGetJSONWithArg("正在获取设备基本配置","/cgi-bin/getHostCfg?","",function( cfg ){
		Page_Time_Synchronization(cfg)
	});
}

function Page_Time_Synchronization(cfg)
{
	//主机名
	setValue('base_hostname', cfg.host);
	//设备ID
	setValue('base_devid', cfg.id);
	//设备时区
	setValue('base_timezone', cfg.tz);
	//时间同步
	setCheckBoxEnable("base_time", Number(cfg.sync)!=0);
	//候选NTP服务器1
	setValue('base_ntp1', cfg.ntp1);
	//候选NTP服务器2
	setValue('base_ntp2', cfg.ntp2);
	
	//setCheckBoxEnable("base_debug", (cfg.dbg != null) && (Number(cfg.dbg)!=0));
}

function Apply_Time_Synchronization()
{
	//主机名
	var base_hostname = document.getElementById("base_hostname").value;
	//设备ID
	var base_devid = document.getElementById("base_devid").value;
	//设备ID
	var base_timezone = document.getElementById("base_timezone").value;
	//时间同步
	var base_time = getChecked("base_time");
	//候选NTP服务器1
	var base_ntp1 = document.getElementById("base_ntp1").value;
	//候选NTP服务器2
	var base_ntp2 = document.getElementById("base_ntp2").value;
	
	//var base_debug = getChecked("base_debug");

	if (chk(base_hostname, "主机名")) return;
	//if (chk(base_devid, "设备ID")) return;
	if (chk(base_timezone, "时区设置")) return;
	//if (chk(base_ntp1, "候选NTP服务器1")) return;

	//注：在此处填写保存代码即可；
	var setval = {
		host:base_hostname, 
		id:base_devid, 
		tz:Number(base_timezone), 
		sync:base_time?1:0, 
		ntp1:base_ntp1, 
		ntp2:base_ntp2
	};
	
	MyGetJSONWithArg("正在设置设备基本配置","/cgi-bin/setHostCfg?", JSON.stringify(setval));
}


//历史数据保存
function Show_Storage_Cfg()
{
	MyGetJSONWithArg("正在获取历史数据保存配置","/cgi-bin/getStorageCfg?","",function( cfg ){
		Page_Storage_Cfg(cfg)
	});
}

function Page_Storage_Cfg(cfg)
{
	//是否存储
	setCheckBoxEnable("storage_en",Number(cfg.se)!=0);
	//存储间隔
	setValue('storage_step', cfg.ss);
	//存储位置
	setValue('storage_path', cfg.path);
}

//历史数据保存配置
function Apply_Storage_Cfg()
{
	//是否存储
	var storage_en = getChecked("storage_en");
	//存储间隔
	var storage_step = getValue("storage_step");
	//存储位置
	var storage_path = getValue("storage_path");

	if (chk(storage_step, "保存间隔")) return;
	if (chk(storage_path, "保存路径")) return;

		//注：在此处填写保存代码即可；
	var setval = {
		se:storage_en?1:0, 
		ss:Number(storage_step), 
		path:Number(storage_path)
	};
	
	MyGetJSONWithArg("正在设置历史数据保存配置","/cgi-bin/setStorageCfg?",JSON.stringify(setval),function( rsp ){
		if( rsp != null && rsp.ret != null && rsp.ret != 0 ) {
			alert( "配置失败,请检查SD卡是否插入正常" );
		}
	});
}

//日志
function Show_Log()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Log("注：此处传入一个对象");
}

function Page_Log(obj)
{

	//系统日志缓冲区大小 
	//setValue('log_size', obj.null);
	//chenqq
	setValue('log_size', null);
	//远程log服务器 
	//setValue('log_remote', obj.null);
	setValue('log_remote', null);
	//远程log服务器端口
	//setValue('log_port', obj.null);
	setValue('log_port', null);

	//日志记录等级
	var tmp_log_level = document.getElementById("log_level");
	for (var i = 0; i < tmp_log_level.options.length; i++)
	{
		if (tmp_log_level.options[i].value == "此处填写对象.值")
		{
			tmp_log_level.selectedIndex = i;
			break;
		}
	}

	//Cron日志级别 
	var log_cron = document.getElementById("log_cron");
	for (var i = 0; i < log_cron.options.length; i++)
	{
		if (log_cron.options[i].value == "此处填写对象.值")
		{
			log_cron.selectedIndex = i;
			break;
		}
	}
}

function Apply_Log()
{
	//系统日志缓冲区大小
	var log_size = document.getElementById("log_size").value;
	//远程log服务器 
	var log_remote = document.getElementById("log_remote").value;
	//远程log服务器端口
	var log_port = document.getElementById("log_port").value;
	//日志记录等级
	var v = document.getElementById("log_level").selectedIndex;
	var log_level_ = document.getElementById("log_level").options[v].text;

	//Cron日志级别
	v = document.getElementById("log_cron").selectedIndex;
	var log_cron_ = document.getElementById("log_cron").options[v].text;


	if (chk(log_size, "系统日志缓冲区大小")) return;
	if (chk(log_remote, "远程log服务器")) return;
	if (chk(log_port, "远程log服务器端口")) return;
	if (chk(log_level_, "日志记录等级")) return;
	if (chk(log_cron_, "Cron日志级别")) return;

	//注：在此处填写保存代码即可；
	msg();
}


//管理权限
function Show_Jurisdiction()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("正在获取管理者信息","/cgi-bin/getAuthCfg?","",function( info ){
		Page_Jurisdiction(info)
	});
}

function Page_Jurisdiction(info)
{
	//用户名 
	setValue('manage_user', info.user);
	//密码
	//setValue('manage_pwd', info.psk);
	//SSH访问
	document.getElementsByName("manage_ssh")[0].checked = (Number(info.ssh)!=0);//此处填写值；
	document.getElementsByName("manage_ssh")[1].checked = !(Number(info.ssh)!=0);//此处填写值；
	//端口
	setValue('manage_port', info.sp);
	//允许SSH密码验证
	document.getElementsByName("manage_ssh_cer")[0].checked = (Number(info.sshcer)!=0);;//此处填写值；
	document.getElementsByName("manage_ssh_cer")[1].checked = !(Number(info.sshcer)!=0);//此处填写值；
	//时间设置 
	setValue('manage_time', info.t);
}


function Apply_Jurisdiction()
{
	//用户名
	var manage_user = document.getElementById("manage_user").value;
	//密码 
	var manage_pwd = document.getElementById("manage_pwd").value;
	var manage_pwd_1 = document.getElementById("manage_pwd_1").value;
	//SSH访问
	var manage_ssh1 = document.getElementsByName("manage_ssh")[0].checked;
	var manage_ssh2 = document.getElementsByName("manage_ssh")[1].checked;
	//端口
	var manage_port = document.getElementById("manage_port").value;
	//允许SSH密码验证
	var manage_ssh_cer1 = document.getElementsByName("manage_ssh_cer")[0].checked;
	var manage_ssh_cer2 = document.getElementsByName("manage_ssh_cer")[1].checked;
	//时间设置
	var manage_time = document.getElementById("manage_time").value;


	if (chk(manage_user, "用户名")) return;
	if (chk(manage_pwd, "密码")) return;
	if (chk(manage_pwd_1, "确认密码")) return;
	if (manage_pwd != manage_pwd_1) {
		alert("密码与确认密码不一致！");
		return;
	}
	if (chk2(manage_ssh1, manage_ssh2, "SSH访问")) return;
	if (chk(manage_port, "端口")) return;
	if (chk2(manage_ssh_cer1, manage_ssh_cer2, "允许SSH密码验证")) return;
	if (chk(manage_time, "时间设置")) return;

	//注：在此处填写保存代码即可；
	var setval = {
		user:manage_user, 
		psk:manage_pwd, 
		ssh:manage_ssh1?1:0, 
		sp:Number(manage_port), 
		sshcer:manage_ssh_cer1?1:0,
		t:Number(manage_time)
	};
	
	MyGetJSONWithArg("正在设置管理者信息","/cgi-bin/setAuthCfg?",JSON.stringify(setval),function( info ){
		if( info.ret != null && info.ret == 0 ) {
			alert("修改成功,需要重新登录!");
			window.location.href="http://"+window.location.host+"/login.html";
		}
	});
}


function Show_Local_Setup()
{
	var v = Number(document.getElementById("local_dhcp").value);
	document.getElementById("local_ip").disabled = (v>0);
	document.getElementById("local_mask").disabled = (v>0);
	document.getElementById("local_gateway").disabled = (v>0);
	document.getElementById("local_dns").disabled = (v>0);
}

//远程传输设置
function Show_Remote_Transmission()
{
	var v = document.getElementById("remote_group").selectedIndex;
	var setval = { n:Number(v) };
	MyGetJSONWithArg("正在获取以太网 TCP/IP配置","/cgi-bin/getTcpipCfg?",JSON.stringify(setval),function( info ){
		Page_Remote_Transmission(info)
	});
}

function Enable_Remote_Transmission()
{
	var prefix = 'remote';
	var v = !getChecked(prefix + '_group_is');

	document.getElementById(prefix + '_socket_type').disabled = v;
	document.getElementById(prefix + '_working_type').disabled = v;
	document.getElementById(prefix + '_working_cs').disabled = v;
	document.getElementById(prefix + '_proto').disabled = v;
	document.getElementById(prefix + '_proto_ms').disabled = v;

	document.getElementById(prefix + '_ip').disabled = v;
	document.getElementById(prefix + '_port').disabled = v;
	document.getElementById(prefix + '_upload_interval').disabled = v;
	document.getElementById(prefix + '_heartbeat').disabled = v;
	
	show_remote_btn();
}

function Page_Remote_Transmission(cfg)
{
	var prefix = 'remote';
	//是否启用 
	setCheckBoxEnable(prefix + '_group_is', Number(cfg.en)!=0);
	//以太网工作模式
	setValue(prefix + '_socket_type', cfg.tt);
	setValue(prefix + '_working_cs', cfg.cs);
	//设备NM号（0-20位）
	//setValue('remote_dev', obj.null);
	//域名地址
	setValue(prefix + '_ip', cfg.pe);
	//域名端口
	setValue(prefix + '_port', cfg.po);
	//上传间隔
	setValue(prefix + '_upload_interval', cfg.it);
	//是否启用心跳包 
	setCheckBoxEnable(prefix + '_heartbeat', Number(cfg.kl)!=0);
	
	setValue(prefix + '_working_type', cfg.md);
	//协议
	if(cfg.md == TCP_IP_M_NORMAL) {
		if( cfg.normal.pt != null ) setValue(prefix + '_proto', cfg.normal.pt);
		if( cfg.normal.ms != null ) setValue(prefix + '_proto_ms', cfg.normal.ms);
		if( cfg.normal.mad != null ) setValue(prefix + '_modbus_addr', cfg.normal.mad);
	} else if(cfg.md == TCP_IP_M_XFER) {
		setValue(prefix + '_xfer_mode', cfg.xfer.md);
		if( cfg.xfer.pt != null ) setValue(prefix + '_wm_xfer_proto_type', cfg.xfer.pt);
		if( cfg.xfer.dt != null ) setValue(prefix + '_xfer_gw_trt_dst_type', cfg.xfer.dt);
		if( cfg.xfer.tidx != null ) setValue(prefix + '_xfer_gw_trt_dst_gprs_index', cfg.xfer.tidx);
		if( cfg.xfer.uty != null ) setValue(prefix + '_xfer_gw_trt_dst_uart_type', cfg.xfer.uty);
		if( cfg.xfer.ubr != null ) setValue(prefix + '_xfer_gw_trt_dst_uart_baudrate', cfg.xfer.ubr);
		if( cfg.xfer.udb != null && cfg.xfer.usb != null  && cfg.xfer.upy != null ) setValue(prefix + '_xfer_gw_trt_dst_uart_bits', (Number(cfg.xfer.udb) * 100) + (Number(cfg.xfer.upy) * 10) + Number(cfg.xfer.usb));
	}
	
	MyGetJSONWithArg("","/cgi-bin/getXferUartCfg?","", function (info)
	{
		Show_xfer_uart_cfg('', info.cfg);
	});

	Enable_Remote_Transmission();
}

function nUartGetIndex(instance)
{
	switch (instance) {
	case 0:
		return PROTO_DEV_RS1;
		break;
	case 1:
		return PROTO_DEV_RS2;
		break;
	case 2:
		return PROTO_DEV_RS3;
		break;
	case 3:
		return PROTO_DEV_RS4;
		break;
	}

	return -1;
}

function nUartGetInstance(index)
{
	switch (index) {
	case PROTO_DEV_RS1:
		return 0;
		break;
	case PROTO_DEV_RS2:
		return 1;
		break;
	}
	return -1;
}

function Apply_xfer_uart_cfg(_prefix)
{
	var uart_set = {
		cfg:new Array()
	};
	for(var i = PROTO_DEV_RS1; i <= PROTO_DEV_RS_MAX; i++) {
		uart_set.cfg[i] = {'n':i,'en':0,'cnt':0,'addrs':''};
	}
	
	for(var i = PROTO_DEV_RS1; i <= PROTO_DEV_RS_MAX; i++) {
		var _addrs_str = getValue(_prefix+'xfer_uart_cfgs_'+i);
		uart_set.cfg[i] = {
		   'n':i,
		   'en':(_addrs_str.length>0)?1:0, 
		   'cnt':_addrs_str.length>0?_addrs_str.split(",").length:0,
		   'addrs':_addrs_str
		};
	}
	
	MyGetJSONWithArg("正在设置串口地址表","/cgi-bin/setXferUartCfg?",JSON.stringify(uart_set));
}

function Show_xfer_uart_cfg(_prefix, _cfg)
{
	for(var i = 0; i < _cfg.length; i++) {
		var n = _cfg[i]['n'];
		if(n <= PROTO_DEV_RS_MAX) {
			setValue(_prefix+'xfer_uart_cfgs_'+n, _cfg[i].addrs!=null?_cfg[i].addrs:"");
		} else if(PROTO_DEV_ZIGBEE == n) {
			setValue(_prefix+'xfer_uart_cfgs_zgb', _cfg[i].addrs!=null?_cfg[i].addrs:"");
		} /*else if(PROTO_DEV_LORA == n) {
			setValue(_prefix+'xfer_uart_cfgs_lora', _cfg[i].addrs!=null?_cfg[i].addrs:"");
		}*/
	}
}


function Apply_Remote_Transmission_downlist()
{
	var prefix = 'remote';
	//是否启用
	var remote_group_is = getChecked(prefix + '_group_is');
	//以太网工作模式 
	var remote_socket_type = getValue(prefix + '_socket_type');
	var remote_working_type = getValue(prefix + '_working_type');
	var remote_working_cs = getValue(prefix + '_working_cs');

	//设备NM号（0-20位）
	//var remote_dev = getValue("remote_dev");
	//域名地址
	var remote_ip = getValue(prefix + '_ip');
	//域名端口
	var remote_port = getValue(prefix + '_port');
	//上传间隔
	var remote_upload_interval = getValue(prefix + '_upload_interval');
	
	//是否启用心跳包
	var remote_heartbeat = getChecked(prefix + '_heartbeat');
	
	//协议
	var remote_proto = getValue(prefix + '_proto');
	var remote_proto_ms = getValue(prefix + '_proto_ms');
	var remote_modbus_addr = getValue(prefix + '_modbus_addr');
	
	var remote_wm_xfer_proto_type = getValue(prefix + '_wm_xfer_proto_type');
	var remote_xfer_gw_trt_dst_type = getValue(prefix + '_xfer_gw_trt_dst_type');
	var remote_xfer_gw_trt_dst_uart_type = getValue(prefix + '_xfer_gw_trt_dst_uart_type');
	var remote_xfer_gw_trt_dst_uart_baudrate = getValue(prefix + '_xfer_gw_trt_dst_uart_baudrate');
	var remote_xfer_gw_trt_dst_uart_bits = getValue(prefix + '_xfer_gw_trt_dst_uart_bits');
	var remote_xfer_gw_trt_dst_gprs_index = getValue(prefix + '_xfer_gw_trt_dst_gprs_index');

	var md = Number(remote_working_type);
	
	if( remote_group_is ) {
		if (chk2(remote_socket_type, remote_working_cs, "以太网套接字类型")) return;
		if (chk(remote_working_type, "以太网通讯模式")) return;
		if( Number(remote_working_cs) != 1 ) {
			if (chk(remote_ip, "域名地址")) return;
		}
		if (chk(remote_port, "域名端口")) return;
		if (chk(remote_upload_interval, "上传间隔")) return;
		//if (chk(remote_parameter, "上传的数据")) return;

		if(md == TCP_IP_M_NORMAL) {
			if(Number(remote_proto) == 1 ) {
				remote_proto_ms = 0;
			}
			if (chk2(remote_proto, remote_proto_ms, "协议")) return;
			if( Number(remote_proto)==PROTO_MODBUS_RTU_OVER_TCP&&Number(remote_proto_ms)==0) {
				if (chk(remote_modbus_addr, "Modbus 从机地址")) return;
			}
		} else if(md == TCP_IP_M_XFER) {
			var is_gw = getNumber(prefix + '_xfer_mode')== XFER_M_GW;
			var is_trt = getNumber(prefix + '_xfer_mode')== XFER_M_TRT;
			var dst_type = -1;
			if(is_gw||is_trt) {
				dst_type = getNumber(prefix + '_xfer_gw_trt_dst_type');
			}
			if((is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX) {
				//if(chk(remote_xfer_gw_trt_dst_uart_type, "转发端口")) return;
				if(chk(remote_xfer_gw_trt_dst_uart_baudrate, "转发波特率")) return;
				if(chk(remote_xfer_gw_trt_dst_uart_bits, "转发数据位")) return;
			}
			if((is_gw||is_trt)&&dst_type==PROTO_DEV_GPRS) {
				if(chk(remote_xfer_gw_trt_dst_gprs_index, "转发目标GPRS/LTE组号")) return;
			}
			if(is_gw||is_trt) {
				if(chk(remote_xfer_gw_trt_dst_type, "转发端口")) return;
			}
			if(is_gw) {
				if(chk(remote_wm_xfer_proto_type, "转发协议")) return;
			}
		}
	}

	//注：在此处填写保存代码即可；
	var n = document.getElementById(prefix + '_group').selectedIndex;
	
	var setval = {
		n:Number(n), 
		en:remote_group_is?1:0, 
		md:Number(remote_working_type), 
		tt:Number(remote_socket_type), 
		cs:Number(remote_working_cs), 
		pe:remote_ip, 
		po:Number(remote_port), 
		it:Number(remote_upload_interval), 
		kl:remote_heartbeat?1:0
	};
	
	if(md == TCP_IP_M_NORMAL) {
		setval['normal'] = {
			pt:Number(remote_proto), 
			ms:Number(remote_proto_ms), 
			mad:Number(remote_modbus_addr)
		}
	} else if(md == TCP_IP_M_XFER) {
		setval['xfer'] = {
			md:getNumber(prefix + '_xfer_mode'), 
			pt:Number(remote_wm_xfer_proto_type), 
			dt:Number(remote_xfer_gw_trt_dst_type), 
			tidx:Number(remote_xfer_gw_trt_dst_gprs_index), 
			ubr:Number(remote_xfer_gw_trt_dst_uart_baudrate), 
			udb:parseInt(remote_xfer_gw_trt_dst_uart_bits.charAt(0)), 
			upy:parseInt(remote_xfer_gw_trt_dst_uart_bits.charAt(1)), 
			usb:parseInt(remote_xfer_gw_trt_dst_uart_bits.charAt(2))
		}
	}
	
	MyGetJSONWithArg("正在设置以太网 TCP/IP配置","/cgi-bin/setTcpipCfg?",JSON.stringify(setval));
}


//本地网络配置
function Show_Local_Network_Configuration()
{
	MyGetJSONWithArg("","/cgi-bin/getNetCfg?","", function (info)
	{
		Page_Local_Network_Configuration(info);
	});
}

function Page_Local_Network_Configuration(cfg)
{
	var prefix = 'local';
	//DHCP开关
	setValue(prefix + '_dhcp', cfg.dh);
	//IP 地址
	setValue(prefix + '_ip', cfg.ip);
	//子网掩码
	setValue(prefix + '_mask', cfg.mask);
	//网关
	setValue(prefix + '_gateway', cfg.gw);
	//DNS
	setValue(prefix + '_dns', cfg.dns);
	
	Show_Local_Setup();
}

function Apply_Local_Network_Configuration()
{
	var prefix = 'local';
	//IP 地址
	var local_dhcp = document.getElementById(prefix + '_dhcp').value;
	//IP 地址
	var local_ip = document.getElementById(prefix + '_ip').value;
	//子网掩码
	var local_mask = document.getElementById(prefix + '_mask').value;
	//网关
	var local_gateway = document.getElementById(prefix + '_gateway').value;
	//DNS
	var local_dns = document.getElementById(prefix + '_dns').value;

	if (chk(local_dhcp, "DHCP开关")) return;
	if( Number(local_dhcp) != 1 ) {
		if (chk(local_ip, "IP 地址")) return;
		if (chk(local_mask, "子网掩码")) return;
		if (chk(local_gateway, "网关")) return;
		if (chk(local_dns, "DNS")) return;
	}
	
	var setval = {
		dh:Number(local_dhcp), 
		ip:local_ip, 
		mask:local_mask, 
		gw:local_gateway, 
		dns:local_dns
	};
	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在配置网络,请稍后...","/cgi-bin/setNetCfg?",JSON.stringify(setval));
}


//LoRa 配置
function Show_LoRa()
{
	MyGetJSONWithArg("正在获取LoRa配置","/cgi-bin/getLoRaCfg?", "", function( cfg ){
		if(cfg != null && cfg.ret != 0) {
			alert("LoRa 初始化失败,请检查后再重试。");
		} else {
			Page_LoRa(cfg);
		}
	});
}

function Page_LoRa(cfg)
{
	//工作模式
	setValue('LoRa_Working_Mode',cfg.wmd);
	//学习间隔
	setValue('LoRa_LearnStep',cfg.lstep);
	//ID
	setValue('LoRa_ID', cfg.id);
	//组地址
	setValue('LoRa_Maddr', cfg.maddr);
	//本机地址
	setValue('LoRa_Addr', cfg.addr);
	//带宽
	setValue('LoRa_BW', cfg.bw);
	//编码速率
	setValue('LoRa_CR', cfg.cr);
	//发送功率
	setValue('LoRa_Pow', cfg.pow);
	//发送频率
	setValue('LoRa_Tfreq', (Number(cfg.tfreq)/1000000).toFixed(1));
	//接收频率
	setValue('LoRa_Rfreq', (Number(cfg.rfreq)/1000000).toFixed(1));
	//发送扩频因子
	setValue('LoRa_Tsf', cfg.tsf);
	//接收扩频因子
	setValue('LoRa_Rsf', cfg.rsf);
	setValue('LoRa_It', cfg.it);
}

function Apply_LoRa()
{
	var LoRa_Working_Mode = getValue("LoRa_Working_Mode");
	var LoRa_LearnStep = getValue("LoRa_LearnStep");
	var LoRa_ID = getValue("LoRa_ID");
	var LoRa_Maddr = getValue("LoRa_Maddr");
	var LoRa_Addr = getValue("LoRa_Addr");
	var LoRa_BW = getValue("LoRa_BW");
	var LoRa_CR = getValue("LoRa_CR");
	var LoRa_Pow = getValue("LoRa_Pow");
	var LoRa_Tfreq = getValue("LoRa_Tfreq");
	var LoRa_Rfreq = getValue("LoRa_Rfreq");
	var LoRa_Tsf = getValue("LoRa_Tsf");
	var LoRa_Rsf = getValue("LoRa_Rsf");
	var LoRa_It = getValue("LoRa_It");
	
	if (chk(LoRa_Working_Mode, "LoRa 工作模式")) return;
	if (chk(LoRa_LearnStep, "LoRa 学习间隔")) return;
	if (chk(LoRa_Maddr, "LoRa 组地址")) return;
	if (chk(LoRa_Addr, "LoRa 本机地址")) return;
	if (chk(LoRa_BW, "LoRa 工作模式")) return;
	if (chk(LoRa_CR, "LoRa 编码速率")) return;
	if (chk(LoRa_Pow, "LoRa 发送功率")) return;
	if (chk(LoRa_Tfreq, "LoRa 发送频率")) return;
	if (chk(LoRa_Rfreq, "LoRa 接收频率")) return;
	if (chk(LoRa_Tsf, "LoRa 发送扩频因子")) return;
	if (chk(LoRa_Rsf, "LoRa 接收扩频因子")) return;
	if (chk(LoRa_It, "LoRa 采集间隔")) return;
	
	var setval = {
		wmd:Number(LoRa_Working_Mode), 
		lstep:Number(LoRa_LearnStep), 
		maddr:LoRa_Maddr, 
		addr:LoRa_Addr, 
		bw:Number(LoRa_BW), 
		cr:Number(LoRa_CR), 
		pow:Number(LoRa_Pow), 
		tfreq:Number(LoRa_Tfreq) * 1000000, 
		rfreq:Number(LoRa_Rfreq) * 1000000, 
		tsf:Number(LoRa_Tsf), 
		rsf:Number(LoRa_Rsf), 
		it:Number(LoRa_It)
	};
	
	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在配置LoRa,请稍后...","/cgi-bin/setLoRaCfg?",JSON.stringify(setval));
}

function Apply_Network_Adapter_Configuration()
{
	//adapter
	var adpt = getNumber('net_adapter');

	var setval = {
		adpt:adpt
	};
	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在配置网络适配器,请稍后...","/cgi-bin/setNetCfg?",JSON.stringify(setval));
}

//GPRS 配置——工作模式及中心配置
function Show_GPRS_Transmission()
{
	//每次选择时，都会调用该函数；
	//获取【GPRS 配置——工作模式及中心配置】；=====================================================================>需要调用【设备接口】获取
	//如下是例子，例如：
	//Tabel_GPRS_Parameter(new Array(new Array("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", "GGG", "HHH", "JJJ"), new Array("KKK", "LLL", "MMM", "OOO", "PPP", "QQQ", "RRR", "SSS", "TTT"), new Array("UUU", "VVV", "WWW")));
	//-----------------------------------------------------------------------------------------------------------------------

	var val = document.getElementById("GPRS_group").value;
	var setval = { n:(Number(val)+ENET_TCPIP_NUM) };
	
	MyGetJSONWithArg("正在获取GRPS TCP/IP配置","/cgi-bin/getTcpipCfg?",JSON.stringify(setval), function (info)
	{
		Page_GPRS_Transmission(info)
	});
}


var GPRS_Parameter_Items = 0;
function Tabel_GPRS_Parameter(obj)
{
	var str = "<table id='table_gprs_upload' style='display:none'>";

	try
	{
		var k = 0;
		for (var n = 0; n < obj.length; n++)
		{
			str += "<tr>";
			for (var j = 0; j < obj[n].length; j++)
			{
				str += "<td><input name='GPRS_upload_data' type='checkbox' id='GPRS_upload_data" + k + "' value='" + k + "' />&nbsp;参数" + obj[n][j] + "</td>";
				k++;
			}
			str += "</tr>";
		}
		GPRS_Parameter_Items = k;
	}
	catch (e)
	{
	}

	str += "</table>";

	document.getElementById("div_gprs_parameter").innerHTML = str;

}

function GPRS_proto_change() {
	setDisplay('GPRS_proto_ms_addr', (getNumber('GPRS_proto_ms')== 0)&&getNumber('GPRS_proto')==PROTO_MODBUS_RTU_OVER_TCP);
}

function GPRS_working_type_change() {
	setDisplay('GPRS_wm_normal_ul', getNumber('GPRS_working_type') == 0 );
	setDisplay('GPRS_wm_xfer_ul', getNumber('GPRS_working_type') == 1 );
}

function GPRS_xfer_gw_trt_dst_type_change() {
	var is_gw = getNumber('GPRS_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber('GPRS_xfer_mode')== XFER_M_TRT;
	var dst_type = -1;
	if(is_gw||is_trt) {
		dst_type = getNumber('GPRS_xfer_gw_trt_dst_type');
	}
	setDisplay('GPRS_xfer_gw_trt_dst_net_index_ul', (is_gw||is_trt)&&(dst_type==PROTO_DEV_NET));
	setDisplay('GPRS_xfer_gw_trt_dst_uart_type_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay('GPRS_xfer_gw_trt_dst_uart_baudrate_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay('GPRS_xfer_gw_trt_dst_uart_bits_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
}

function GPRS_xfer_mode_change() {
	var is_gw = getNumber('GPRS_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber('GPRS_xfer_mode')== XFER_M_TRT;
	setDisplay('GPRS_wm_xfer_proto_type_ul', is_gw);
	setDisplay('GPRS_xfer_gw_trt_dst_type_ul', is_gw||is_trt);
	GPRS_xfer_gw_trt_dst_type_change();
}

function show_GPRS_btn(){
	setDisplay('show_GPRS_dialog_btn', false);
	setDisplay('GPRS_proto_ms_ul', true);
	setEnable('GPRS_working_cs', true);
	setEnable('GPRS_socket_type', true);
	switch(getNumber('GPRS_proto')) {
	case PROTO_CC_BJDC: case PROTO_HJT212: case PROTO_DM101: case PROTO_MQTT:
		setDisplay('show_GPRS_dialog_btn', true);
		setEnable('GPRS_working_cs', false);
		if(__is_gprs()) {
			setEnable('GPRS_socket_type', false);
		}
		setDisplay('GPRS_proto_ms_ul', false);
		setValue('GPRS_working_cs', 0);
		if(__is_gprs()) {
			setValue('GPRS_socket_type', 0);
		}
		break;
	}
	GPRS_proto_change();
	GPRS_working_type_change();
	GPRS_xfer_mode_change();
}

//通讯协议弹窗
function show_GPRS_dialog(){
		setGPRSFileToTextarea('');
		showDialog('GPRS_dialog');

		$("#dialog_GPRS_ok_btn").off().on("click", function(){
			saveGPRSConfig();
			hideDialog('GPRS_dialog');
		})
}

function setGPRSFileToTextarea()
{
	if(getValue('GPRS_proto')==PROTO_HJT212) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_hjt212_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#GPRS_config_textarea").val(data);
			}
		}); 
	} else if (getValue('GPRS_proto')==PROTO_DM101) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_dm101_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#GPRS_config_textarea").val(data);
			}
		}); 
	} else if (getValue('GPRS_proto')==PROTO_MQTT) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_mqtt_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#GPRS_config_textarea").val(data);
			}
		}); 
	} else if (getValue('GPRS_proto')==PROTO_DH) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_dh_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#GPRS_config_textarea").val(data);
			}
		}); 
	}
}

function saveGPRSConfig()
{
	var _val = $("#GPRS_config_textarea").val();
	var xhr = createXMLHttpRequest();
	if (xhr) {
		Show("正在保存配置,请稍后...");
		xhr.onreadystatechange = function() {
			if( xhr.readyState==4 ) {
				Close();
				if( xhr.status==200 ) {
					alert("保存成功，重启生效！");
				} else {
					alert("失败,请重试");
				}
			}
		}
		if(getValue('GPRS_proto')==PROTO_HJT212) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_hjt212_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini" );
		} else if (getValue('GPRS_proto')==PROTO_DM101) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_dm101_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini" );
		} else if (getValue('GPRS_proto')==PROTO_MQTT) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_mqtt_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini" );
		} else if (getValue('GPRS_proto')==PROTO_DH) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_dh_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini" );
		}
		xhr.send(_val);
	}
}

function Enable_GPRS_Transmission()
{
	var v = !getChecked("GPRS_group_is");

	document.getElementById("GPRS_socket_type").disabled = v;
	document.getElementById("GPRS_working_cs").disabled = v;
	document.getElementById("GPRS_proto").disabled = v;
	document.getElementById("GPRS_proto_ms").disabled = v;

	document.getElementById("GPRS_peer_ip").disabled = v;
	document.getElementById("GPRS_peer_port").disabled = v;
	document.getElementById("GPRS_upload_interval").disabled = v;
	document.getElementById("GPRS_heartbeat").disabled = v;

	show_GPRS_btn();
}


function Page_GPRS_Transmission(cfg)
{
	//是否启用 
	setCheckBoxEnable("GPRS_group_is", Number(cfg.en)!=0);
	//以太网工作模式
	setValue('GPRS_socket_type', cfg.tt);
	setValue('GPRS_working_cs', cfg.cs);
	//设备NM号（0-20位）
	//setValue('remote_dev', obj.null);
	//域名地址
	setValue('GPRS_peer_ip', cfg.pe);
	//域名端口
	setValue('GPRS_peer_port', cfg.po);
	//上传间隔
	setValue('GPRS_upload_interval', cfg.it);

	//是否启用心跳包 
	setCheckBoxEnable("GPRS_heartbeat", Number(cfg.kl)!=0);

	setValue('GPRS_working_type', cfg.md);
	//协议
	if(cfg.md == TCP_IP_M_NORMAL) {
		if( cfg.normal.pt != null ) setValue('GPRS_proto', cfg.normal.pt);
		if( cfg.normal.ms != null ) setValue('GPRS_proto_ms', cfg.normal.ms);
		if( cfg.normal.mad != null ) setValue('GPRS_modbus_addr', cfg.normal.mad);
	} else if(cfg.md == TCP_IP_M_XFER) {
		setValue('GPRS_xfer_mode', cfg.xfer.md);
		if( cfg.xfer.pt != null ) setValue('GPRS_wm_xfer_proto_type', cfg.xfer.pt);
		if( cfg.xfer.dt != null ) setValue('GPRS_xfer_gw_trt_dst_type', cfg.xfer.dt);
		if( cfg.xfer.tidx != null ) setValue('GPRS_xfer_gw_trt_dst_net_index', cfg.xfer.tidx);
		//if( cfg.xfer.uty != null ) setValue('GPRS_xfer_gw_trt_dst_uart_type', cfg.xfer.uty);
		if( cfg.xfer.ubr != null ) setValue('GPRS_xfer_gw_trt_dst_uart_baudrate', cfg.xfer.ubr);
		if( cfg.xfer.udb != null && cfg.xfer.usb != null  && cfg.xfer.upy != null ) setValue('GPRS_xfer_gw_trt_dst_uart_bits', (Number(cfg.xfer.udb) * 100) + (Number(cfg.xfer.upy) * 10) + Number(cfg.xfer.usb));
	}
	
	MyGetJSONWithArg("","/cgi-bin/getXferUartCfg?","", function (info)
	{
		Show_xfer_uart_cfg('GPRS_', info.cfg);
	});
	
	Enable_GPRS_Transmission();
}


var table_gprs_upload_turnoff = 0;
function Show_gprs_Transmission_downlist()
{
	if (table_gprs_upload_turnoff == 0)
	{
		table_gprs_upload_turnoff = 1;
		document.getElementById("table_gprs_upload").style.display = "";
	}
	else
	{
		table_gprs_upload_turnoff = 0;
		document.getElementById("table_gprs_upload").style.display = "none";
	}

}


function Apply_gprs_Transmission_downlist()
{
	//是否启用
	var GPRS_group_is = getChecked("GPRS_group_is");
	//以太网工作模式 
	var GPRS_working_type = getValue("GPRS_working_type");
	var GPRS_socket_type = getValue("GPRS_socket_type");
	var GPRS_working_cs = getValue("GPRS_working_cs");

	//设备NM号（0-20位）
	//var GPRS_dev_ = getValue("GPRS_dev");
	//域名地址
	var GPRS_peer_ip = getValue("GPRS_peer_ip");
	//域名端口
	var GPRS_peer_port = getValue("GPRS_peer_port");
	//上传间隔
	var GPRS_upload_interval_ = getValue("GPRS_upload_interval");

	//是否启用心跳包
	var GPRS_heartbeat = getChecked("GPRS_heartbeat");
	
	//协议
	var GPRS_proto = getValue("GPRS_proto");
	var GPRS_proto_ms = getValue("GPRS_proto_ms");
	var GPRS_modbus_addr = getValue("GPRS_modbus_addr");
	
	var GPRS_wm_xfer_proto_type = getValue("GPRS_wm_xfer_proto_type");
	var GPRS_xfer_gw_trt_dst_type = getValue("GPRS_xfer_gw_trt_dst_type");
	var GPRS_xfer_gw_trt_dst_uart_type = getValue("GPRS_xfer_gw_trt_dst_uart_type");
	var GPRS_xfer_gw_trt_dst_uart_baudrate = getValue("GPRS_xfer_gw_trt_dst_uart_baudrate");
	var GPRS_xfer_gw_trt_dst_uart_bits = getValue("GPRS_xfer_gw_trt_dst_uart_bits");
	var GPRS_xfer_gw_trt_dst_net_index = getValue("GPRS_xfer_gw_trt_dst_net_index");

	var md = Number(GPRS_working_type);

	if( GPRS_group_is ) {
		if (chk2(GPRS_socket_type, GPRS_working_cs, "套接字类型")) return;
		//if (chk(GPRS_dev_, "设备NM号")) return;
		if( Number(GPRS_working_cs) != 1 ) {
			if (chk(GPRS_peer_ip, "域名地址")) return;
		}
		if (chk(GPRS_peer_port, "域名端口")) return;
		if (chk(GPRS_upload_interval_, "上传间隔")) return;
		//if (chk(GPRS_parameter, "上传的数据")) return;
		if(md == TCP_IP_M_NORMAL) {
			if(Number(GPRS_proto) == 1 ) {
				GPRS_proto_ms = 0;
			}
			if (chk2(GPRS_proto, GPRS_proto_ms, "协议")) return;
			if( Number(GPRS_proto)==PROTO_MODBUS_RTU_OVER_TCP&&Number(GPRS_proto_ms)==0) {
				if (chk(GPRS_modbus_addr, "Modbus 从机地址")) return;
			}
		} else if(md == TCP_IP_M_XFER) {
			var is_gw = getNumber('GPRS_xfer_mode')== XFER_M_GW;
			var is_trt = getNumber('GPRS_xfer_mode')== XFER_M_TRT;
			var dst_type = -1;
			if(is_gw||is_trt) {
				dst_type = getNumber('GPRS_xfer_gw_trt_dst_type');
			}
			if((is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX) {
				//if(chk(GPRS_xfer_gw_trt_dst_uart_type, "转发端口")) return;
				if(chk(GPRS_xfer_gw_trt_dst_uart_baudrate, "转发波特率")) return;
				if(chk(GPRS_xfer_gw_trt_dst_uart_bits, "转发数据位")) return;
			}
			if((is_gw||is_trt)&&(dst_type==PROTO_DEV_NET)) {
				if(chk(GPRS_xfer_gw_trt_dst_net_index, "转发目标以太网组号")) return;
			}
			if(is_gw||is_trt) {
				if(chk(GPRS_xfer_gw_trt_dst_type, "转发端口")) return;
			}
			if(is_gw) {
				if(chk(GPRS_wm_xfer_proto_type, "转发协议")) return;
			}
		}
	}

	//注：在此处填写保存代码即可；
	var val = getNumber("GPRS_group") + ENET_TCPIP_NUM;
	var setval = {
		n:Number(val), 
		en:GPRS_group_is?1:0, 
		md:Number(GPRS_working_type), 
		tt:Number(GPRS_socket_type), 
		cs:Number(GPRS_working_cs), 
		pe:GPRS_peer_ip, 
		po:Number(GPRS_peer_port), 
		it:Number(GPRS_upload_interval_), 
		kl:GPRS_heartbeat?1:0
	};
	
	if(md == TCP_IP_M_NORMAL) {
		setval['normal'] = {
			pt:Number(GPRS_proto), 
			ms:Number(GPRS_proto_ms), 
			mad:Number(GPRS_modbus_addr)
		}
	} else if(md == TCP_IP_M_XFER) {
		setval['xfer'] = {
			md:getNumber('GPRS_xfer_mode'), 
			pt:Number(GPRS_wm_xfer_proto_type), 
			dt:Number(GPRS_xfer_gw_trt_dst_type), 
			tidx:Number(GPRS_xfer_gw_trt_dst_net_index), 
			ubr:Number(GPRS_xfer_gw_trt_dst_uart_baudrate), 
			udb:parseInt(GPRS_xfer_gw_trt_dst_uart_bits.charAt(0)), 
			upy:parseInt(GPRS_xfer_gw_trt_dst_uart_bits.charAt(1)), 
			usb:parseInt(GPRS_xfer_gw_trt_dst_uart_bits.charAt(2))
		}
	}

	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + " TCP/IP配置","/cgi-bin/setTcpipCfg?",JSON.stringify(setval));
}

//GPRS 工作参数配置
function Show_GPRS_Config()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("正在获取" + GPRS_OR_NBIOT + "工作参数","/cgi-bin/getGPRSWorkCfg?","", function (cfg)
	{
		Page_GPRS_Config(cfg)
	});
}

function gprs_wmode_change()
{
	var _en = (getNumber('GPRS_Config_Mode') != 3);
	setEnable('GPRS_Config_Activation', _en);
	setEnable('GPRS_Config_Level', _en);
	setEnable('GPRS_Config_SIM', _en);
	setEnable('GPRS_Config_MS', _en);
	setEnable('GPRS_Config_Reg', _en);
	setEnable('GPRS_Config_Jump', _en);
	setEnable('GPRS_Config_Time', _en);
}

function Page_GPRS_Config(cfg)
{
	//工作模式
	setValue('GPRS_Config_Mode', cfg.wm);
	//激活方式
	setValue('GPRS_Config_Activation', cfg.om);
	//调试等级
	setValue('GPRS_Config_Level', cfg.dl);
	//SIM卡号码
	setValue('GPRS_Config_SIM', cfg.simno);
	//数据帧时间
	setValue('GPRS_Config_MS', cfg.it);
	//自定义注册包
	setValue('GPRS_Config_Reg', cfg.reg);
	//自定义心跳包
	setValue('GPRS_Config_Jump', cfg.hrt);
	//重连次数
	setValue('GPRS_Config_Time', cfg.rt);

	gprs_wmode_change();
}


function Apply_GPRS_Config()
{
	//工作模式
	var GPRS_Config_Mode = getValue("GPRS_Config_Mode");
	
	//激活方式
	var GPRS_Config_Activation = getValue("GPRS_Config_Activation");
	
	//调试等级
	var GPRS_Config_Level = getValue("GPRS_Config_Level");
	
	//SIM卡号码
	var GPRS_Config_SIM = getValue("GPRS_Config_SIM");
	//数据帧时间
	var GPRS_Config_MS = getValue("GPRS_Config_MS");
	//自定义注册包
	var GPRS_Config_Reg = getValue("GPRS_Config_Reg");
	//自定义心跳包
	var GPRS_Config_Jump = getValue("GPRS_Config_Jump");
	//重连次数
	var GPRS_Config_Time = getNumber("GPRS_Config_Time");
	
	//注：在此处填写保存代码即可；
	var setval = { };

	if( Number(GPRS_Config_Mode) != 3 ) {
		if (chk(GPRS_Config_Mode, "工作模式")) return;
		if (chk(GPRS_Config_Activation, "激活方式")) return;
		if (chk(GPRS_Config_Level, "调试等级")) return;
		//if (chk(GPRS_Config_SIM, "SIM卡号码")) return;
		if (chk(GPRS_Config_MS, "数据帧时间")) return;
		//if (chk(GPRS_Config_Reg, "自定义注册包")) return;
		//if (chk(GPRS_Config_Jump, "自定义心跳包")) return;
		if (chk(GPRS_Config_Time, "重连次数")) return;
		
		setval = {
			wm:Number(GPRS_Config_Mode), 
			om:Number(GPRS_Config_Activation), 
			dl:Number(GPRS_Config_Level), 
			simno:GPRS_Config_SIM, 
			it:Number(GPRS_Config_MS), 
			rt:Number(GPRS_Config_Time), 
			reg:GPRS_Config_Reg, 
			hrt:GPRS_Config_Jump
		};
		
	} else {
		setval = {
			wm:Number(GPRS_Config_Mode)
		}
	}

	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + "工作参数","/cgi-bin/setGPRSWorkCfg?",JSON.stringify(setval));
}


//无线配置
function Show_GPRS_Wireless()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_GPRS_Wireless("注：此处传入一个对象");
	
	MyGetJSONWithArg("正在获取" + GPRS_OR_NBIOT + "配置","/cgi-bin/getGPRSNetCfg?","", function (info)
	{
		Page_GPRS_Wireless(info)
	});
}

function Page_GPRS_Wireless(info)
{
	//无线网络APN
	setValue('GPRS_Wireless_APN', info.apn);
	//APN用户名
	setValue('GPRS_Wireless_User', info.user);
	//APN密码
	setValue('GPRS_Wireless_PWD', info.psk);
	//APN拨号号码
	setValue('GPRS_Wireless_NUM', info.apnno);
	//短信中心号码
	setValue('GPRS_Wireless_Center', info.msgno);
}


function Apply_GPRS_Wireless()
{
	//无线网络APN
	var GPRS_Wireless_APN = getValue("GPRS_Wireless_APN");
	//APN用户名
	var GPRS_Wireless_User = getValue("GPRS_Wireless_User");
	//APN密码
	var GPRS_Wireless_PWD = getValue("GPRS_Wireless_PWD");
	//APN拨号号码
	var GPRS_Wireless_NUM = getValue("GPRS_Wireless_NUM");
	//短信中心号码
	var GPRS_Wireless_Center = getValue("GPRS_Wireless_Center");

	//if (chk(GPRS_Wireless_APN, "无线网络APN")) return;
	//if (chk(GPRS_Wireless_User, "APN用户名")) return;
	//if (chk(GPRS_Wireless_PWD, "APN密码")) return;
	//if (chk(GPRS_Wireless_NUM, "APN拨号号码")) return;
	//if (chk(GPRS_Wireless_Center, "短信中心号码")) return;

	//注：在此处填写保存代码即可；
	var setval = {
		apn:GPRS_Wireless_APN, 
		user:GPRS_Wireless_User, 
		psk:GPRS_Wireless_PWD, 
		apnno:''+GPRS_Wireless_NUM, 
		msgno:''+GPRS_Wireless_Center
	};
	
	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + "配置信息","/cgi-bin/setGPRSNetCfg?",JSON.stringify(setval));
}


//串口配置
function Show_Serial_Port()
{
	var n = document.getElementById("Serial_Port_Select").selectedIndex;
	//var Serial_Port_Select = document.getElementById("Serial_Port_Select").options[v].text;
	//alert('当前选择【' + Serial_Port_Select + '】请此处添加代码。');
	//【你需要添加代码】通过“Serial_Port_Select”序号查找到对应的信息；
	//再把“对应信息”传给Page_Remote_Transmission函数；
	//Page_Serial_Port(Serial_Port_Select);
		
	var setval = {
		n:Number(n)
	};
	
	MyGetJSONWithArg("正在获取串口配置信息","/cgi-bin/getUartCfg?",JSON.stringify(setval), function( cfg ){
		Page_Serial_Port(cfg)
	});
}

function resreshLuaSelectList(_id, _list)
{
	var selectobj = window.document.getElementById(_id);
	if(selectobj==null) return ;
	selectobj.options.length = 0;
	if (_list!=null&&_list.length>0) {
		for(var i = 0; i < _list.length; i++) {
			if (_list[i].type == 'file') {
				var _name = getLuaName(_list[i].name);
				if (_name.toLowerCase() != 'rtu_main') {
					selectobj.options.add(new Option(_name, _name));
				}
			}
		}
	}
}

function tryResreshLuaSelectList(_id)
{
	var selectobj = window.document.getElementById(_id);
	if(selectobj==null) return ;
	if (xLuaList==null||xLuaList.length<=0) {
		MyGetJSON("正在获取脚本列表","/cgi-bin/listDir?", 'path', "/lua" ,function( info ){
		if (info.list!=null && info.list!=null) {
			xLuaList = info.list.concat();
			for( var i = 0; i < xLuaList.length; i++ ) {
				if( xLuaList[i].type == 'file' && getLuaName(xLuaList[i].name).toLowerCase() == "rtu_main" ) {
					xLuaList[0] = xLuaList.splice(i,1,xLuaList[0])[0];
					break;
				}
			}
			resreshLuaSelectList(_id, info.list);
		}
		});
	} else {
		resreshLuaSelectList(_id,xLuaList);
	}
}

function onSerialProtoChange()
{
	setDisplay('Serial_Port_proto_lua_ul', getNumber('Serial_Port_proto') == PROTO_LUA );
	setDisplay('Serial_Port_proto_nlua_ul', getNumber('Serial_Port_proto') != PROTO_LUA && !__proto_is_master_fixed(getNumber('Serial_Port_proto')));
	if(getNumber('Serial_Port_proto') == PROTO_LUA) {
		tryResreshLuaSelectList('Serial_Port_proto_lua_list');
	}
	setDisplay('Serial_Port_proto_ms_addr', (getNumber('Serial_Port_proto_ms') == 0)&&(getNumber('Serial_Port_proto')!=PROTO_LUA)&&(!__proto_is_master_fixed(getNumber('Serial_Port_proto'))) );
}

function Page_Serial_Port(cfg)
{
	//串口模式
	//setValue("Serial_Port_Mode", cfg.ut);

	//协议配置
	setValue("Serial_Port_proto", cfg.po);
	setValue("Serial_Port_proto_ms", cfg.ms);
	//mdobus从机地址
	setValue("Serial_Port_ModbusAddr", cfg.ad);
	
	//串口波特率
	setValue("Serial_Port_Rate", cfg.bd);
	//串口校验位
	//setValue("Serial_Port_parity", cfg.py);

	//采集间隔（MS）
	//change name by chenqianqian
	//setValue("Serial_Port_Interval", cfg.in);
	setValue("Serial_Port_Interval", cfg['in']);

	//数据位+校验位+停止位
	setValue("Serial_Port_Data", (Number(cfg.da) * 100) + (Number(cfg.py) * 10) + cfg.st );
	setDisplay('Serial_Port_proto_ms_addr', (getNumber('Serial_Port_proto_ms') == 0)&&(getNumber('Serial_Port_proto')!=PROTO_LUA)&&(!__proto_is_master_fixed(getNumber('Serial_Port_proto'))) );
	onSerialProtoChange(cfg.luapo);
	setValue("Serial_Port_proto_lua_list", cfg.luapo);
}

function Apply_Serial_Port()
{
	//选择串口
	var Serial_Port_Select = document.getElementById("Serial_Port_Select").value;
	
	//var Serial_Port_Mode = document.getElementById("Serial_Port_Mode").value;
	
	//协议配置
	var Serial_Port_proto = document.getElementById("Serial_Port_proto").value;
	var Serial_Port_proto_lua = document.getElementById("Serial_Port_proto_lua_list").value;
	var Serial_Port_proto_ms = document.getElementById("Serial_Port_proto_ms").value;
		
	//mdobus从机地址
	var Serial_Port_ModbusAddr = document.getElementById("Serial_Port_ModbusAddr").value;
	
	//串口波特率
	var Serial_Port_Rate = document.getElementById("Serial_Port_Rate").value;
	//var Serial_Port_parity = document.getElementById("Serial_Port_parity").value;

	//采集间隔（MS）
	var Serial_Port_Interval = document.getElementById("Serial_Port_Interval").value;
	
	//数据位
	var Serial_Port_data_stop = document.getElementById("Serial_Port_Data").value;

	if (chk(Serial_Port_Select, "选择串口")) return;
	//if (chk(Serial_Port_Mode, "选择串口模式")) return;
	if (chk(Serial_Port_proto, "协议配置")) return;
	if(Number(Serial_Port_proto)==PROTO_LUA) {
		if (chk(Serial_Port_proto_lua, "选择lua协议")) return;
	} else {
		if (chk(Serial_Port_proto_ms, "协议主从配置")) return;
		if( Number(Serial_Port_proto_ms) ==  0 ) {
			if (chk(Serial_Port_ModbusAddr, "Modbus从机地址")) return;
		}
	}
	if (chk(Serial_Port_Rate, "串口波特率")) return;

	//if (chk(Serial_Port_parity, "校验位")) return;
	if (chk(Serial_Port_Interval, "采集间隔")) return;
	
	var Serial_Port_data = parseInt(Serial_Port_data_stop.charAt(0));
	var Serial_Port_parity = parseInt(Serial_Port_data_stop.charAt(1));
	var Serial_Port_stop = parseInt(Serial_Port_data_stop.charAt(2));

	//注：在此处填写保存代码即可；
	var setval = {
		n:Number(Serial_Port_Select), 
		bd:Number(Serial_Port_Rate), 
		//ut:Number(Serial_Port_Mode), 
		po:Number(Serial_Port_proto), 
		py:Number(Serial_Port_parity), 
		ms:Number(Serial_Port_proto_ms), 
		luapo:Serial_Port_proto_lua,
		ad:Number(Serial_Port_ModbusAddr), 
		//change name by chenqq
		//in:Number(Serial_Port_Interval), 
		'in':Number(Serial_Port_Interval), 
		da:Number(Serial_Port_data), 
		st:Number(Serial_Port_stop)
	};
	
	MyGetJSONWithArg("正在设置串口配置信息","/cgi-bin/setUartCfg?",JSON.stringify(setval));
}

//导出
function Derived(){
	if (confirm('你确定导出吗？')){
		var Derived_type = document.getElementById("Derived_type").value;
		var data_export=document.getElementById("data_export").value;
		//console.log($('#Derived_type').val());
		if (chk(data_export, "数据导出")) return;
		if (chk(Derived_type, "导出类型")) return;
	}
}

function Show_Defin()
{
	var n = document.getElementById("div_defin_script");
	//========================================================================
	//获取全部脚本列表；（不需分页）
	//========================================================================
	/*var s = "";
	s += "<table style='width:96%;margin-top:0;'>";
	s+='<tr><td class="back_8fc6">ID</td><td class="back_8fc6">变量名</td><td class="back_8fc6">数据类型</td><td class="back_8fc6">初始值</td><td class="back_8fc6">最小值</td><td class="back_8fc6">最大值</td><td class="back_8fc6">表达式</td><td class="back_8fc6">别名</td><td class="back_8fc6">备注说明</td><td class="back_8fc6">操作</td></tr>';
	//------------重复-----------------
	s += "<tr><td>1</td><td>prame1</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率1</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"1\",\"prame1\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率1\");' /></td></tr>";
	s += "<tr><td>2</td><td>prame2</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率2</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"2\",\"prame2\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率2\");' /></td></tr>";
	s += "<tr><td>3</td><td>prame3</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率3</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"3\",\"prame3\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率3\");' /></td></tr>";
	s += "<tr><td>4</td><td>prame4</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率4</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"4\",\"prame4\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率4\");' /></td></tr>";
	//---------------------------------
	s += "</table>";

	n.innerHTML = s;*/
}


function Detail_Defin(a, b, c, d, e, f, g, h, i)
{
	document.getElementById("user_defin_id").value = a;
	document.getElementById("user_defin_name").value = b;
	setValue('user_defin_datatype', c);
	document.getElementById("user_defin_init").value = d;
	document.getElementById("user_defin_min").value = e;
	document.getElementById("user_defin_max").value = f;
	document.getElementById("user_defin_express").value = g;
	document.getElementById("user_defin_rename").value = h;
	document.getElementById("user_defin_note").value = i;

	setEnable("btn_defin_del", true);
}

//新建
function Clear_Defin()
{
	document.getElementById("user_defin_id").value = "";
	document.getElementById("user_defin_name").value = "";
	setValue('user_defin_datatype', 0);
	document.getElementById("user_defin_init").value = "";
	document.getElementById("user_defin_min").value = "";
	document.getElementById("user_defin_max").value = "";
	document.getElementById("user_defin_express").value = "";
	document.getElementById("user_defin_rename").value = "";
	document.getElementById("user_defin_note").value = "";
	setEnable("btn_defin_del", false);
}


//保存
function Add_Defin()
{
	if (confirm('你确定继续保存？'))
	{
		var n = document.getElementById("div_defin_script");
		var user_defin_id = document.getElementById("user_defin_id").value;
		var user_defin_name = document.getElementById("user_defin_name").value;
		var user_defin_datatype = document.getElementById("user_defin_datatype").value;
		var user_defin_init = document.getElementById("user_defin_init").value;
		var user_defin_min = document.getElementById("user_defin_min").value;
		var user_defin_max = document.getElementById("user_defin_max").value;
		var user_defin_express = document.getElementById("user_defin_express").value;
		var user_defin_rename = document.getElementById("user_defin_rename").value;
		var user_defin_note = document.getElementById("user_defin_note").value;

		if (chk(user_defin_id, "ID")) return;
		if (chk(user_defin_name, "变量名")) return;
		if (chk(user_defin_datatype, "数据类型")) return;
		if (chk(user_defin_init, "初始值")) return;
		if (chk(user_defin_min, "最小值")) return;
		if (chk(user_defin_max, "最大值")) return;
		if (chk(user_defin_express, "表达式")) return;
		if (chk(user_defin_rename, "别名")) return;
		if (chk(user_defin_note, "备注说明")) return;

		if (n.innerHTML.lastIndexOf("<td>" + user_defin_name + "</td>") == -1)
		{
			//=======================保存接口 
			alert('保存成功');
			Show_Defin();
			Clear_Defin();
		}
		else
		{
			alert("变量名在RTU设备已存在！！");
		}
	}
}

function Del_Defin()
{
	if (confirm('你确定继续删除？'))
	{
		//=======================删除接口
	}
}

function resetdev()
{
	if( window.confirm('确定要重启设备吗?\n\n重启大约需要40秒钟') ){
		MyGetJSONWithArg("","/cgi-bin/devReset?","",function( cfg ){
			location.reload(true);
		});
	}
}
/*
function downloadCfg()
{
	window.open("http://"+window.location.host+"/download/" + CONFIG_PATH + "/rtu_cfg_v0.cfg");
}
*/

function downloadCfg()
{
	MyGetJSONWithArg("","/cgi-bin/saveCfgWithJson?","",function(res){
		if( res != null && 0 == res.ret ) {
			window.open("http://"+window.location.host+"/download/" + CONFIG_PATH + "/rtu_board_json.cfg");
		} else {
			alert("获取配置失败，请检查后重试！");
		}
	});
	
}

function factoryReset()
{
	if( window.confirm('建议先备份配置.\n\n确定要恢复出厂设置吗?\n\n') ){
		MyGetJSONWithArg("","/cgi-bin/factoryReset?","",function( cfg ){
			location.reload(true);
		});
	}
}

//上传数据配置 chenqq

function Show_upload_cfg(){
	//列表
	MyGetJSONWithArg("正在获取数据上传配置,请稍后...","/cgi-bin/getVarManageExtData?", "{\"all\":1}",function (res) {
		if( res != null && 0 == res.ret ) {
			xVarManageExtDataBase = res.list.concat();
			xProtoDevList = res.protolist;
			xUpProtoDevList = res.upprotolist;
			refreshAllUploadDataCfgBase(0);
		} else {
			alert("获取失败,请重试");
		}
	});
}

function refreshAllUploadDataCfgBase(index)
{
	refreshProtoDevList( xUpProtoDevList, "undefine____dead____", "upload_data_pro_dev" );
	myTableItemRemoveAll("upload_data_table");
	for( var n = 0; n < xVarManageExtDataBase.length; n++ ) {
		var _var = xVarManageExtDataBase[n];
		var _io = _var['io'];
		var _up = _var['up'];
		
		upCfgTableAddItem( 
			_up.en, 
			_tostr_(_var.na), 
			varExtGetVarTypeName(_io.ovt,_io.ovs), 
			_tostr_(_up.nid), 
			_tostr_(_up.fid), 
			_tostr_(_up.unit), 
			varExtGetProtoDev(_up.dt, _up.dtn), 
			varExtGetProto(_up.dt,_up.pt),
			_tostr_(_up.desc)
		);
	}
	var table = window.document.getElementById("upload_data_table");
	onUpCfgTableItemClick(table, table.rows[index]);
}

function upCfgTableAddItem(enable,name,valtype,nid,fid,unit,podev,potype,desc)
{		

	var table = window.document.getElementById("upload_data_table");
	var row = table.insertRow(table.rows.length);
	row.style.height="25px";
	row.onclick = function(){onUpCfgTableItemClick( table,row );}
	var obj = row.insertCell(0);
	obj.innerHTML = enable!=0?"启用":"停用";
	obj = row.insertCell(1);
	obj.innerHTML = name;
	obj = row.insertCell(2);
	obj.innerHTML = valtype;
	obj = row.insertCell(3);
	obj.innerHTML = nid;
	obj = row.insertCell(4);
	obj.innerHTML = fid;
	obj = row.insertCell(5);
	obj.innerHTML = unit;
	obj = row.insertCell(6);
	obj.innerHTML = enable!=0?podev:'--';
	obj = row.insertCell(7);
	obj.innerHTML = enable!=0?potype:'----';
	obj = row.insertCell(8);
	obj.innerHTML = desc;
}

function onUpCfgTableItemClick(tb,row)
{
	if( row != null && row.rowIndex != null && row.rowIndex >= 0 ) {
		
		for (var i = 0; i < tb.rows.length; i++) {
			if( xVarManageExtDataBase[i]['up'].en > 0 ) {
				tb.rows[i].style.background="#FFFFFF";
			} else {
				tb.rows[i].style.background="#F0F0F0";
			}
		}
		row.style.background="#E5E5E5";
		
		var _rowIndex = row.rowIndex;//去掉表头
		var _var = xVarManageExtDataBase[_rowIndex];
		var _io = _var['io'];
		var _up = _var['up'];
		if( _var!=null && _io!=null && _up!=null  ) {
			setCheckBoxEnable('upload_data_enable', _up.en);   //启用   （只或取这个已启用，下面两内容）
			setValue('upload_data_name', _var.na);	 //变量名
			setValue('upload_data_vartype', varExtGetVarTypeName( _io.ovt, _io.ovs));   //数据类型
			setValue('upload_data_nid', _up.nid); 
			setValue('upload_data_fid', _up.fid); 
			setValue('upload_data_pi', _up.pi); 
			setValue('upload_data_unit', _up.unit);
			setValue('upload_data_pro_dev', _up.dt + "|" + _up.dtn);
			setValue('upload_data_pro_type', _up.pt);
			setValue('upload_data_desc', _up.desc); 
		}
		refreshOneProtoDevList(true,'upload_data_pro_dev','upload_data_pro_type');
	}
}

function setUpCfg()
{
	var _name = getValue('upload_data_name');
	var id = -1;
	
	for(var i = 0; i < xVarManageExtDataBase.length; i++) {
		if( _name == xVarManageExtDataBase[i].na ) {
			id = i;
			break;
		}
	}
	
	if( id >= 0 && id < EXT_VAR_LIMIT ) {
		var _upload_data_nid = getValue('upload_data_nid');
		var _upload_data_fid = getValue('upload_data_fid');
		var _upload_data_pi = getValue('upload_data_pi');
		var _upload_data_pro_dev = getValue('upload_data_pro_dev');
		var _upload_data_pro_type = getValue('upload_data_pro_type');
		
		//if (chk(_upload_data_nid, "关联Nid")) return;
		if (chk(_upload_data_fid, "关联Fid")) return;
		if (chk(_upload_data_pi, "小数点位数")) return;
		if (chk(_upload_data_pro_dev, "上传接口")) return;
		if (chk(_upload_data_pro_type, "上传协议")) return;
	
		var proto = getValue("upload_data_pro_dev").split("|");
		var _up = {
			en:getChecked('upload_data_enable')?1:0,
			nid:getValue('upload_data_nid'), 
			fid:getValue('upload_data_fid'), 
			pi:getNumber('upload_data_pi'), 
			unit:getValue('upload_data_unit'), 
			dt:Number(proto[0]), 
			dtn:Number(proto[1]), 
			pt:getNumber('upload_data_pro_type'), 
			desc:getValue('upload_data_desc')
		};
		var setval = {
			n:id, 
			'up':_up
		};
		
		if( xVarManageExtDataBase != null && id < xVarManageExtDataBase.length ) {
			if(xVarManageExtDataBase[id]['up']==null) xVarManageExtDataBase[id]['up'] = { };
			xVarManageExtDataBase[id]['up'].en=_up.en;
			xVarManageExtDataBase[id]['up'].nid=_up.nid;
			xVarManageExtDataBase[id]['up'].fid=_up.fid;
			xVarManageExtDataBase[id]['up'].unit=_up.unit;
			xVarManageExtDataBase[id]['up'].dt=_up.dt;
			xVarManageExtDataBase[id]['up'].dtn=_up.dtn;
			xVarManageExtDataBase[id]['up'].pt=_up.pt;
			xVarManageExtDataBase[id]['up'].pi=_up.pi;
			xVarManageExtDataBase[id]['up'].desc=_up.desc;
		}
		
		MyGetJSONWithArg("正在设置采集变量信息,请稍后...","/cgi-bin/setVarManageExtData?",JSON.stringify(setval), function (res) {
			if( res != null && 0 == res.ret ) {
				getAllVarManageExtDataBase(0);
				refreshAllUploadDataCfgBase(id);
				alert( "设置成功" );
			} else {
				alert("设置失败,请重试");
			}
		});
	} else {
		alert("请先在列表中，选择要修改的选项，再进行设置");
	}
}

/**end 上传数据配置功能***/

function Page_Xfer_Net_Cfg(n,cfg)
{
	//是否启用
	document.getElementsByName("xfer_net_radio_"+n)[0].checked = (Number(cfg.en)!=0);
	document.getElementsByName("xfer_net_radio_"+n)[1].checked = !(Number(cfg.en)!=0);

	setValue('xfer_net_type_'+n, cfg.tt);
	setValue("xfer_net_cs_"+n, cfg.cs );
	setValue("xfer_net_proto_type_"+n, cfg.pt);
	setValue('xfer_net_peer_'+n, cfg.pe);
	setValue('xfer_net_port_'+n, cfg.po);
	setValue('xfer_net_dst_type_'+n, cfg.dt);
	
	if( cfg.uty != null ) setValue('xfer_net_dst_uart_type_'+n, cfg.uty);
	if( cfg.ubr != null ) setValue('xfer_net_dst_uart_baudrate_'+n, cfg.ubr);
	if( cfg.udb != null && cfg.usb != null  && cfg.upy != null ) setValue('xfer_net_dst_uart_bits_'+n, (Number(cfg.udb) * 100) + (Number(cfg.upy) * 10) + Number(cfg.usb));
	
	setDisplay('xfer_net_cs_'+n+'_ip', getNumber('xfer_net_cs_'+n) == 0 );
}

function setXferNetCfg(n)
{
	var xfer_net_radio_a = document.getElementsByName("xfer_net_radio_"+n)[0].checked;
	var xfer_net_radio_b = document.getElementsByName("xfer_net_radio_"+n)[1].checked;
	var xfer_net_type_ = getValue('xfer_net_type_'+n);
	var xfer_net_cs_ = getValue('xfer_net_cs_'+n);
	var xfer_net_proto_type_ = getValue('xfer_net_proto_type_'+n);
	var xfer_net_peer_ = getValue('xfer_net_peer_'+n);
	var xfer_net_port_ = getValue('xfer_net_port_'+n);
	var xfer_net_dst_type_ = getValue('xfer_net_dst_type_'+n);
	//var xfer_net_dst_uart_type_ = getValue('xfer_net_dst_uart_type_'+n);
	var xfer_net_dst_uart_baudrate_ = getValue('xfer_net_dst_uart_baudrate_'+n);
	var xfer_net_dst_uart_bits_ = getValue('xfer_net_dst_uart_bits_'+n);

	if (chk2(xfer_net_radio_a, xfer_net_radio_b, "是否启用")) return;
	if( xfer_net_radio_a ) {
		if (chk(xfer_net_type_, "工作模式")) return;
		if (chk(xfer_net_cs_, "工作模式")) return;
		if (chk(xfer_net_proto_type_, "转发协议")) return;
		if( Number(xfer_net_cs_) == 0 ) {
			if (chk(xfer_net_peer_, "IP地址或域名")) return;
		}
		if (chk(xfer_net_port_, "端口号")) return;
		if (chk(xfer_net_dst_type_, "转发接口")) return;
		if( Number(xfer_net_dst_type_) >= 0 && Number(xfer_net_dst_type_) <= 4 ) {
			//if (chk(xfer_net_dst_uart_type_, "串口类型")) return;
			if (chk(xfer_net_dst_uart_baudrate_, "串口波特率")) return;
			if (chk(xfer_net_dst_uart_bits_, "串口数据位")) return;
		}
	}

	//注：在此处填写保存代码即可；
	var setval = {
		n:Number(n), 
		en:xfer_net_radio_a?1:0, 
		tt:Number(xfer_net_type_), 
		cs:Number(xfer_net_cs_), 
		pt:Number(xfer_net_proto_type_), 
		pe:xfer_net_peer_, 
		po:Number(xfer_net_port_), 
		dt:Number(xfer_net_dst_type_), 
		//uty:Number(xfer_net_dst_uart_type_), 
		ubr:Number(xfer_net_dst_uart_baudrate_), 
		udb:parseInt(xfer_net_dst_uart_bits_.charAt(0)), 
		upy:parseInt(xfer_net_dst_uart_bits_.charAt(1)), 
		usb:parseInt(xfer_net_dst_uart_bits_.charAt(2))
	};	
	
	MyGetJSONWithArg("正在设置转发配置","/cgi-bin/setXferNetCfg?",JSON.stringify(setval));
}

function numToString(num, xx, len) {
	var str_xx = '0000000000000000'+ num.toString(xx);
	if(str_xx.length >= len) {
		return str_xx.substring(str_xx.length - len, str_xx.length);
	}
}

function stringArrayToUint8Array_check(strings) {
	var index = arguments[1]?arguments[1]:0;
	var stringList = strings.split(' ');
	for (var i = index; i < stringList.length; i++) {
		var num = parseInt(stringList[i], 16);
		if( isNaN(num) ) {
			return false;
		}
	}
	return true;
}

function stringArrayToUint8Array(strings) {
	var index = arguments[1]?arguments[1]:0;
	var stringList = strings.split(' ');
	var uintArray = [];
	for (var i = index; i < stringList.length; i++) {
		var num = parseInt(stringList[i], 16);
		if( isNaN(num) ) {
			alert("非法内容:" + stringList[i]);
			return null;
		}
		uintArray.push(num);
	}
	return new Uint8Array(uintArray);
}

function stringToUint8Array(string) {
	var index = arguments[1]?arguments[1]:0;
	var charList = string.split('');
	var uintArray = [];
	for (var i = index; i < charList.length; i++) {
		var wchr = charList[i].charCodeAt(0);
		if(wchr<0x80){
			uintArray.push(wchr);
		} else if(wchr<0x800 ) {
			var c1=wchr&0xff;
			var c2=(wchr>>8)&0xff;
			uintArray.push(0xC0|(c2<<2)|((c1>>6)&0x3));
			uintArray.push(0x80|(c1&0x3F));
		} else {
			var c1=wchr&0xff;
			var c2=(wchr>>8)&0xff;
			uintArray.push(0xE0|(c2>>4));
			uintArray.push(0x80|((c2<<2)&0x3C)|((c1>>6)&0x3));
			uintArray.push(0x80|(c1&0x3F));
		}
	}
	return new Uint8Array(uintArray);
}

function uint8ArrayToString(array) {
	var index = arguments[1]?arguments[1]:0;
	var out,i,len, c1,c2,c3;
	out="";len=array.length;i=index;
	while(i<len){
		c1 = array[i++];
		switch(c1>>4){
		case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7: out+=String.fromCharCode(c1); break;
		case 12:case 13: c2=array[i++];out+=String.fromCharCode(((c1&0x1F)<<6)|(c2&0x3F)); break;
		case 14:c2=array[i++];c3=array[i++];out+=String.fromCharCode(((c1&0x0F)<<12)|((c2&0x3F)<<6)|((c3&0x3F)<<0)); break;
		}
	}
	return out;
}

function uint8ArrayToHexString(uintArray) {
	var index = arguments[1]?arguments[1]:0;
	var string = "";
	for(var i=index; i<uintArray.length; i++) {
	   var tmp = uintArray[i].toString(16);
	   if(tmp.length == 1) {
		   tmp = "0" + tmp;
	   }
	   string += ((i==index)?tmp:(" " + tmp));
	}
	return string;
}

//add by chenqq 
function showDialog(id){
	if (id == 'data_dialog') {
		applyVarExtInfo(1,0);
		refreshOneProtoDevList(false,'var_ext_pro_dev','var_ext_pro_type');
	}
	$('.theme-popover-mask').show();
	$('#'+id).show();
}

function hideDialog(id){
	$('.theme-popover-mask').hide();
	$('#'+id).hide();
}

//采集变量删除操作
function Del_data_collection(){
	var _val = $("#var_ext_name0").val();
	var _index = Number($("#var_ext_id").val());
	MyGetJSONWithArg("正在删除采集变量","/cgi-bin/delVarManageExtData?", JSON.stringify({"na":_val}), function(){
		//重新请求列表
		getAllVarManageExtDataBase(_index);
	});
}

function remote_work_cs_change() {
	var prefix = 'remote';
	setDisplay(prefix + '_working_cs_ip', getNumber(prefix + '_working_cs') == 0 );
	setDisplay(prefix + '_working_cs_interval', getNumber(prefix + '_working_cs') == 0 );
}

function remote_working_type_change() {
	var prefix = 'remote';
	setDisplay(prefix + '_wm_normal_ul', getNumber(prefix + '_working_type') == 0 );
	setDisplay(prefix + '_wm_xfer_ul', getNumber(prefix + '_working_type') == 1 );
}

function remote_proto_change() {
	var prefix = 'remote';
	setDisplay(prefix + '_proto_ms_addr', (getNumber(prefix + '_proto_ms')== 0)&&getNumber(prefix + '_proto')==PROTO_MODBUS_RTU_OVER_TCP);
}

function remote_xfer_gw_trt_dst_type_change() {
	var prefix = 'remote';
	var is_gw = getNumber(prefix + '_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber(prefix + '_xfer_mode')== XFER_M_TRT;
	var dst_type = -1;
	if(is_gw||is_trt) {
		dst_type = getNumber(prefix + '_xfer_gw_trt_dst_type');
	}
	setDisplay(prefix + '_xfer_gw_trt_dst_gprs_index_ul', (is_gw||is_trt)&&(dst_type==PROTO_DEV_GPRS));
	setDisplay(prefix + '_xfer_gw_trt_dst_uart_type_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay(prefix + '_xfer_gw_trt_dst_uart_baudrate_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay(prefix + '_xfer_gw_trt_dst_uart_bits_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
}

function remote_xfer_mode_change() {
	var prefix = 'remote';
	var is_gw = getNumber(prefix + '_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber(prefix + '_xfer_mode')== XFER_M_TRT;
	setDisplay(prefix + '_wm_xfer_proto_type_ul', is_gw);
	setDisplay(prefix + '_xfer_gw_trt_dst_type_ul', is_gw||is_trt);
	remote_xfer_gw_trt_dst_type_change();
}

function show_remote_btn(){
	var prefix = 'remote';
	setDisplay(prefix + '_dialog_btn', false);
	setDisplay(prefix + '_proto_ms_ul', true);
	setEnable(prefix + '_working_cs', true);
	setEnable(prefix + '_socket_type', true);
	setEnable(prefix + '_working_type', true);
	switch(getNumber(prefix + '_proto')) {
	case PROTO_CC_BJDC: case PROTO_HJT212: case PROTO_DM101: case PROTO_MQTT:
		setDisplay(prefix + '_dialog_btn', true);
		setEnable(prefix + '_working_cs', false);
		setEnable(prefix + '_socket_type', getNumber(prefix + '_proto') == PROTO_DM101);
		setEnable(prefix + '_working_type', false);
		setDisplay(prefix + '_proto_ms_ul', false);
		setValue(prefix + '_working_cs', 0);
		setValue(prefix + '_socket_type', 0);
		setValue(prefix + '_working_type', 0);
		break;
	case PROTO_SMF:
		setEnable(prefix + '_working_cs', false);
		setEnable(prefix + '_socket_type', false);
		setEnable(prefix + '_working_type', false);
		setDisplay(prefix + '_proto_ms_ul', false);
		setValue(prefix + '_working_cs', 1);
		setValue(prefix + '_socket_type', 0);
		setValue(prefix + '_working_type', 0);
		setValue(prefix + '_proto_ms', 1);
		break;
	case PROTO_DH:
		setDisplay(prefix + '_dialog_btn', true);
		setEnable(prefix + '_working_cs', false);
		setEnable(prefix + '_socket_type', false);
		setEnable(prefix + '_working_type', false);
		setDisplay(prefix + '_proto_ms_ul', false);
		setValue(prefix + '_working_cs', 0);
		setValue(prefix + '_socket_type', 0);
		setValue(prefix + '_working_type', 0);
		setValue(prefix + '_proto_ms', 1);
		break;
	}
	remote_work_cs_change();
	remote_working_type_change();
	remote_proto_change();
	remote_xfer_mode_change();
}
//通讯协议弹窗
function show_remote_dialog(){
		var prefix = 'remote';
		setFileToTextarea();
		showDialog(prefix + '_dialog');

		$('#dialog_' + prefix + '_ok_btn').off().on("click", function(){
			saveRemoteConfig();
			hideDialog(prefix + '_dialog');
		});
}

function setFileToTextarea()
{
	if(getNumber('remote_proto')==PROTO_CC_BJDC) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_cc_bjdc_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	} else if(getNumber('remote_proto')==PROTO_HJT212) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_hjt212_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	} else if(getNumber('remote_proto')==PROTO_DM101) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_dm101_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	} else if(getNumber('remote_proto')==PROTO_MQTT) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_mqtt_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	} else if(getNumber('remote_proto')==PROTO_DH) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_dh_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	}
}

function saveRemoteConfig()
{
	var _val = $("#remote_config_textarea").val();
	/*alert(_val);
	$.ajax({
		type: "post",
		url: "/ini/upload/" + CONFIG_PATH + "/rtu_cc_bjdc_"+getValue('remote_group')+".ini",
		data:_val,
		dataType: "html",
		async: true,
		cache: false,
		success: function(data){
			 alert("保存成功，重启生效！");
		}
	}); */
	
	var xhr = createXMLHttpRequest();
	if (xhr) {
		Show("正在保存配置,请稍后...");
		xhr.onreadystatechange = function() {
			if( xhr.readyState==4 ) {
				Close();
				if( xhr.status==200 ) {
					alert("保存成功，重启生效！");
				} else {
					alert("失败,请重试");
				}
			}
		}
		if(getNumber('remote_proto')==PROTO_CC_BJDC) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_cc_bjdc_"+getValue('remote_group')+".ini" );
		} else if(getNumber('remote_proto')==PROTO_HJT212) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_hjt212_"+getValue('remote_group')+".ini" );
		} else if(getNumber('remote_proto')==PROTO_DM101) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_dm101_"+getValue('remote_group')+".ini" );
		} else if(getNumber('remote_proto')==PROTO_MQTT) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_mqtt_"+getValue('remote_group')+".ini" );
		} else if(getNumber('remote_proto')==PROTO_DH) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_dh_"+getValue('remote_group')+".ini" );
		}
		xhr.send(_val);
	}
}
 function ajaxUpload(options) {
		var feature = {};
		feature.fileapi = $("<input type='file'/>").get(0).files !== undefined;
		feature.formdata = window.FormData !== undefined;

		options = options || {};
		options.type = (options.type || "GET").toUpperCase();
		options.dataType = options.dataType || "json";
		var params = formatParams(options.data);

		//创建 - 非IE6 - 第一步
		if (window.XMLHttpRequest) {
			var xhr = new XMLHttpRequest();
		} else { //IE6及其以下版本浏览器
			var xhr = new ActiveXObject('Microsoft.XMLHTTP');
		}

		//接收 - 第三步
		xhr.onreadystatechange = function () {
			if (xhr.readyState == 4) {
				var status = xhr.status;
				if (status >= 200 && status < 300) {
					options.success && options.success(xhr.responseText, xhr.responseXML);
				} else {
					options.error && options.error(status);
				}
			}
		}

		//连接 和 发送 - 第二步
		if (options.type == "GET") {
			xhr.open("GET", options.url + "?" + params, true);
			xhr.send(null);
		} else if (options.type == "POST") {
			options.xhr = xhr;
			//设置表单提交时的内容类型
			if(!feature.formdata){
				fileUploadIframe(options)
			}else{
				fileUploadXhr(options);
			}
		　　
		}
	}
//格式化参数
function formatParams(data) {
	var arr = [];
	for (var name in data) {
		arr.push(encodeURIComponent(name) + "=" + encodeURIComponent(data[name]));
	}
	arr.push(("v=" + Math.random()).replace(".",""));
	return arr.join("&");
}

function fileUploadXhr(s){
	 var form = document.getElementById(s.formId);
	 var formData = new FormData(form);
	  s.url = form.action;
 
		if (s.xhr.upload) {
			s.xhr.upload.onprogress = function(event) {
				var percent = 0;
				var position = event.loaded || event.position; /*event.position is deprecated*/
				var total = event.total;
				if (event.lengthComputable) {
					percent = Math.ceil(position / total * 100);
				}
				s.uploadProgress(event, position, total, percent);
			};
		}
		s.xhr.open('POST', form.action);
		s.xhr.send(formData);
}

/**ie低版本浏览器上传文件**/
function fileUploadIframe(options){
	var form = document.getElementById(options.formId);
	form.submit();
	/* var uid = new Date().getTime(),idIO='jUploadFrame'+uid,_this=this;
	var jIO=$('<iframe name="'+idIO+'" id="'+idIO+'" style="display:block">').appendTo('body');
	$(form).attr("target",idIO);

	function onLoad(){  
			if (options.success)  
				options.success();  

			if (options.complete)  
				options.complete();  
			 
		}  

		try{
			form.submit() 
		} catch(e){
			options.error(); 
		} ;

	 var io = document.getElementById(idIO);
	io.onload = function(){ 
		onLoad();
	} 

  
	setTimeout(function()  
		{   try   
			{  
				$(io).remove();  
				$(form).removeAttr("target");	
			} catch(e)   
			{  
				console.log("异常"); 
			}									 

		}, 1000); 
*/
}

function date_check(_d) {
	var date = _d;
	var result = date.match(/^(\d{1,4})(-|\/)(\d{1,2})\2(\d{1,2})$/);

	if (result == null)
		return false;
	var d = new Date(result[1], result[3] - 1, result[4]);
	return (d.getFullYear() == result[1] && (d.getMonth() + 1) == result[3] && d.getDate() == result[4]);

}

function __to_timestamp(_d)
{
	var _t = Date.parse(new Date(_d)) / 1000;
	return parseInt(_t & 0xFFFFFFFF);
}

function format_log_lvl(lvl)
{
	switch(lvl) {
	case 0: return "异常";
	case 1: return "错误";
	case 2: return "警告";
	case 3: return "信息";
	case 4: return "调试";
	case 5: return "详细";
	}
	
	return "未知";
}

function refresh_rtu_log(loglst)
{
	myTableItemRemoveAll('rtu_log_table');
	var table = window.document.getElementById("rtu_log_table");
	for(var n = 0; n < loglst.length; n++) {
		var row = table.insertRow(table.rows.length);
		row.style.height="25px";
		var obj = row.insertCell(0);
		obj.innerHTML = loglst[n]['n'];
		obj = row.insertCell(1);
		obj.innerHTML = loglst[n]['t'];
		obj = row.insertCell(2);
		obj.innerHTML = format_log_lvl(loglst[n]['l']);
		obj = row.insertCell(3);
		obj.innerHTML = loglst[n]['g'];
	}
}

function rtu_log_delete()
{
	if (confirm("确定清空？\n\n 该操作不可恢复！")){
		MyGetJSONWithArg("正在清空设备日志,请稍后...","/cgi-bin/delLogData?", "", function (res) {
			if( res != null && 0 == res.ret ) {
				alert("清空设备日志成功！");
			} else {
				alert("清空设备日志失败,请重试");
			}
		});
	}
}

var _rtu_log_page = 0;

function rtu_log_download_with_page(page)
{
	var start_time = 0;
	var end_time = -1;
	
	var log_start_time = getValue('log_start_time');
	var log_end_time = getValue('log_end_time');
	
	if(log_start_time.length > 0 && !date_check(getValue('log_start_time'))) {
		alert("请输入正确格式的开始日期(例：2017-01-01)");
		return ;
	}
	if(log_start_time.length > 0) start_time = __to_timestamp(log_start_time);
	if(log_end_time.length > 0 && !date_check(getValue('log_end_time'))) {
		alert("请输入正确格式的结束日期(例：2017-01-02)");
		return ;
	}
	if(log_end_time.length > 0) end_time = __to_timestamp(log_end_time);
	
	var setval = {
		'start':start_time, 
		'end':end_time, 
		'page':_rtu_log_page
	}
	
	MyGetJSONWithArg("正在查询日志,请稍后...","/cgi-bin/getLogData?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			if (res.logs.length > 0) {
				refresh_rtu_log(res.logs);
			} else {
				//setEnable('rtu_log_prev', true);
				//setEnable('rtu_log_next', false);
				_rtu_log_page--;
				alert("别点啦, 已经是最后一页了！");
			}
			window.document.getElementById('rtu_log_page').innerHTML = '第' + (_rtu_log_page + 1) + '页';
		} else {
			alert("查询失败,请重试");
		}
	});
}

function rtu_log_download()
{
	_rtu_log_page = 0;
	//setEnable('rtu_log_prev', false);
	//setEnable('rtu_log_next', true);
	rtu_log_download_with_page(_rtu_log_page);
}


function rtu_log_download_page(n)
{
	_rtu_log_page+=n;
	if (_rtu_log_page < 0) {
		_rtu_log_page = 0;
	}
	//setEnable('rtu_log_prev', _rtu_log_page != 0);
	//setEnable('rtu_log_next', true);
	rtu_log_download_with_page(_rtu_log_page);
}

function check_none_gprs_model()
{
	if (__none_gprs()) {
		document.getElementById("gprs_menu_title").innerText="GPRS配置(未检测到模块)";
		document.getElementById("txtMonitorParameter")[4].innerText="GPRS(未检测到模块)";
		document.getElementById("gprs_net_info_title").innerText="GPRS网络(未检测到模块)";
		document.getElementById("gprs_net_status_title").innerText="GPRS连接状态(未检测到模块)";
		document.getElementById("net_adapter")[1].innerText="GPRS网络(未检测到模块)";
	}
}

var rule_add_flag = false;
var rule_name_bak = "";

function rule_type_format(type)
{
	switch(type) {
	case 0: return "系统事件";
	case 1: return "控制事件";
	case 2: return "短信事件";
	case 3: return "无输出";
	default: return "";
	}
}

function rule_add_init()
{
	rule_add_flag = true;
	setCheckBoxEnable('rule_enable', true);//启用
	setValue('rule_name', '');
	setValue('rule_in_param', '');
	setValue('rule_input', '');
	setValue('rule_type', RULE_TYPE_NONE);
	setValue('rule_out_param', '');
	setValue('rule_output', '');
}

function rule_onchange_type()
{
	var _type = getNumber('rule_type');
	setDisplay('rule_out_div', _type != RULE_TYPE_NONE);
}

function get_all_rules(_name)
{
	MyGetJSONWithArg("正在获取规则表,请稍后...","/cgi-bin/getRuleList?", "", function (res) {
		if( res != null && 0 == res.ret ) {
			xRuleList = res.list.concat();
			refresh_all_rules(_name);
		} else {
			alert("获取失败,请重试");
		}
	});
}

function rule_tab_add_item(enable,name,type,p_in,c_in,p_out,c_out,sys)
{
	var table = window.document.getElementById("rtu_rule_table");
	var row = table.insertRow(table.rows.length);
	row.style.height="25px";
	row.onclick = function(){ onRuleTableItemClick( table, row ); };
	row.ondblclick = function(){ onRuleTableItemDbClick( table, row ); };
	obj = row.insertCell(0);
	obj.innerHTML = enable!=0?"启用":"未启用";
	var obj = row.insertCell(1);
	obj.innerHTML = table.rows.length-1;
	obj = row.insertCell(2);
	obj.innerHTML = name;
	obj = row.insertCell(3);
	obj.innerHTML = p_in;
	obj = row.insertCell(4);
	obj.innerHTML = c_in;
	obj = row.insertCell(5);
	obj.innerHTML = rule_type_format(type);
	obj = row.insertCell(6);
	obj.innerHTML = p_out;
	obj = row.insertCell(7);
	obj.innerHTML = c_out;
}

function onRuleTableItemClick(tb,row)
{
	if( row != null && row.rowIndex != null && row.rowIndex >= 0 ) {
		for (var i = 0; i < tb.rows.length; i++) {
			if( xRuleList[i].en > 0 ) {
				tb.rows[i].style.background="#FFFFFF";
			} else {
				tb.rows[i].style.background="#F0F0F0";
			}
		}
		row.style.background="#E5E5E5";
		
		var _rowIndex = row.rowIndex;
		setCheckBoxEnable('rule_enable',xRuleList[_rowIndex].en);//启用
		setValue('rule_name', xRuleList[_rowIndex].name);
		setValue('rule_in_param', xRuleList[_rowIndex].p_in);
		setValue('rule_input', xRuleList[_rowIndex].c_in);
		setValue('rule_type', xRuleList[_rowIndex].type);
		setValue('rule_out_param', xRuleList[_rowIndex].p_out);
		setValue('rule_output', xRuleList[_rowIndex].c_out);
		
		rule_onchange_type();
	}
}

function onRuleTableItemDbClick(tb,row)
{
	rule_add_flag = false;
	onRuleTableItemClick(tb,row);
	rule_name_bak = getValue('rule_name');
	showDialog('rule_dialog');
}

function rule_has_name(_name)
{
	if(xRuleList != null && xRuleList.length > 0) {
		for(var n = 0; n < xRuleList.length; n++) {
			if( xRuleList[n].name != null && xRuleList[n].name == _name ) {
				return n;
			}
		}
	}
	return -1;
}

function refresh_all_rules(name)
{
	myTableItemRemoveAll("rtu_rule_table");
	for( var n = 0; n < xRuleList.length; n++ ) {
		var _en = xRuleList[n].en;
		var _name = xRuleList[n].name;
		var _type = xRuleList[n].type;
		var _p_in = xRuleList[n].p_in;
		var _c_in = xRuleList[n].c_in;
		var _p_out = xRuleList[n].p_out;
		var _c_out = xRuleList[n].c_out;
		var _sys = xRuleList[n].sys;
		
		rule_tab_add_item(_en, _name, _type, _p_in, _c_in, _p_out, _c_out, _sys);
	}
	var table = window.document.getElementById("rtu_rule_table");
	var index = (name != null ? rule_has_name(name) : 0);
	onRuleTableItemClick(table, table.rows[index >= 0?index:0]);
}

function rule_set_or_add()
{
	var _en = getChecked('rule_enable');
	var _name = getValue('rule_name');
	var _p_in = getValue('rule_in_param');
	var _c_in = getValue('rule_input');
	var _type = getNumber('rule_type');
	var _p_out = getValue('rule_out_param');
	var _c_out = getValue('rule_output');
	
	
	if (chk(_name, "规则名称")) return -1;
	if (chk(_c_in, "控制规则")) return -1;
	
	if (_type == RULE_TYPE_CTRL) {
		if (chk(_c_out, "输出规则")) return -1;
	}
	
	if (_type == RULE_TYPE_NONE) {
		_p_out = "";
		_c_out = "";
		setValue('rule_out_param', '');
		setValue('rule_output', '');
	}
	
	var setval = {
		en:_en?1:0, 
		name:_name, 
		type:_type, 
		p_in:_p_in, 
		c_in:_c_in, 
		p_out:_p_out, 
		c_out:_c_out
	};
	
	if (rule_add_flag) {
		if (rule_has_name(_name) >= 0) {
			alert("规则名称重复，请检查！");
			return -1;
		}
	} else {
		if (rule_name_bak != _name) {
			alert("规则名称变化，将新建一个并保留原有规则，如需删除请手动删除！");
		}
	}
	
	MyGetJSONWithArg("正在设置规则信息,请稍后...", rule_add_flag ? "/cgi-bin/addRule?" : "/cgi-bin/setRule?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			get_all_rules(_name);
			alert( "设置成功" );
		} else {
			alert("设置失败,请重试");
		}
	});
	
	return 0;
}

function rule_del()
{
	 var _name = $("#rule_name").val();
	 MyGetJSONWithArg("正在删除规则","/cgi-bin/delRule?", JSON.stringify({"name":_name}), function(){
		get_all_rules(null);
	 });
}

function __rule_param_in(name, n)
{
	if (name != null && name.length > 1) {
		var id = "rule_param_in_" + n;
		return "<ul class='detail'><li class='detail_l'>[I]" + name + "=</li><li class='detail_r'><input type='text' id='" + id + "'/></li></ul>"
	} else {
		return "<ul class='detail'><li class='detail_l'></li><li class='detail_r'></li></ul>"
	}
}

function __rule_param_out(name, n)
{
	if (name != null && name.length > 1) {
		var id = "rule_param_out_" + n;
		return "<ul class='detail'><li class='detail_l'>[O]" + name + "=</li><li class='detail_r'><input type='text' id='" + id + "'/></li></ul>"
	} else {
		return "<ul class='detail'><li class='detail_l'></li><li class='detail_r'></li></ul>"
	}
}

function rule_create_param_div()
{
	var html_str = "";
	var _rule_name = getValue('var_ext_exp_rule');
	if (_rule_name != null && _rule_name.length > 0) {
		var n = rule_has_name(_rule_name);
		
		if (n >= 0) {
			var p_in = new Array();
			var p_out = new Array();
			p_in = xRuleList[n].p_in.split(",");
			p_out = xRuleList[n].p_out.split(",");
			
			var n_max = Math.max(p_in.length, p_out.length);
			for (var n = 0; n < n_max; n++ ) {
				html_str += __rule_param_in(n < p_in.length ? p_in[n] : null, n);
				html_str += __rule_param_out(n < p_out.length ? p_out[n] : null, n);
			}
		}
	}
	document.getElementById('ext_exp_rule_param_div').innerHTML = html_str;
	if (!var_ext_add_flag && var_ext_rule_name_bak == _rule_name) {
		var_ext_exp_rule_string_toparam('rule_param_in', var_ext_rule_p_in_bak);
		var_ext_exp_rule_string_toparam('rule_param_out', var_ext_rule_p_out_bak);
	}
}

function rule_create_select_list(_id)
{
	var selectobj = window.document.getElementById(_id);
	if(selectobj==null) return ;
	if (xRuleList != null && xRuleList.length > 0) {
		selectobj.options.length = 0;
		for(var i = 0; i < xRuleList.length; i++) {
			if (xRuleList[i].en) {
				var _name = xRuleList[i].name;
				selectobj.options.add(new Option(_name, _name));
			}
		}
	}
}

function var_ext_exp_type_change(_rule_name)
{
	var _type = getNumber('var_ext_exp_type');
	if (_type == IO_EXP_TYPE_EXP) {
		setDisplay('ext_exp_exp_div', true);
		setDisplay('ext_exp_rule_div', false);
	} else if (_type == IO_EXP_TYPE_RULE) {
		setDisplay('ext_exp_exp_div', false);
		setDisplay('ext_exp_rule_div', true);
		rule_create_select_list('var_ext_exp_rule');
		if (_rule_name == null) {
			_rule_name = getValue('var_ext_exp_rule');
		} else {
			setValue('var_ext_exp_rule', _rule_name);
		}
		rule_create_param_div();
	}
}

function var_ext_exp_rule_param_tostring(id_prefix)
{
	var pstr = "";
	var n = 0;
	while (1) {
		var id_0 = id_prefix + "_" + n;
		var id_1 = id_prefix + "_" + (n + 1);
		if (document.getElementById(id_1)) {
			pstr += getValue(id_0) + ',';
		} else if (document.getElementById(id_0)) {
			pstr += getValue(id_0);
			break;
		} else {
			break;
		}
		n++;
	}
	return pstr;
}

function var_ext_exp_rule_string_toparam(id_prefix, pstr)
{
	var params = new Array();
	params = pstr.split(",");
	for(var n = 0; n < params.length; n++) {
		setValue(id_prefix + "_" + n, params[n]);
	}
}

var _refresh_status_timer = null;
var _search_setval = {
	'dt':0, 
	'dtn':0
}
var _search_result = new Array();

function var_ext_dev_type0_format(type0)
{
	if (type0 == VAR_DEV_TYPE0_AI) return "AI模块";
	if (type0 == VAR_DEV_TYPE0_TTL) return "TTL模块";
}

function var_ext_dev_type1_format(type0, type1)
{
	if (type0 == VAR_DEV_TYPE0_AI) {
		return "";
	} else if (type0 == VAR_DEV_TYPE0_TTL) {
		if (type1 == VAR_DEV_TYPE1_TTL_4DI_4DO) return "4DI_4DO";
		if (type1 == VAR_DEV_TYPE1_TTL_4DO_4DI) return "4DO_4DI";
		if (type1 == VAR_DEV_TYPE1_TTL_8DI) return "8DI";
		if (type1 == VAR_DEV_TYPE1_TTL_8DO) return "8DO";
	}
}

function varExtSearchTableAddItem(sn,type0,type1,slaveaddr,btn_flag)
{
	var index = 0;
	var table = window.document.getElementById("rtu_var_ext_search_table");
	var row = table.insertRow(table.rows.length);
	row.style.height="25px";
	var obj = row.insertCell(index++);
	obj.innerHTML = table.rows.length-1;
	obj = row.insertCell(index++);
	obj.innerHTML = sn;
	obj = row.insertCell(index++);
	obj.innerHTML = var_ext_dev_type0_format(type0);
	obj = row.insertCell(index++);
	obj.innerHTML = var_ext_dev_type1_format(type0, type1);
	obj = row.insertCell(index++);
	obj.innerHTML = slaveaddr;
	obj = row.insertCell(index++);
	if (btn_flag) {
		obj.innerHTML = "<button class='dialog_ok_btn' onclick=\"" + "var_ext_add_group('" + sn + "'," + type0 + "," + type1 + "," + slaveaddr + ");\"> 添加设备 </button>";
	} else {
		obj.innerHTML = "";
	}
}

var _refresh_status_timer_count = 0;

function _refresh_status_timer_hook() {
	MyGetJSONWithArg("","/cgi-bin/searchVarManageExtGroupStatus?", "", function (res) {
		if( res != null && 0 == res.ret ) {
			setDisplay('var_ext_dev_search_status', true);
			if (res.pos <= 0) {
				window.document.getElementById('var_ext_dev_search_status').innerText = "0%";
			} else if (res.pos <= 247) {
				window.document.getElementById('var_ext_dev_search_status').innerText = (parseInt(res.pos * 100 / 247) + "%");
				if (++_refresh_status_timer_count % 5 == 0) {
					MyGetJSONWithArg("","/cgi-bin/searchVarManageExtGroupResult?", JSON.stringify(_search_setval), function (res) {
						myTableItemRemoveAll("rtu_var_ext_search_table");
						_search_result = res.list;
						for( var n = 0; n < _search_result.length; n++ ) {
							varExtSearchTableAddItem( 
								_search_result[n].sn, 
								_search_result[n].dtp0, 
								_search_result[n].dtp1, 
								_search_result[n].sa,
								false
							);
						}
					});
				}
			} else {
				window.document.getElementById('var_ext_dev_search_status').innerText = "100%";
				setDisplay('var_ext_dev_search_btn', true);
				setEnable('var_ext_search_dev', true);
				window.clearInterval(_refresh_status_timer);
				_refresh_status_timer = null;
				
				MyGetJSONWithArg("","/cgi-bin/searchVarManageExtGroupResult?", JSON.stringify(_search_setval), function (res) {
					myTableItemRemoveAll("rtu_var_ext_search_table");
					_search_result = res.list;
					for( var n = 0; n < _search_result.length; n++ ) {
						varExtSearchTableAddItem( 
							_search_result[n].sn, 
							_search_result[n].dtp0, 
							_search_result[n].dtp1, 
							_search_result[n].sa,
							true
						);
					}
				});
			}
		}
	});
}

function var_ext_start_add_group_init()
{
	if (_refresh_status_timer != null) window.clearInterval(_refresh_status_timer);
	_refresh_status_timer = null;
	setEnable('var_ext_search_dev', true);
	setDisplay('var_ext_dev_search_btn', true);
	setDisplay('var_ext_dev_search_status', false);
	myTableItemRemoveAll("rtu_var_ext_search_table");
}

function var_ext_start_search()
{
	var protoDevList = window.document.getElementById('var_ext_search_dev');
	if(protoDevList.options.length > 0 ) {
		var devary = protoDevList.value.split("|");
		if(devary.length==2) {
			_search_setval.dt = Number(devary[0]);
			_search_setval.dtn = Number(devary[1]);
			if (_refresh_status_timer != null) window.clearInterval(_refresh_status_timer);
			_refresh_status_timer = null;
			_refresh_status_timer_count = 0;
			if (_search_setval.dt == PROTO_DEV_LORA) {
				MyGetJSONWithArg("","/cgi-bin/getLoRaList?","", function (info)
				{
					myTableItemRemoveAll("rtu_var_ext_search_table");
					for( var n = 0; n < info.list.length; n++ ) {
						var sa = info.list[n].adlst.replace('[', '').replace(']', '');
						if (sa.length > 0) {
							varExtSearchTableAddItem( 
								info.list[n].id, 
								info.list[n].type0, 
								info.list[n].type1, 
								Number(sa),
								true
							);
						}
					}
					window.document.getElementById('var_ext_dev_search_status').innerText = "100%";
					setDisplay('var_ext_dev_search_btn', true);
					setEnable('var_ext_search_dev', true);
				});
			} else {
				MyGetJSONWithArg("开始扫描...","/cgi-bin/searchVarManageExtGroupStart?", JSON.stringify(_search_setval), function (res) {
					if( res != null && 0 == res.ret ) {
						window.document.getElementById('var_ext_dev_search_status').innerText = "0%";
						setDisplay('var_ext_dev_search_status', true);
						setDisplay('var_ext_dev_search_btn', false);
						setEnable('var_ext_search_dev', false);
						myTableItemRemoveAll("rtu_var_ext_search_table");
						_refresh_status_timer = window.setInterval(_refresh_status_timer_hook, 800);
					} else {
						alert("开始扫描失败,请重试");
					}
				});
			}
		}
	}
}

function var_ext_add_group(sn, type0, type1, slaveaddr)
{
	var prefix = prompt("请输入变量前缀(如：设备1, 则会生成对应的变量名: 设备1_AI_0)", "")
	if (prefix != null && prefix != "") {
		var setval = {
			'dt':_search_setval.dt, 
			'dtn':_search_setval.dtn,
			'dtp0':type0,
			'dtp1':type1,
			'sa':slaveaddr,
			'sn':sn,
			'prefix':prefix
		}
		MyGetJSONWithArg("添加设备...","/cgi-bin/addVarManageExtGroup?", JSON.stringify(setval), function (res) {
			if( res != null && 0 == res.ret ) {
				//重新请求列表
				getAllVarManageExtDataBase(0);
				alert("添加设备成功!");
			} else {
				alert("添加失败：" + __format_err_code(res.ret));
			}
		});
	} else {
		alert("变量前缀为空，请重新添加！！！！");
	}
}

function Del_data_group(){
	var _val = $("#var_ext_name0").val();
	var _index = Number($("#var_ext_id").val());
	if (confirm("确定删除变量组:" + xVarManageExtDataBase[_index].gp)) {
		MyGetJSONWithArg("正在删除变量组","/cgi-bin/delVarManageExtGroup?", JSON.stringify({"gp":xVarManageExtDataBase[_index].gp}), function(){
			//重新请求列表
			getAllVarManageExtDataBase(0);
		});
	}
}

function var_ext_do_set(val)
{
	var n = var_ext_select_item_index;
	if (n >= 0 && n < xVarManageExtDataBase.length && xVarManageExtDataBase[n].attr == VAR_ATTR_DO) {
		var setval = {
			'n':n, 
			'val':val
		}
		MyGetJSONWithArg("正在操作","/cgi-bin/setVarManageDoValue?", JSON.stringify(setval), function(){
			getAllVarManageExtDataBase(n);
		});
	}
}

function getVarManageAiValue()
{
	var n = var_ext_select_item_index;
	if (n >= 0 && n < xVarManageExtDataBase.length && xVarManageExtDataBase[n].attr == VAR_ATTR_AI && 
		!($("#data_dialog").is(":hidden")) && 
		!($("#dataAttrtab4").hasClass("hide"))) {
		var setval = {
			'n':n
		}
		MyGetJSONWithArg("","/cgi-bin/getVarManageAIValue?", JSON.stringify(setval), function(res){
			if( res != null && 0 == res.ret ) {
				setValue('Input_Range_Engineering', res.eng);
				setValue('Input_Range_Electrical', res.meas.toFixed(4));
			}
		});
	}
}

function download_dev_log_file(fname)
{
	var a = document.createElement('a');
	a.download = fname;
	a.href = 'http://'+window.location.host + "/download/media/nand/monitor/" + fname;
	a.click();
}

function del_dev_log_file(fname)
{
	if (confirm("确定删除(" + fname + ")？\n\n 该操作不可恢复！")) {
		MyGetJSON("正在删除日志文件","/cgi-bin/delFile?", 'path', "/media/nand/monitor/" + fname, function(res){
			if( res != null && 0 == res.ret ) {
				show_dev_logs_list();
			}
		});
	}
}

function dev_log_table_add_item(fname,fsize,mtime)
{
	var table = window.document.getElementById("dev_logs_table");
	var row = table.insertRow(table.rows.length);
	obj = row.insertCell(0);
	obj.innerHTML = _tostr_(fname);
	var obj = row.insertCell(1);
	obj.innerHTML = _tostr_(fsize);
	obj = row.insertCell(2);
	obj.innerHTML = _tostr_(mtime);
	obj = row.insertCell(3);
	obj.innerHTML = "<input type='button' style='width:60px;text-indent:0;' value='下载' class='width_45 height_25' class='text_center' onclick='download_dev_log_file(\"" + fname + "\")' />" + 
					"<input type='button' style='width:60px;text-indent:0;' value='删除' class='width_45 height_25' class='text_center' onclick='del_dev_log_file(\"" + fname + "\")' />";
}

function show_dev_logs_list()
{
	MyGetJSON("正在获取端口日志列表","/cgi-bin/listDir?", 'path', "/media/nand/monitor" ,function( info ){
		if (info.list!=null) {
			var log_file_list = info.list;
			log_file_list = log_file_list.sort(function compareFunction(item1, item2) {
				return item1.mtime.localeCompare(item2.mtime);
			});
			myTableItemRemoveAll('dev_logs_table');
			for(var i = 0; i < log_file_list.length; i++) {
				if (log_file_list[i].type == 'file') {
					var monitor_filter = getNumber('monitor_filter');
					var add_flag = false;
					if (monitor_filter == 0) {
						add_flag = true;
					} else if (monitor_filter == 1) {
						add_flag = (log_file_list[i].name.indexOf('COM1_') >= 0);
					} else if (monitor_filter == 2) {
						add_flag = (log_file_list[i].name.indexOf('COM2_') >= 0);
					}
					if (add_flag) {
						dev_log_table_add_item(log_file_list[i].name, log_file_list[i].size, log_file_list[i].mtime);
					}
				}
			}
		}
	});
}

function dev_logs_table_refresh()
{
	show_dev_logs_list();
}

function monitor_get_cfg()
{
	MyGetJSONWithArg("","/cgi-bin/get_monitor_cfg?","", function (res)
	{
		if (res != null && 0 == res.ret) {
			setCheckBoxEnable('monitor_com1', res.com1);
			setCheckBoxEnable('monitor_com2', res.com2);
		} else {
			alert("设置成功，请重试！");
		}
	});
}

function monitor_set_cfg()
{
	var setval = {
		'com1':getChecked('monitor_com1') ? 1 : 0, 
		'com2':getChecked('monitor_com2') ? 1 : 0, 
	};
	MyGetJSONWithArg("正在设置...","/cgi-bin/set_monitor_cfg?", JSON.stringify(setval), function (res) {
		if (res != null && 0 == res.ret) {
			alert("设置成功!");
		} else {
			alert("设置成功，请重试！");
		}
	});
}

function ext_new_onclick()
{
	addVarExtInfo();
	var_ext_add_init();
	showDialog('data_dialog');
	ext_onchange_proto(true);
	ext_oncheck_self_dev();
	onVarExtVartypeChange('var_ext_vartype0',false);
	onVarExtVartypeChange('var_ext_vartype1',false);
	onVarExtVartypeChange('var_ext_out_vartype',false);
	var_dh_type_change();
}

function var_dh_type_change()
{
	var _value = getNumber('var_ext_dh_type');
	var _keys = [];
	var _descs = [];
	if (_value == "256") {
		_keys = type_256_key;
		_descs = type_256_desc;
	} else if (_value == "512") {
		_keys = type_512_key;
		_descs = type_512_desc;
	} else if (_value == "1280") {
		_keys = type_1280_key;
		_descs = type_1280_desc;
	} else if (_value == "1536") {
		_keys = type_1536_key;
		_descs = type_1536_desc;
	} else if (_value == "1792") {
		_keys = type_1792_key;
		_descs = type_1792_desc;
	} else if (_value == "2304") {
		_keys = type_2304_key;
		_descs = type_2304_desc;
	}
	
	var _key_select = window.document.getElementById('var_ext_dh_key');
	if (_key_select == null) return ;
	if (_keys != null && _keys.length > 0) {
		_key_select.options.length = 0;
		for(var i = 0; i < _keys.length; i++) {
			_key_select.options.add(new Option(_descs[i], _keys[i]));
		}
	}
}



