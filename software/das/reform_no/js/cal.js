
//  �����ú����� setday(this,[object])��setday(this)��[object]�ǿؼ�����Ŀؼ��������������ӣ�
//  һ��<input name=txt><input type=button value=setday onclick="setday(this,document.all.txt)">
//  ����<input onfocus="setday(this)">

var bMoveable=true;
var strFrame;  

document.writeln('<iframe id=endDateLayer frameborder=0 width=162 height=211 style="position: absolute;  z-index: 9998; display: none"></iframe>');
strFrame='<style>';
strFrame+='INPUT.button{BORDER-RIGHT: #63A3E9 1px solid;BORDER-TOP: #63A3E9 1px solid;BORDER-LEFT: #63A3E9 1px solid;';
strFrame+='BORDER-BOTTOM: #63A3E9 1px solid;BACKGROUND-COLOR: #63A3E9;font-family:����;}';
strFrame+='TD{FONT-SIZE: 9pt;font-family:����;}';
strFrame+='</style>';
strFrame+='<scr' + 'ipt>';
strFrame+='var datelayerx,datelayery;';
strFrame+='var bDrag;';
strFrame+='function document.onmousemove()';
strFrame+='{if(bDrag && window.event.button==1)';
strFrame+=' {var DateLayer=parent.document.all.endDateLayer.style;';
strFrame+='  DateLayer.posLeft += window.event.clientX-datelayerx;';
strFrame+='  DateLayer.posTop += window.event.clientY-datelayery;}}';
strFrame+='function DragStart()';
strFrame+='{var DateLayer=parent.document.all.endDateLayer.style;';
strFrame+=' datelayerx=window.event.clientX;';
strFrame+=' datelayery=window.event.clientY;';
strFrame+=' bDrag=true;}';
strFrame+='function DragEnd(){';
strFrame+=' bDrag=false;}';
strFrame+='</scr' + 'ipt>';
strFrame+='<div style="z-index:9999;position: absolute; left:0; top:0;" onselectstart="return false">';
strFrame+='<span id=tmpSelectYearLayer  style="z-index: 9999;position: absolute;top: 3; left: 19;display: none"></span>';
strFrame+='<span id=tmpSelectMonthLayer  style="z-index: 9999;position: absolute;top: 3; left: 78;display: none"></span>';
strFrame+='<span id=tmpSelectHourLayer  style="z-index: 9999;position: absolute;top: 188; left: 35px;display: none"></span>';
strFrame+='<span id=tmpSelectMinuteLayer style="z-index:9999;position:absolute;top: 188; left: 77px;display: none"></span>';
strFrame+='<span id=tmpSelectSecondLayer style="z-index:9999;position:absolute;top: 188; left: 119px;display: none"></span>';
strFrame+='<table border=1 cellspacing=0 cellpadding=0 width=142 height=160 bordercolor=#63A3E9 bgcolor=#63A3E9 >';
strFrame+='	<tr><td width=142 height=23  bgcolor=#FFFFFF>';
strFrame+='		<table border=0 cellspacing=1 cellpadding=0 width=158  height=23>';
strFrame+='			<tr align=center >';
strFrame+='				<td width=16 align=center bgcolor=#63A3E9 style="font-size:12px;cursor: hand;color: #ffffff" ';
strFrame+='		onclick="parent.meizzPrevM()" title="��ǰ�� 1 ��" ><b >&lt;</b></td>';
strFrame+='	   <td width=60 align="center" bgcolor="#63A3E9"  style="font-size:12px;cursor:hand" ';
strFrame+='		   onmouseover="style.backgroundColor=\'#aaccf3\'"';
strFrame+='		onmouseout="style.backgroundColor=\'#63A3E9\'" ';
strFrame+='		onclick="parent.tmpSelectYearInnerHTML(this.innerText.substring(0,4))" ';
strFrame+='		title="�������ѡ�����"><span  id=meizzYearHead></span></td>';
strFrame+='	   <td width=48 align="center" style="font-size:12px;font-color: #ffffff;cursor:hand" ';
strFrame+='		bgcolor="#63A3E9" onmouseover="style.backgroundColor=\'#aaccf3\'" ';
strFrame+='		onmouseout="style.backgroundColor=\'#63A3E9\'" ';
strFrame+='		onclick="parent.tmpSelectMonthInnerHTML(this.innerText.length==3?this.innerText.substring(0,1):this.innerText.substring(0,2))"';
strFrame+='		title="�������ѡ���·�"><span id=meizzMonthHead ></span></td>';
strFrame+='	   <td width=16 bgcolor=#63A3E9 align=center style="font-size:12px;cursor: hand;color: #ffffff" ';
strFrame+='		onclick="parent.meizzNextM()" title="��� 1 ��" ><b >&gt;</b></td>';
strFrame+='	  </tr>';
strFrame+='	 </table></td></tr>';
strFrame+='	<tr><td width=142 height=18 >';
strFrame+='	 <table border=0 cellspacing=0 cellpadding=2 bgcolor=#63A3E9 ' + (bMoveable? 'onmousedown="DragStart()" onmouseup="DragEnd()"':'');
strFrame+='	BORDERCOLORLIGHT=#63A3E9 BORDERCOLORDARK=#FFFFFF width=140 height=20  style="cursor:' + (bMoveable ? 'move':'default') + '">';
strFrame+='	<tr><td style="font-size:12px;color:#ffffff" width=20>&nbsp;��</td>';
strFrame+='<td style="font-size:12px;color:#FFFFFF" >&nbsp;һ</td><td style="font-size:12px;color:#FFFFFF">&nbsp;��</td>';
strFrame+='<td style="font-size:12px;color:#FFFFFF" >&nbsp;��</td><td style="font-size:12px;color:#FFFFFF" >&nbsp;��</td>';
strFrame+='<td style="font-size:12px;color:#FFFFFF" >&nbsp;��</td><td style="font-size:12px;color:#FFFFFF" >&nbsp;��</td></tr>';
strFrame+='</table></td></tr>';
strFrame+='  <tr ><td width=142 height=120 >';
strFrame+='	<table border=1 cellspacing=2 cellpadding=2 BORDERCOLORLIGHT=#63A3E9 BORDERCOLORDARK=#FFFFFF bgcolor=#fff8ec width=140 height=120 >';
var n=0; for (j=0;j<5;j++){ strFrame+= ' <tr align=center >'; for (i=0;i<7;i++){
	strFrame+='<td width=20 height=20 id=meizzDay'+n+' style="font-size:12px" onclick=parent.meizzDayClick(this.innerText,0)></td>';n++;}
	strFrame+='</tr>';}
strFrame+='	  <tr align=center >';
for (i=35;i<37;i++)strFrame+='<td width=20 height=20 id=meizzDay'+i+' style="font-size:12px"  onclick="parent.meizzDayClick(this.innerText,0)"></td>';
strFrame+='		<td colspan=5 align=right style="color:#1478eb"><span onclick="parent.setNull()" style="font-size:12px;cursor: hand"';
strFrame+='		 onmouseover="style.color=\'#ff0000\'" onmouseout="style.color=\'#1478eb\'" title="�������ÿ�">�ÿ�</span>&nbsp;&nbsp;<span onclick="parent.meizzToday()" style="font-size:12px;cursor: hand"';
strFrame+='		 onmouseover="style.color=\'#ff0000\'" onmouseout="style.color=\'#1478eb\'" title="��ǰ����ʱ��">��ǰ</span>&nbsp;&nbsp;<span style="cursor:hand" id=evaAllOK onmouseover="style.color=\'#ff0000\'" onmouseout="style.color=\'#1478eb\'"  onclick="parent.closeLayer()" title="�ر�����">�ر�&nbsp;</span></td></tr>';
strFrame+='	</table></td></tr><tr ><td >';
strFrame+='		<table border=0 cellspacing=1 cellpadding=0 width=100%  bgcolor=#FFFFFF height=22 >';
strFrame+='		  <tr bgcolor="#63A3E9"><td id=bUseTimeLayer width=30  style="cursor:hand" title="�����������/����ʱ��"';
strFrame+='	onmouseover="style.backgroundColor=\'#aaccf3\'" align=center onmouseout="style.backgroundColor=\'#63A3E9\'"';
strFrame+='	 onclick="parent.UseTime(this)">';
strFrame+=' <span></span></td>';
strFrame+='			 <td style="cursor:hand" onclick="parent.tmpSelectHourInnerHTML(this.innerText.length==3?this.innerText.substring(0,1):this.innerText.substring(0,2))"';
strFrame+=' onmouseover="style.backgroundColor=\'#aaccf3\'" onmouseout="style.backgroundColor=\'#63A3E9\'"';
strFrame+=' title="�������ѡ��ʱ��" align=center width=42>' ;
strFrame+='	 <span id=meizzHourHead></span></td>';
strFrame+='			 <td style="cursor:hand" onclick="parent.tmpSelectMinuteInnerHTML(this.innerText.length==3?this.innerText.substring(0,1):this.innerText.substring(0,2))"';
strFrame+=' onmouseover="style.backgroundColor=\'#aaccf3\'" onmouseout="style.backgroundColor=\'#63A3E9\'"';
strFrame+=' title="�������ѡ��ʱ��" align=center width=42>' ;
strFrame+='	 <span id=meizzMinuteHead></span></td>';
strFrame+='			 <td style="cursor:hand" onclick="parent.tmpSelectSecondInnerHTML(this.innerText.length==3?this.innerText.substring(0,1):this.innerText.substring(0,2))"';
strFrame+=' onmouseover="style.backgroundColor=\'#aaccf3\'" onmouseout="style.backgroundColor=\'#63A3E9\'"';
strFrame+=' title="�������ѡ��ʱ��" align=center width=42>' ;
strFrame+='	 <span id=meizzSecondHead></span></td>';
strFrame+='	</tr></table></td></tr></table></div>';

window.frames.endDateLayer.document.writeln(strFrame);
window.frames.endDateLayer.document.close();  //���ie������������������


//==================================================== WEB ҳ����ʾ���� ======================================================
var outObject;
var outButton;  //����İ�ť

var outDate="";  //��Ŷ��������
var bUseTime=false;  //�Ƿ�ʹ��ʱ��
var odatelayer=window.frames.endDateLayer.document.all;  //�����������
var odatelayer=window.endDateLayer.document.all;
//odatelayer.bUseTimeLayer.innerText="NO";
bImgSwitch();
odatelayer.bUseTimeLayer.innerHTML=bImg;

function setday(tt,obj) //��������
{
	if (arguments.length > 2){alert("�Բ��𣡴��뱾�ؼ��Ĳ���̫�࣡");return;}
	if (arguments.length == 0){alert("�Բ�����û�д��ر��ؼ��κβ�����");return;}
	var dads = document.all.endDateLayer.style;
	var th = tt;
	var ttop = tt.offsetTop; //TT�ؼ��Ķ�λ���

	var thei = tt.clientHeight; //TT�ؼ�����ĸ�
	var tleft = tt.offsetLeft; //TT�ؼ��Ķ�λ���

	var ttyp = tt.type; //TT�ؼ�������

	while (tt = tt.offsetParent){ttop+=tt.offsetTop; tleft+=tt.offsetLeft;}
	dads.top = (ttyp=="image") ? ttop+thei : ttop+thei+6;
	dads.left = tleft;
	outObject = (arguments.length == 1) ? th : obj;
	outButton = (arguments.length == 1) ? null : th; //�趨�ⲿ����İ�ť

	//���ݵ�ǰ������������ʾ����������
	var reg = /^(\d+)-(\d{1,2})-(\d{1,2})/;  //����ʱ��
	var r = outObject.value.match(reg);
	if(r!=null){
		r[2]=r[2]-1;
		var d=new Date(r[1],r[2],r[3]);
		if(d.getFullYear()==r[1] && d.getMonth()==r[2] && d.getDate()==r[3])
		{
			outDate=d;
			parent.meizzTheYear = r[1];
			parent.meizzTheMonth = r[2];
			parent.meizzTheDate = r[3];
		}
		else
		{
			outDate="";
		}
		meizzSetDay(r[1],r[2]+1);
	}
	else
	{
		outDate="";
		meizzSetDay(new Date().getFullYear(), new Date().getMonth() + 1);
	}
	dads.display = '';

	//�жϳ�ʼ��ʱ�Ƿ�ʹ��ʱ��,���ϸ���֤
	//if (outObject.value.length>10)
	//{
	bUseTime=true;
	bImgSwitch();
	odatelayer.bUseTimeLayer.innerHTML=bImg;
	meizzWriteHead(meizzTheYear,meizzTheMonth);
	//}
	//else
	//{
	// bUseTime=false;
	// bImgSwitch();
	// odatelayer.bUseTimeLayer.innerHTML=bImg;
	// meizzWriteHead(meizzTheYear,meizzTheMonth);
	//}

	try
	{
		event.returnValue=false;
	}
	catch (e)
	{
		//�˴��ų����󣬴���ԭ����δ�ҵ���
	}
}

var MonHead = new Array(12); //����������ÿ���µ��������

MonHead[0] = 31; MonHead[1] = 28; MonHead[2] = 31; MonHead[3] = 30; MonHead[4]  = 31; MonHead[5]  = 30;
MonHead[6] = 31; MonHead[7] = 31; MonHead[8] = 30; MonHead[9] = 31; MonHead[10] = 30; MonHead[11] = 31;

var meizzTheYear=new Date().getFullYear(); //������ı����ĳ�ʼֵ

var meizzTheMonth=new Date().getMonth()+1; //�����µı����ĳ�ʼֵ

var meizzTheDate=new Date().getDate(); //�����յı����ĳ�ʼֵ 
var meizzTheHour=new Date().getHours(); //����Сʱ�����ĳ�ʼֵ

var meizzTheMinute=new Date().getMinutes();//������ӱ����ĳ�ʼֵ
var meizzTheSecond=new Date().getSeconds();//����������ĳ�ʼֵ

var meizzWDay=new Array(37); //����д���ڵ�����

function document.onclick() //������ʱ�رոÿؼ� //ie6�����������������л����㴦�����
{ 
	with(window.event)
	{
		if (srcElement != outObject && srcElement != outButton)
			closeLayer();
	}
}

function document.onkeyup()  //��Esc���رգ��л�����ر�
{
	if (window.event.keyCode==27){
		if(outObject)outObject.blur();
		closeLayer();
	}
	else if(document.activeElement)
	{
		if(document.activeElement != outObject && document.activeElement != outButton)
		{
			closeLayer();
		}
	}
}

function meizzWriteHead(yy,mm,ss) //�� head ��д�뵱ǰ��������
{
	odatelayer.meizzYearHead.innerText = yy + " ��";
	odatelayer.meizzMonthHead.innerText = format(mm) + " ��";
	//���뵱ǰСʱ����
	odatelayer.meizzHourHead.innerText=bUseTime?(meizzTheHour+" ʱ"):""; 
	odatelayer.meizzMinuteHead.innerText=bUseTime?(meizzTheMinute+" ��"):"";
	odatelayer.meizzSecondHead.innerText=bUseTime?(meizzTheSecond+" ��"):"";
}

function tmpSelectYearInnerHTML(strYear) //��ݵ�������
{
	if (strYear.match(/\D/)!=null){alert("�����������������֣�");return;}
	var m = (strYear) ? strYear : new Date().getFullYear();
	if (m < 1000 || m > 9999) {alert("���ֵ���� 1000 �� 9999 ֮�䣡");return;}
	var n = m - 50;
	if (n < 1000) n = 1000;
	if (n + 101 > 9999) n = 9974;
	var s = "&nbsp;<select name=tmpSelectYear style='font-size: 12px' "
	s += "onblur='document.all.tmpSelectYearLayer.style.display=\"none\"' "
	s += "onchange='document.all.tmpSelectYearLayer.style.display=\"none\";"
	s += "parent.meizzTheYear = this.value; parent.meizzSetDay(parent.meizzTheYear,parent.meizzTheMonth)'>\r\n";
	var selectInnerHTML = s;
	for (var i = n; i < n + 101; i++)
	{
		if (i == m) { selectInnerHTML += "<option value='" + i + "' selected>" + i + "��" + "</option>\r\n"; }
		else { selectInnerHTML += "<option value='" + i + "'>" + i + "��" + "</option>\r\n"; }
	}
	selectInnerHTML += "</select>";
	odatelayer.tmpSelectYearLayer.style.display="";
	odatelayer.tmpSelectYearLayer.innerHTML = selectInnerHTML;
	odatelayer.tmpSelectYear.focus();
}

function tmpSelectMonthInnerHTML(strMonth) //�·ݵ�������
{
	if (strMonth.match(/\D/)!=null){alert("�·���������������֣�");return;}
	var m = (strMonth) ? strMonth : new Date().getMonth() + 1;
	var s = "&nbsp;&nbsp;&nbsp;<select name=tmpSelectMonth style='font-size: 12px' "
	s += "onblur='document.all.tmpSelectMonthLayer.style.display=\"none\"' "
	s += "onchange='document.all.tmpSelectMonthLayer.style.display=\"none\";"
	s += "parent.meizzTheMonth = this.value; parent.meizzSetDay(parent.meizzTheYear,parent.meizzTheMonth)'>\r\n";
	var selectInnerHTML = s;
	for (var i = 1; i < 13; i++)
	{
		if (i == m) { selectInnerHTML += "<option value='"+i+"' selected>"+i+"��"+"</option>\r\n"; }
		else { selectInnerHTML += "<option value='"+i+"'>"+i+"��"+"</option>\r\n"; }
	}
	selectInnerHTML += "</select>";
	odatelayer.tmpSelectMonthLayer.style.display="";
	odatelayer.tmpSelectMonthLayer.innerHTML = selectInnerHTML;
	odatelayer.tmpSelectMonth.focus();
}

/***** ���� Сʱ������ ***/
function tmpSelectHourInnerHTML(strHour) //Сʱ��������
{
	if (!bUseTime){return;}

	if (strHour.match(/\D/)!=null){alert("Сʱ��������������֣�");return;}
	var m = (strHour) ? strHour : new Date().getHours();
	var s = "<select name=tmpSelectHour style='font-size: 12px' "
	s += "onblur='document.all.tmpSelectHourLayer.style.display=\"none\"' "
	s += "onchange='document.all.tmpSelectHourLayer.style.display=\"none\";"
	s += "parent.meizzTheHour = this.value; parent.evaSetTime(parent.meizzTheHour,parent.meizzTheMinute);'>\r\n";
	var selectInnerHTML = s;
	for (var i = 0; i < 24; i++)
	{
		if (i == m) { selectInnerHTML += "<option value='"+i+"' selected>"+i+"</option>\r\n"; }
		else { selectInnerHTML += "<option value='"+i+"'>"+i+"</option>\r\n"; }
	}
	selectInnerHTML += "</select>";
 
	odatelayer.tmpSelectHourLayer.style.display="";
	odatelayer.tmpSelectHourLayer.innerHTML = selectInnerHTML;
	odatelayer.tmpSelectHour.focus();
}

function tmpSelectMinuteInnerHTML(strMinute) //���ӵ�������
{
	if (!bUseTime){return;}

	if (strMinute.match(/\D/)!=null){alert("������������������֣�");return;}
	var m = (strMinute) ? strMinute : new Date().getMinutes();
	var s = "<select name=tmpSelectMinute style='font-size: 12px' "
	s += "onblur='document.all.tmpSelectMinuteLayer.style.display=\"none\"' "
	s += "onchange='document.all.tmpSelectMinuteLayer.style.display=\"none\";"
	s += "parent.meizzTheMinute = this.value; parent.evaSetTime(parent.meizzTheHour,parent.meizzTheMinute);'>\r\n";
	var selectInnerHTML = s;
	for (var i = 0; i < 60; i++)
	{
		if (i == m) { selectInnerHTML += "<option value='"+i+"' selected>"+i+"</option>\r\n"; }
		else { selectInnerHTML += "<option value='"+i+"'>"+i+"</option>\r\n"; }
	}
	selectInnerHTML += "</select>";
	odatelayer.tmpSelectMinuteLayer.style.display="";
	odatelayer.tmpSelectMinuteLayer.innerHTML = selectInnerHTML;
	odatelayer.tmpSelectMinute.focus();
}

function tmpSelectSecondInnerHTML(strSecond) //���������
{
	if (!bUseTime){return;}

	if (strSecond.match(/\D/)!=null){alert("������������������֣�");return;}
	var m = (strSecond) ? strSecond : new Date().getMinutes();
	var s = "<select name=tmpSelectSecond style='font-size: 12px' "
	s += "onblur='document.all.tmpSelectSecondLayer.style.display=\"none\"' "
	s += "onchange='document.all.tmpSelectSecondLayer.style.display=\"none\";"
	s += "parent.meizzTheSecond = this.value; parent.evaSetTime(parent.meizzTheHour,parent.meizzTheMinute,parent.meizzTheSecond);'>\r\n";
	var selectInnerHTML = s;
	for (var i = 0; i < 60; i++)
	{
		if (i == m) { selectInnerHTML += "<option value='"+i+"' selected>"+i+"</option>\r\n"; }
		else { selectInnerHTML += "<option value='"+i+"'>"+i+"</option>\r\n"; }
	}
	selectInnerHTML += "</select>";
	odatelayer.tmpSelectSecondLayer.style.display="";
	odatelayer.tmpSelectSecondLayer.innerHTML = selectInnerHTML;
	odatelayer.tmpSelectSecond.focus();
}

function closeLayer() //�����Ĺر�
{
	var o = document.getElementById("endDateLayer");
	if (o != null)
	{
		o.style.display="none";
	}
}

function showLayer() //�����Ĺر�
{
	document.all.endDateLayer.style.display="";
}

function IsPinYear(year) //�ж��Ƿ���ƽ��
{
	if (0==year%4&&((year%100!=0)||(year%400==0))) return true;else return false;
}

function GetMonthCount(year,month) //�������Ϊ29��
{
	var c=MonHead[month-1];if((month==2)&&IsPinYear(year)) c++;return c;
}

function GetDOW(day,month,year) //��ĳ������ڼ�
{
	var dt=new Date(year,month-1,day).getDay()/7; return dt;
}

function meizzPrevY() //��ǰ�� Year
{
	if(meizzTheYear > 999 && meizzTheYear <10000){meizzTheYear--;}
	else{alert("��ݳ�����Χ��1000-9999����");}
	meizzSetDay(meizzTheYear,meizzTheMonth);
}
function meizzNextY() //���� Year
{
	if(meizzTheYear > 999 && meizzTheYear <10000){meizzTheYear++;}
	else{alert("��ݳ�����Χ��1000-9999����");}
	meizzSetDay(meizzTheYear,meizzTheMonth);
}
function setNull()
{
	outObject.value = '';
	closeLayer();
}
function meizzToday() //Today Button
{
	parent.meizzTheYear  = new Date().getFullYear();
	parent.meizzTheMonth = new Date().getMonth()+1;
	parent.meizzTheDate  = new Date().getDate();
	parent.meizzTheHour  = new Date().getHours();
	parent.meizzTheMinute = new Date().getMinutes();
	parent.meizzTheSecond = new Date().getSeconds();
	var meizzTheSecond  = new Date().getSeconds();

	if (meizzTheMonth<10 && meizzTheMonth.length<2) //��ʽ������λ����
	{
		parent.meizzTheMonth="0"+parent.meizzTheMonth;
	}
	if (parent.meizzTheDate<10 && parent.meizzTheDate.length<2) //��ʽ������λ����
	{
		parent.meizzTheDate="0"+parent.meizzTheDate;
	}
	//meizzSetDay(meizzTheYear,meizzTheMonth);
	if(outObject)
	{
		if (bUseTime)
		{
			outObject.value= parent.meizzTheYear + "-" + format( parent.meizzTheMonth) + "-" + 
				format(parent.meizzTheDate) + " " + format(parent.meizzTheHour) + ":" + 
				format(parent.meizzTheMinute) + ":" + format(parent.meizzTheSecond); 
			//ע�����������������ĳ�����Ҫ�ĸ�ʽ
		}
		else
		{
			outObject.value= parent.meizzTheYear + "-" + format( parent.meizzTheMonth) + "-" + 
				format(parent.meizzTheDate); //ע�����������������ĳ�����Ҫ�ĸ�ʽ
		}
	}
	closeLayer();
}
function meizzPrevM() //��ǰ���·�
{
	if(meizzTheMonth>1){meizzTheMonth--}else{meizzTheYear--;meizzTheMonth=12;}
	meizzSetDay(meizzTheYear,meizzTheMonth);
}
function meizzNextM() //�����·�
{
	if(meizzTheMonth==12){meizzTheYear++;meizzTheMonth=1}else{meizzTheMonth++}
	meizzSetDay(meizzTheYear,meizzTheMonth);
}

// TODO: �������
function meizzSetDay(yy,mm) //��Ҫ��д����**********
{
	meizzWriteHead(yy,mm);
	//���õ�ǰ���µĹ�������Ϊ����ֵ

	meizzTheYear=yy;
	meizzTheMonth=mm;

	for (var i = 0; i < 37; i++){meizzWDay[i]=""}; //����ʾ�������ȫ�����

	var day1 = 1,day2=1,firstday = new Date(yy,mm-1,1).getDay(); //ĳ�µ�һ������ڼ�

	for (i=0;i<firstday;i++)meizzWDay[i]=GetMonthCount(mm==1?yy-1:yy,mm==1?12:mm-1)-firstday+i+1 //�ϸ��µ������

	for (i = firstday; day1 < GetMonthCount(yy,mm)+1; i++) { meizzWDay[i]=day1;day1++; }
	for (i=firstday+GetMonthCount(yy,mm);i<37;i++) { meizzWDay[i]=day2;day2++; }
	for (i = 0; i < 37; i++)
	{
		var da = eval("odatelayer.meizzDay"+i) //��д�µ�һ���µ�������������

		if (meizzWDay[i]!="")
		{
			//��ʼ���߿�
			da.borderColorLight="#63A3E9";
			da.borderColorDark="#63A3E9";
			da.style.color="#1478eb";
			if(i<firstday)  //�ϸ��µĲ���
			{
				da.innerHTML="<b><font color=#BCBABC>" + meizzWDay[i] + "</font></b>";
				da.title=(mm==1?12:mm-1) +"��" + meizzWDay[i] + "��";
				da.onclick=Function("meizzDayClick(this.innerText,-1)");

				if(!outDate)
					da.style.backgroundColor = ((mm==1?yy-1:yy) == new Date().getFullYear() && 
					 (mm==1?12:mm-1) == new Date().getMonth()+1 && meizzWDay[i] == new Date().getDate()) ?
					  "#5CEFA0":"#f5f5f5";
				else
				{
					da.style.backgroundColor =((mm==1?yy-1:yy)==outDate.getFullYear() && (mm==1?12:mm-1)== outDate.getMonth() + 1 && 
					meizzWDay[i]==outDate.getDate())? "#84C1FF" :
					(((mm==1?yy-1:yy) == new Date().getFullYear() && (mm==1?12:mm-1) == new Date().getMonth()+1 && 
					meizzWDay[i] == new Date().getDate()) ? "#5CEFA0":"#f5f5f5");
					//��ѡ�е�������ʾΪ����ȥ

					if((mm==1?yy-1:yy)==outDate.getFullYear() && (mm==1?12:mm-1)== outDate.getMonth() + 1 && 
					meizzWDay[i]==outDate.getDate())
					{
						da.borderColorLight="#FFFFFF";
						da.borderColorDark="#63A3E9";
					}
				}
			}
			else if (i>=firstday+GetMonthCount(yy,mm))  //�¸��µĲ���
			{
				da.innerHTML="<b><font color=#BCBABC>" + meizzWDay[i] + "</font></b>";
				da.title=(mm==12?1:mm+1) +"��" + meizzWDay[i] + "��";
				da.onclick=Function("meizzDayClick(this.innerText,1)");
				if(!outDate)
					da.style.backgroundColor = ((mm==12?yy+1:yy) == new Date().getFullYear() && 
					 (mm==12?1:mm+1) == new Date().getMonth()+1 && meizzWDay[i] == new Date().getDate()) ?
					  "#5CEFA0":"#f5f5f5";
				else
				{
					da.style.backgroundColor =((mm==12?yy+1:yy)==outDate.getFullYear() && (mm==12?1:mm+1)== outDate.getMonth() + 1 && 
					meizzWDay[i]==outDate.getDate())? "#84C1FF" :
					(((mm==12?yy+1:yy) == new Date().getFullYear() && (mm==12?1:mm+1) == new Date().getMonth()+1 && 
					meizzWDay[i] == new Date().getDate()) ? "#5CEFA0":"#f5f5f5");
					//��ѡ�е�������ʾΪ����ȥ

					if((mm==12?yy+1:yy)==outDate.getFullYear() && (mm==12?1:mm+1)== outDate.getMonth() + 1 && 
					meizzWDay[i]==outDate.getDate())
					{
						da.borderColorLight="#FFFFFF";
						da.borderColorDark="#63A3E9";
					}
				}
			}
			else  //���µĲ���

			{
				da.innerHTML="<b>" + meizzWDay[i] + "</b>";
				da.title=mm +"��" + meizzWDay[i] + "��";
				da.onclick=Function("meizzDayClick(this.innerText,0)");  //��td����onclick�¼��Ĵ���

				//����ǵ�ǰѡ������ڣ�����ʾ����ɫ�ı���������ǵ�ǰ���ڣ�����ʾ����ɫ����
				if(!outDate)
					da.style.backgroundColor = (yy == new Date().getFullYear() && mm == new Date().getMonth()+1 && meizzWDay[i] == new Date().getDate())?
					 "#5CEFA0":"#f5f5f5";
				else
				{
					da.style.backgroundColor =(yy==outDate.getFullYear() && mm== outDate.getMonth() + 1 && meizzWDay[i]==outDate.getDate())?
					 "#84C1FF":((yy == new Date().getFullYear() && mm == new Date().getMonth()+1 && meizzWDay[i] == new Date().getDate())?
					 "#5CEFA0":"#f5f5f5");
					//��ѡ�е�������ʾΪ����ȥ

					if(yy==outDate.getFullYear() && mm== outDate.getMonth() + 1 && meizzWDay[i]==outDate.getDate())
					{
						da.borderColorLight="#FFFFFF";
						da.borderColorDark="#63A3E9";
					}
				}
			}
			da.style.cursor="hand"
		}
		else { da.innerHTML="";da.style.backgroundColor="";da.style.cursor="default"; }
	}
}

function meizzDayClick(n,ex) //�����ʾ��ѡȡ���ڣ������뺯��*************
{
	parent.meizzTheDate=n;
	var yy=meizzTheYear;
	var mm = parseInt(meizzTheMonth)+ex; //ex��ʾƫ����������ѡ���ϸ��·ݺ��¸��·ݵ�����
	var hh=meizzTheHour;
	var mi=meizzTheMinute;
	var se=meizzTheSecond;
	//�ж��·ݣ������ж�Ӧ�Ĵ���

	if(mm<1){
		yy--;
		mm=12+mm;
	}
	else if(mm>12){
		yy++;
		mm=mm-12;
	}

	if (mm < 10) {mm = "0" + mm;}
	if (hh<10)  {hh="0" + hh;} //ʱ
	if (mi<10)  {mi="0" + mi;} //��
	if (se<10)  {se="0" + se;} //��

	if (outObject)
	{
		if (!n) { //outObject.value=""; 
			return;}
		if ( n < 10){n = "0" + n;}

		WriteDateTo(yy,mm,n,hh,mi,se);

		closeLayer(); 
		if (bUseTime)
		{
			try
			{
				outButton.click();
			}
			catch (e)
			{
				setday(outObject);
			}
		}
	}
	else {closeLayer(); alert("����Ҫ����Ŀؼ����󲢲����ڣ�");}
}

function format(n) //��ʽ������Ϊ��λ�ַ���ʾ
{
	var m=new String();
	var tmp=new String(n);
	if (n<10 && tmp.length<2)
	{
		m="0"+n;
	}
	else
	{
		m=n;
	}
	return m;
}

function evaSetTime()  //�����û�ѡ���Сʱ������
{
	odatelayer.meizzHourHead.innerText=meizzTheHour+" ʱ";
	odatelayer.meizzMinuteHead.innerText=meizzTheMinute+" ��";
	odatelayer.meizzSecondHead.innerText=meizzTheSecond+" ��";
	var strTime = outObject.value;

	//WriteDateTo(meizzTheYear,meizzTheMonth,meizzTheDate,meizzTheHour,meizzTheMinute,meizzTheSecond)
	WriteTimeTo(strTime.split(" ")[0],meizzTheHour,meizzTheMinute,meizzTheSecond);
}

function evaSetTimeNothing() //����ʱ��ؼ�Ϊ��
{
	odatelayer.meizzHourHead.innerText="";
	odatelayer.meizzMinuteHead.innerText="";
	odatelayer.meizzSecondHead.innerText="";
	WriteDateTo(meizzTheYear,meizzTheMonth,meizzTheDate,meizzTheHour,meizzTheMinute,meizzTheSecond)
}

function evaSetTimeNow() //����ʱ��ؼ�Ϊ��ǰʱ��
{
	odatelayer.meizzHourHead.innerText=new Date().getHours()+" ʱ";
	odatelayer.meizzMinuteHead.innerText=new Date().getMinutes()+" ��";
	odatelayer.meizzSecondHead.innerText=new Date().getSeconds()+" ��";
	meizzTheHour = new Date().getHours();
	meizzTheMinute = new Date().getMinutes();
	meizzTheSecond = new Date().getSeconds();
	WriteDateTo(meizzTheYear,meizzTheMonth,meizzTheDate,meizzTheHour,meizzTheMinute,meizzTheSecond)
}

function UseTime(ctl)
{
	bUseTime=!bUseTime;
	if (bUseTime)
	{
		bImgSwitch();
		ctl.innerHTML=bImg;
		evaSetTime();  //��ʾʱ�䣬�û�ԭ��ѡ���ʱ��
		//evaSetTimeNow(); //��ʾ��ǰʱ��
	}
	else
	{
		bImgSwitch();
		ctl.innerHTML=bImg;
		//evaSetTimeNothing();
		setWithOldTime();
	}
}

function setWithOldTime()
{
	odatelayer.meizzHourHead.innerText="";
	odatelayer.meizzMinuteHead.innerText="";
	odatelayer.meizzSecondHead.innerText="";
	var strTime = outObject.value;
	var ymd = strTime.split(" ")[0];
	var y = ymd.split("-")[0];
	var m = ymd.split("-")[1];
	var d = ymd.split("-")[2];
	//WriteDateTo(meizzTheYear,meizzTheMonth,meizzTheDate,meizzTheHour,meizzTheMinute,meizzTheSecond)
	WriteDateTo(y,m,d,meizzTheHour,meizzTheMinute,meizzTheSecond)
}

function WriteTimeTo(ymn,hh,mi,se)
{
	outObject.value= ymn + " " + format(hh) + ":" + format(mi) + ":" + format(se); //ע�����������������ĳ�����Ҫ�ĸ�ʽ
}

function WriteDateTo(yy,mm,n,hh,mi,se)
{
	if (bUseTime)
	{
		outObject.value= yy + "-" + format(mm) + "-" + format(n) + " " + format(hh) + ":" + format(mi) + ":" + format(se); //ע�����������������ĳ�����Ҫ�ĸ�ʽ
	}
	else
	{
		outObject.value= yy + "-" + format(mm) + "-" + format(n); //ע�����������������ĳ�����Ҫ�ĸ�ʽ
	}
}

function bImgSwitch()
{
	if (bUseTime)
	{
		bImg="����";
	}
	else
	{
		bImg="�ر�";
	}
}
