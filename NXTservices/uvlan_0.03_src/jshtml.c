const char *jshtml_part1 = 
"<html>\n"
"<head>\n"
"<title>UVLAN Stats</title>\n"
"<style type=\"text/css\">\n"
"TD { text-align:right; }\n"
"</style>\n"
"<script>\n"
"header = ['Peer','MAC Addr','IP Addr','Total<br>Rcvd/Sent','Rate<br>Rcvd/Sent'];\n"
"indata = [\n";

const char *jshtml_part2 = 
"];\n"
"data = [];\n"
"\n"
"function adjSort(by) {\n"
"  var i,sortby=0;\n"
"  if (by == null || by.length == 0) by = document.cookie;\n"
"  else document.cookie = by;\n"
"\n"
"  for (i=0; i<header.length; i++)\n"
"    if (header[i] == by)\n"
"      sortby = i;\n"
"  \n"
"  if (sortby == 4) sortby += 1;\n"
"  for (i=0; i<indata.length; i++) data[i] = indata[i];\n"
"  sortData(0, Math.floor(data.length/7)-1, sortby);\n"
"  doStats();\n"
"}\n"
"\n"
"function sortData(from, to, sortby) {\n"
"  var ptr1=from, ptr2=to, now=Math.floor((from+to)/2);\n"
"  tmp = ['','','','','','',''];\n"
"\n"
"  do {\n"
"    while(strcmp(data[ptr1*7+sortby],data[now*7+sortby]) > 0)\n"
"      ptr1++;\n"
"    while(strcmp(data[ptr2*7+sortby],data[now*7+sortby]) < 0)\n"
"      ptr2--;\n"
"    if (ptr1 <= ptr2) {\n"
"      // swap\n"
"      for (var i=0; i<7; i++) {\n"
"        tmp = data[ptr1*7+i];\n"
"        data[ptr1*7+i] = data[ptr2*7+i];\n"
"        data[ptr2*7+i] = tmp;\n"
"      }\n"
"      ptr1++; ptr2--;\n"
"    }\n"
"  } while(ptr1 <= ptr2);\n"
"\n"
"  if (from < ptr2)\n"
"    sortData(from, ptr2, sortby);\n"
"  if (ptr1 < to)\n"
"    sortData(ptr1, to, sortby);\n"
"}\n"
"\n"
"function doStats() {\n"
"  var html = '';\n"
"\n"
"  html += '<table border=1 cellpadding=3 cellspacing=0 width=\"100%\"><tr>';\n"
"  for (var i=0; i<header.length; i++) {\n"
"    if (i==3||i==4) s=2;\n"
"    else            s=1;\n"
"    html += '<th colspan='+s+'><a href=\"javascript:adjSort(\\''+ header[i] +'\\');\">'+ header[i] +'</a></th>';\n"
"  }\n"
"  html += '</tr>';\n"
"\n"
"  for (var i=0; i<data.length/7; i++) {\n"
"    html += '<tr>';\n"
"    for (var j=0; j<7; j++) {\n"
"      e='';\n"
"      h='td>';\n"
"      d = data[i*7+j];\n"
"      if (j==0) h='th>';\n"
"      if (j==1) e='<font style=\"font-family:monospace;\">';\n"
"      if (j==2) d=ipfy(d);\n"
"      if (j>=3) d=sizefy(d);\n"
"      html += '<'+h+e+d+'</'+h;\n"
"    }\n"
"    html += '</tr>';\n"
"  }\n"
"  html += '</table>';\n"
"  setHTML('mainstats', html);\n"
"}\n"
"\n"
"function sizefy(n) {\n"
"  if (n < 1024) return n +'&nbsp;B';\n"
"  n = Math.floor(n/10.24)/100;\n"
"  if (n < 1024) return n +'&nbsp;KB';\n"
"  n = Math.floor(n/10.24)/100;\n"
"  if (n < 1024) return n +'&nbsp;MB';\n"
"  n = Math.floor(n/10.24)/100;\n"
"  if (n < 1024) return n +'&nbsp;GB';\n"
"  n = Math.floor(n/10.24)/100;\n"
"  if (n < 1024) return n +'&nbsp;TB';\n"
"}\n"
"function ipfy(n) {\n"
"  return (n>>>24) +'.'+ ((n>>>16)&0xff) +'.'+ ((n>>>8)&0xff) +'.'+ (n&0xff);\n"
"}\n"
"function setHTML(id, html) {\n"
"  document.getElementById(id).innerHTML = html;\n"
"}\n"
"function addHTML(id, html) {\n"
"  document.getElementById(id).innerHTML += html;\n"
"}\n"
"function strcmp(a,b) {\n"
"  if (typeof a == 'number') return a - b;\n"
"  for (var i=0; i<a.length && i<b.length; i++) {\n"
"    if (a.charCodeAt(i) > b.charCodeAt(i)) return 1;\n"
"    if (a.charCodeAt(i) < b.charCodeAt(i)) return -1;\n"
"  }\n"
"  if (a.length > b.length) return 1;\n"
"  if (a.length < b.length) return -1;\n"
"  return 0;\n"
"}\n"
"function startAll() {\n"
"  setTimeout('window.location.reload()', 10*1000);\n"
"  adjSort();\n"
"}\n"
"\n"
"window.onload = startAll;\n"
"</script>\n"
"<table border=1 cellpadding=3 cellspacing=0>\n"
"<tr><td>\n"
" <table width=\"100%\" border=1 cellpadding=3 cellspacing=0>\n"
" <tr><th>Traffic Source</th><th>Total KBytes</th><th>Current Rate</th></tr>\n";

const char *jshtml_part3 =
" </table>\n"
"</td></tr>\n"
"<tr><td>\n"
"<div id=\"mainstats\"></div>\n"
"</td></tr>\n"
"</table>\n"
"<div id=\"debug\"></div>\n";