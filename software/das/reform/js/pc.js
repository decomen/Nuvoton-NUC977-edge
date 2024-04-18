	 // $(function(){
		//	 $(".menu > ul").eq(0).show();
		//	 $(".menu h3").click(function(){
		//		 $(this).next().stop().slideToggle();
		//		 $(this).siblings().next("ul").stop().slideUp();
		//	 });
		//	 $(".menu > ul > li > a").click(function(){
		//	 	var index=$(this).index();
		//		 $(this).addClass("selected").parent().siblings().find("a").removeClass("selected");
		//		 $(this).addClass("active").parent().siblings().find("a").removeClass("active");

		//	 })
		// });
		$(document).ready(function(){

			/*function myfunction(li,li_a,menu_tab){

				li.click(function(){
				var index=$(this).attr("index")
				menu_tab.eq(index).addClass("active").parent().siblings().next(".tab").removeClass("active");
				li_a.removeClass("selected");
				li_a.eq(index).addClass("selected").siblings().removeClass("selected");
				$(".menuf .tab").css("display","none");
				$(".menugg .tab").css("display","none");
			});
			$(".menu h3").click(function(){
				$(".menuf .tab").css("display","none");
				$(".menugg .tab").css("display","none");
				$(".menue .tab").css("display","none");
				$(".menuhh .tab").css("display","none");
			})
			$('.tu7').click(function(){
				$(".menue .tab").css("display","block");
			});
			$(".tu12").click(function(){
				$('.menue .tab').css("display","none");
				$(".menuf .tab").css("display","block");
			});
			$(".tu13").click(function(){
				$('.menue .tab').css("display","none");
				$(".menugg .tab").css("display","block");
				Show_Network_Information();
			});
			$(".tu14").click(function(){
				$(".menuhh .tab").css("display","block");
				
			})
			}
			myfunction($(".container .menu .ulmenu1 li"),$(".container .ulmenu1 li a"),$(".container .content .menu1 .tab"));
			myfunction($(".container .menu .ulmenuz li"),$(".container .ulmenuz li a"),$(".container .content .menuz .tab"));
			myfunction($(".container .menu .ulmenu2 li"),$(".container .ulmenu2 li a"),$(".container .content .menu2 .tab"));
			myfunction($(".container .menu .ulmenu3 li"),$(".container .ulmenu3 li a"),$(".container .content .menu3 .tab"));
			myfunction($(".container .menu .ulmenu4 li"),$(".container .ulmenu4 li a"),$(".container .content .menu4 .tab"));
			myfunction($(".container .menu .ulmenu5 li"),$(".container .ulmenu5 li a"),$(".container .content .menu5 .tab"));
			myfunction($(".container .menu .ulmenu6 li"),$(".container .ulmenu6 li a"),$(".container .content .menu6 .tab"));
			myfunction($(".container .menu .ulmenu7 li"),$(".container .ulmenu7 li a"),$(".container .content .menu7 .tab"));
			myfunction($(".container .menu .ulmenu8 li"),$(".container .ulmenu8 li a"),$(".container .content .menu8 .tab"));
			myfunction($(".container .menu .ulmenu9 li"),$(".container .ulmenu9 li a"),$(".container .content .menu9 .tab"));
			myfunction($(".container .menu .ulmenub li"),$(".container .ulmenub li a"),$(".container .content .menub .tab"));
			myfunction($(".container .menu .ulmenue li"),$(".container .ulmenue li a"),$(".container .content .menue .tab"));
			myfunction($(".container .menu .ulmenua li"),$(".container .ulmenua li a"),$(".container .content .menua .tab"));
			myfunction($(".container .menu .ulmenuc li"),$(".container .ulmenuc li a"),$(".container .content .menuc .tab"));
			myfunction($(".container .menu .ulmenud li"),$(".container .ulmenud li a"),$(".container .content .menud .tab"));
			myfunction($(".container .menu .ulmenuf li"),$(".container .ulmenuf li a"),$(".container .content .menuf .tab"));
			myfunction($(".container .menu .ulmenugg li"),$(".container .ulmenugg li a"),$(".container .content .menugg .tab"));*/


			// var li1=$(".container .menu ul li");
			// var lia=$(".container .menu ul li a");
			// var tab1=$(".container .content .menu1 .tab ");

			// li1.click(function(){
			// 	var index=$(this).index();

			// 	tab1.eq(index).addClass("active").siblings().removeClass("active");
			// 	lia.removeClass("selected");
			// 	lia.eq(index).addClass("selected").siblings().removeClass("selected");
			// });

			$(function(){			//ul/li的折叠效果
				$(".menu > ul").eq(0).show();
				 $(".menu h3").click(function(){
				 		var val=($(this).next().attr('class'));
				 		val = val ? val : $(this).next()[0].className;

				 		var menu_value=(val.substring(val.length-1));
				 		var $li = $(".container .menu .ulmenu"+menu_value+" li");
				 		//找menu对应的tab
				 		$(".menu_tab > div").removeClass("active");
				 		$(".menu h3").removeClass("selected");
				 		$li.find("a").removeClass("selected");

				 		$(".container .content .menu"+menu_value+" .tab").eq(0).addClass("active");
				 		if($li.length == 0){
				 			$(this).addClass("selected");
				 		}else{
				 			$li.find("a").eq(0).addClass("selected");
				 		}
				 		//$(".container .menu .ulmenu"+menu_value+" li>a").removeClass("selected");
				 		//$(".container .menu .ulmenu"+menu_value+" li a").eq(0).addClass("selected");//默认设置为被选中状态
				 		

				 		// $("."+"val").child().child().("selected")
				 		
				 			//这是ul收缩效果
							//$(this).next().stop().toggle();
							//$(this).siblings().next("ul").stop().slideUp();
						   $(this).siblings().next("ul").hide();
						   $(this).next("ul").toggle();
						   
							
						   // if($(".container .ulmenu"+menu_value+" li ").find("a").attr("class")==="selected"){
						   // 		$(".container .content .menu"+menu_value+" .tab:first-child")
						   // }
						});


				$.each($(".menu ul"), function(){
					$.each($(this).find("li"), function(j){
							 $(this).attr("index",j)
					})
				})
				 //二级菜单事件绑定 by chenqq 18180984862
				 $(".menu ul li").click(function(){
				 		$(".menu_tab > div").removeClass("active");

				 		var index = $(this).attr("index");
				 		var val=$(this).parents("ul").attr('class');
				 		val = val ? val : $(this).parents("ul")[0].className;
				 		var menu_value=(val.substring(val.length-1));

				 		$(this).parent().find("li a").removeClass("selected");
				 		$(this).find("a").addClass("selected");

				 		$(".container .content .menu"+menu_value+" .tab").eq(index).addClass("active");
				 })

			});
			
			$(function(){   // 导航 >
				 $(".container .menu > h3").click(function(){

				 	$(".container .content .A1").empty().text($(this).text());
				 	
				 });
			});


			//弹窗关闭按钮
			$(".theme-poptit .close").on("click", function(){
				$('.theme-popover-mask').hide();
				$('.theme-popover').hide();
			})
			
			//驱动弹窗菜单绑定
			$(".J_data_tab li").on("click", function(){
				$(".J_data_tab li").removeClass("selected");
				$(this).addClass("selected");

				$(".data_tab_content").addClass("hide");
				$("#"+$(this).attr("data-link")).removeClass("hide");
			})
			
			$(".J_add_group_tab li").on("click", function(){
				$(".J_add_group_tab li").removeClass("selected");
				$(this).addClass("selected");

				$(".add_group_tab_content").addClass("hide");
				$("#"+$(this).attr("data-link")).removeClass("hide");
			})
		});




//下拉菜单
		var flag=0;
		var oDiv0=document.getElementById("box0");
		var oDiv1=document.getElementById("box1");
		var oDiv=document.getElementById("items");
		oDiv1.onclick=function(){
			  if(flag==0){
					   oDiv.style.display="block"; 
								   flag = 1;
			   }
			   else{
								   flag=0; 
								oDiv.style.display="none"; 
								getCheckedValues();
						 }

		} 
		
		oDiv0.onclick=function(){
			if(flag!=0){
								   flag=0; 
								oDiv.style.display="none"; 
								getCheckedValues();
						 }

		} 
		/*
		 * 以下是用于当鼠标离开列表时，列表消失 
		  oDiv.onmouseleave = function(){
				flag=0; this.style.display="none"; getCheckedValues();		
		} */
		function getCheckedValues(){
				
				var values = document.getElementsByName("test");
				
				var selectValue="";
				
				for(var i=0;i<values.length;i++){
						if(values[i].checked){ 
								selectValue += values[i].value +" | ";
						}
				}		 
				document.getElementById("box0").innerText=selectValue;
		} 



//下拉菜单2
 var flag=0;
		var oDiv00=document.getElementById("box00");
		var oDiv11=document.getElementById("box11");
		var oDivv=document.getElementById("itemss");
		oDiv11.onclick=function(){
			  if(flag==0){
					   oDivv.style.display="block"; 
								   flag = 1;
			   }
			   else{
								   flag=0; 
								oDiv.style.display="none"; 
								getCheckedValues();
						 }

		} 
		
		oDiv00.onclick=function(){
			if(flag!=0){
								   flag=0; 
								oDiv.style.display="none"; 
								getCheckedValues();
						 }

		} 
		/*
		 * 以下是用于当鼠标离开列表时，列表消失 
		  oDiv.onmouseleave = function(){
				flag=0; this.style.display="none"; getCheckedValues();		
		} */
		function getCheckedValues(){
				
				var values = document.getElementsByName("test");
				
				var selectValue="";
				
				for(var i=0;i<values.length;i++){
						if(values[i].checked){ 
								selectValue += values[i].value +" | ";
						}
				}		 
				document.getElementById("box00").innerText=selectValue;
		} 
		
		
		
		
		(function($){
	if(typeof($.fn.lc_switch) != 'undefined') {return false;} // prevent dmultiple scripts inits
	
	$.fn.lc_switch = function(on_text, off_text) {

		// destruct
		$.fn.lcs_destroy = function() {
			
			$(this).each(function() {
				var $wrap = $(this).parents('.lcs_wrap');
				
				$wrap.children().not('input').remove();
				$(this).unwrap();
			});
			
			return true;
		};	

		
		// set to ON
		$.fn.lcs_on = function() {
			
			$(this).each(function() {
				var $wrap = $(this).parents('.lcs_wrap');
				var $input = $wrap.find('input');
				
				if(typeof($.fn.prop) == 'function') {
					$wrap.find('input').prop('checked', true);
				} else {
					$wrap.find('input').attr('checked', true);
				}
				
				$wrap.find('input').trigger('lcs-on');
				$wrap.find('input').trigger('lcs-statuschange');
				$wrap.find('.lcs_switch').removeClass('lcs_off').addClass('lcs_on');
				
				// if radio - disable other ones 
				if( $wrap.find('.lcs_switch').hasClass('lcs_radio_switch') ) {
					var f_name = $input.attr('name');
					$wrap.parents('form').find('input[name='+f_name+']').not($input).lcs_off();	
				}
			});
			
			return true;
		};	
		
		
		// set to OFF
		$.fn.lcs_off = function() {
			
			$(this).each(function() {
				var $wrap = $(this).parents('.lcs_wrap');

				if(typeof($.fn.prop) == 'function') {
					$wrap.find('input').prop('checked', false);
				} else {
					$wrap.find('input').attr('checked', false);
				}
				
				$wrap.find('input').trigger('lcs-off');
				$wrap.find('input').trigger('lcs-statuschange');
				$wrap.find('.lcs_switch').removeClass('lcs_on').addClass('lcs_off');
			});
			
			return true;
		};	
		
		
		// construct
		return this.each(function(){
			
			// check against double init
			if( !$(this).parent().hasClass('lcs_wrap') ) {
			
				// default texts
				var ckd_on_txt = (typeof(on_text) == 'undefined') ? '打开' : on_text;
				var ckd_off_txt = (typeof(off_text) == 'undefined') ? '关闭' : off_text;
			   
			   // labels structure
				var on_label = (ckd_on_txt) ? '<div class="lcs_label lcs_label_on">'+ ckd_on_txt +'</div>' : '';
				var off_label = (ckd_off_txt) ? '<div class="lcs_label lcs_label_off">'+ ckd_off_txt +'</div>' : '';
				
				
				// default states
				var disabled 	= ($(this).is(':disabled')) ? true: false;
				var active 		= ($(this).is(':checked')) ? true : false;
				
				var status_classes = '';
				status_classes += (active) ? ' lcs_on' : ' lcs_off'; 
				if(disabled) {status_classes += ' lcs_disabled';} 
			   
			   
				// wrap and append
				var structure = 
				'<div class="lcs_switch '+status_classes+'">' +
					'<div class="lcs_cursor"></div>' +
					on_label + off_label +
				'</div>';
			   
				if( $(this).is(':input') && ($(this).attr('type') == 'checkbox' || $(this).attr('type') == 'radio') ) {
					
					$(this).wrap('<div class="lcs_wrap"></div>');
					$(this).parent().append(structure);
					
					$(this).parent().find('.lcs_switch').addClass('lcs_'+ $(this).attr('type') +'_switch');
				}
			}
		});
	};	
	
	
	
	// handlers
	$(document).ready(function() {
		
		// on click
		$(document).delegate('.lcs_switch:not(.lcs_disabled)', 'click tap', function(e) {

			if( $(this).hasClass('lcs_on') ) {
				if( !$(this).hasClass('lcs_radio_switch') ) { // not for radio
					$(this).lcs_off();
				}
			} else {
				$(this).lcs_on();	
			}
		});
		
		
		// on checkbox status change
		$(document).delegate('.lcs_wrap input', 'change', function() {
			if( $(this).is(':checked') ) {
				$(this).lcs_on();
			} else {
				$(this).lcs_off();	
			}	
		});
		
	});
	
})($);
