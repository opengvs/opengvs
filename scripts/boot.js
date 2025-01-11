
var bootPATH = bootPath = __CreateJSPath("boot.js");

mini_debugger = true;                                           //

var skin = getCookie("miniuiSkin") || 'default';             //skin cookie    cupertino
var mode = getCookie("miniuiMode") || 'default';                 //mode cookie     medium     

//miniui
document.write('<script src="' + bootPATH + 'jquery.min.js" type="text/javascript"></sc' + 'ript>');
document.write('<script src="' + bootPATH + 'miniui/miniui.js" type="text/javascript" ></sc' + 'ript>');
document.write('<link href="' + bootPATH + '../res/fonts/font-awesome/css/font-awesome.min.css" rel="stylesheet" type="text/css" />');
document.write('<link href="' + bootPATH + 'miniui/themes/default/miniui.css" rel="stylesheet" type="text/css" />');

//common
document.write('<link href="' + bootPATH + '../res/css/common.css" rel="stylesheet" type="text/css" />');
document.write('<script src="' + bootPATH + '../res/js/common.js" type="text/javascript" ></sc' + 'ript>');

//skin
if (skin && skin != "default") document.write('<link href="' + bootPATH + 'miniui/themes/' + skin + '/skin.css" rel="stylesheet" type="text/css" />');

//mode
if (mode && mode != "default") document.write('<link href="' + bootPATH + 'miniui/themes/default/' + mode + '-mode.css" rel="stylesheet" type="text/css" />');

//icon
document.write('<link href="' + bootPATH + 'miniui/themes/icons.css" rel="stylesheet" type="text/css" />');




////////////////////////////////////////////////////////////////////////////////////////
function getCookie(sName) {
    var aCookie = document.cookie.split("; ");
    var lastMatch = null;
    for (var i = 0; i < aCookie.length; i++) {
        var aCrumb = aCookie[i].split("=");
        if (sName == aCrumb[0]) {
            lastMatch = aCrumb;
        }
    }
    if (lastMatch) {
        var v = lastMatch[1];
        if (v === undefined) return v;
        return unescape(v);
    }
    return null;
}

function __CreateJSPath(js) {
    var scripts = document.getElementsByTagName("script");
    var path = "";
    for (var i = 0, l = scripts.length; i < l; i++) {
        var src = scripts[i].src;
        if (src.indexOf(js) != -1) {
            var ss = src.split(js);
            path = ss[0];
            break;
        }
    }
    var href = location.href;
    href = href.split("#")[0];
    href = href.split("?")[0];
    var ss = href.split("/");
    ss.length = ss.length - 1;
    href = ss.join("/");
    if (path.indexOf("https:") == -1 && path.indexOf("http:") == -1 && path.indexOf("file:") == -1 && path.indexOf("\/") != 0) {
        path = href + "/" + path;
    }
    return path;
}

/*
var WinAlertss = window.alert;
window.alert = function (e) {
if (e != null && e.toString().indexOf("试用到期") > -1) {
//和谐了
console.log(e.toString())
}
else {
WinAlertss(e);
}
};
*/
var WinAlerts = window.alert;

window.alert = function (e) {
if (e != null && e.indexOf("提示内容")>-1)
{ 
//和谐了
}
else
{
WinAlerts (e);
}

};
//此方法在旧版本中管用，新版本破解方法（测试于jQuery MiniUI 3.9 Date : 2019-03-21 版本）：
var execscript = null;
window.execScript = function (e)
{
    if (e.toString().indexOf("试用到期") > -1) {
      //  console.log(1111);
    }
    else {
        eval(e);
    }
}

