// JSROOTPainter.js
//
// core methods for Javascript ROOT Graphics.
//

// The "source_dir" variable is defined in JSRootInterface.js

var d, key_tree;

(function(){

   if (typeof JSROOTPainter == "object"){
      var e1 = new Error("JSROOTPainter is already defined");
      e1.source = "JSROOTPainter.js";
      throw e1;
   }

   if (typeof dTree != "function") {
      var e1 = new Error("This extension requires dtree.js");
      e1.source = "JSROOTIO.RootFile.js";
      throw e1;
   }

   if (typeof Highcharts != "object") {
      var e1 = new Error("This extension requires highcharts.js");
      e1.source = "JSROOTIO.RootFile.js";
      throw e1;
   }

   // Initialize Custom colors
   var root_colors = new Array(
      'rgb(255,255,255)',
      'rgb(0,0,0)',
      'rgb(255,0,0)',
      'rgb(0,255,0)',
      'rgb(0,0,255)',
      'rgb(255,255,0)',
      'rgb(255,0,255)',
      'rgb(0,255,255)',
      'rgb(89,211,84)',
      'rgb(89,84,216)',
      'rgb(254,254,254)',
      'rgb(191,181,173)',
      'rgb(76,76,76)',
      'rgb(102,102,102)',
      'rgb(127,127,127)',
      'rgb(153,153,153)',
      'rgb(178,178,178)',
      'rgb(204,204,204)',
      'rgb(229,229,229)',
      'rgb(242,242,242)',
      'rgb(204,198,170)',
      'rgb(204,198,170)',
      'rgb(193,191,168)',
      'rgb(186,181,163)',
      'rgb(178,165,150)',
      'rgb(183,163,155)',
      'rgb(173,153,140)',
      'rgb(155,142,130)',
      'rgb(135,102,86)',
      'rgb(175,206,198)',
      'rgb(132,193,163)',
      'rgb(137,168,160)',
      'rgb(130,158,140)',
      'rgb(173,188,198)',
      'rgb(122,142,153)',
      'rgb(117,137,145)',
      'rgb(104,130,150)',
      'rgb(109,122,132)',
      'rgb(124,153,209)',
      'rgb(127,127,155)',
      'rgb(170,165,191)',
      'rgb(211,206,135)',
      'rgb(221,186,135)',
      'rgb(188,158,130)',
      'rgb(198,153,124)',
      'rgb(191,130,119)',
      'rgb(206,94,96)',
      'rgb(170,142,147)',
      'rgb(165,119,122)',
      'rgb(147,104,112)',
      'rgb(211,89,84)');

   var root_markers = new Array(
      'circle', 'circle', 'diamond', 'diamond', 'circle', 'diamond', 'circle',
      'circle', 'circle', 'circle', 'circle', 'circle', 'circle', 'circle',
      'circle', 'circle', 'circle', 'circle', 'circle', 'circle', 'circle',
      'square', 'triangle', 'triangle-down', 'circle', 'square', 'triangle',
      'diamond', 'diamond', 'diamond', 'diamond', 'diamond', 'triangle-down',
      'diamond', 'diamond');

   JSROOTPainter = {};

   JSROOTPainter.version = "1.4 2012/02/24";

   JSROOTPainter.displayObject = function(obj, idx, options) {
      if (obj['_typename'].match(/\bTH1/) ||
          obj['_typename'].match(/\bTH2/)) {
         JSROOTPainter.displayHistogram(obj, idx, options);
         return true;
      }
      else if (obj['_typename'] == 'JSROOTIO.TGraph') {
         JSROOTPainter.displayGraph(obj, idx, options);
         return true;
      }
      else
         return false;
   };

   JSROOTPainter.displayHistogram = function(histo, idx, options) {
      var i, j;
      var logx = false, logy = false, logz = false, grix = true, gridy = true;
      var time_scalex = 1, time_scaley = 1;
      if (typeof(options) != 'undefined') {
         logx = options[logx];
         logy = options[logy];
         logz = options[logz];
         gridx = options[gridx];
         gridx = options[gridy];
      }
      // check for axis scale format, and convert if required
      // (highcharts time unit is in milliseconds)
      var xaxis_type = logx ? 'logarithmic' : 'linear';
      if (histo['fXaxis']['fTimeDisplay']) {
         time_scalex = 1000; xaxis_type = 'datetime';
      }
      var yaxis_type = logy ? 'logarithmic' : 'linear';
      if (histo['fYaxis']['fTimeDisplay']) {
         time_scaley = 1000; yaxis_type = 'datetime';
      }
      if (histo['_typename'].match(/\bTH1/)) {
         var scale = (histo['fXaxis']['fXmax'] - histo['fXaxis']['fXmin']) /
                      histo['fXaxis']['fNbins'];
         // if time format, convert from seconds to milliseconds (highcharts unit)
         scale *= time_scalex;
         var bin_data = new Array();
         var legend_stats = '';
         for (i=0; i<histo['fArray'].length; ++i) {
            bin_data.push(histo['fArray'][i]);
         }
         var fillcolor = root_colors[histo['fFillColor']];
         var linecolor = root_colors[histo['fLineColor']];
         if (histo['fFillColor'] == 0) {
            fillcolor = '#4572A7';
         }
         if (typeof(histo['fFunctions']) != 'undefined') {
            for (i=0; i<histo['fFunctions'].length; ++i) {
               if (histo['fFunctions'][i]['fName'] == 'stats') {
                  for (j=0; j<histo['fFunctions'][i]['fLines'].length; ++j) {
                     legend_stats += histo['fFunctions'][i]['fLines'][j].fTitle;
                     legend_stats += '<br/>';
                  }
               }
            }
         }
         var render_to = 'histogram' + idx;
         chart = new Highcharts.Chart({
            chart: {
               renderTo:render_to,
               backgroundColor:'#eee',
               borderWidth:1,
               borderColor:'#ccc',
               plotBackgroundColor:'#fff',
               plotBorderWidth:1,
               plotBorderColor:'#ccc',
               height:400,
               reflow:true,
               zoomType: "xy"
            },
            credits: { enabled: false },
            exporting: { enabled: true },
            title: { text: histo['fTitle'] },
            legend: { enabled: false },
            tooltip: {
               enabled:true,
               borderWidth:1,
               //crosshairs: [true, true],
               formatter:function() {
                  var x_val = new Number(this.x);
                  return '<b>X Value:</b> ' + x_val.toPrecision(6) + '<br/>' +
                         '<b>Entries:</b> ' + this.y;
               }
            },
            legend: {
               enabled: legend_stats.length > 1 ? true : false,
               layout: 'vertical',
               backgroundColor: '#FFFFFF',
               floating: true,
               align: 'right',
               verticalAlign: 'top',
               x: -10,
               y: 45,
               labelFormatter: function() {
                  return legend_stats;
               }
            },
            xAxis: {
               type: xaxis_type,
               title: {
                  text: histo['fXaxis']['fTitle'],
                  style: { fontWeight: 'normal' }
               },
               labels: { y:20 },
               gridLineColor:'#e9e9e9',
               gridLineWidth:1,
               minPadding:0,
               maxPadding:0,
               offset: 0.2,
               startOnTick:true,
               tickLength:5,
               tickColor:'#ccc'
            },
            yAxis: {
               type: yaxis_type,
               title: {
                  text: histo['fYaxis']['fTitle'],
                  style: { fontWeight: 'normal' }
               },
               gridLineColor:'#e9e9e9',
               lineColor:'#ccc',
               maxPadding:0.1,
               minPadding:0.02,
               tickWidth:1,
               tickLength:5,
               tickColor:'#ccc'
            },
            series: [{
               name:'Bins',
               type: 'column',
               data: bin_data,
               color: fillcolor,
               animation: false,
               groupPadding: 0,
               pointPadding: 0,
               borderRadius: 0,
               borderColor: fillcolor,
               borderWidth: 0,
               pointStart: histo['fXaxis']['fXmin'],
               pointInterval: scale,
               shadow: false,
               stickyTracking: false
            }]
         });
      }
      else if (histo['_typename'].match(/\bTH2/)) {
         var nbinsx = histo['fXaxis']['fNbins'];
         var nbinsy = histo['fYaxis']['fNbins'];
         var scalex = (histo['fXaxis']['fXmax'] - histo['fXaxis']['fXmin']) /
                       histo['fXaxis']['fNbins'];
         var scaley = (histo['fYaxis']['fXmax'] - histo['fYaxis']['fXmin']) /
                       histo['fYaxis']['fNbins'];
         // if time format, convert from seconds to milliseconds (highcharts unit)
         scalex *= time_scalex;
         scaley *= time_scaley;
         var fillcolor = root_colors[histo['fFillColor']];
         var linecolor = root_colors[histo['fLineColor']];
         if (histo['fLineColor'] > 50) {
            linecolor = '#4572A7';
         }
         var maxbin = -1e32, minbin = 1e32;
         for (var n=0; n<histo['fArray'].length; ++n) {
            if (histo['fArray'][n] > maxbin) maxbin = histo['fArray'][n];
            if (histo['fArray'][n] < minbin) minbin = histo['fArray'][n];
         }
         var legend_stats = '';
         if (typeof(histo['fFunctions']) != 'undefined') {
            for (i=0; i<histo['fFunctions'].length; ++i) {
               if (histo['fFunctions'][i]['fName'] == 'stats') {
                  for (j=0; j<histo['fFunctions'][i]['fLines'].length; ++j) {
                     legend_stats += histo['fFunctions'][i]['fLines'][j].fTitle;
                     legend_stats += '<br/>';
                  }
               }
            }
         }
         var scalebin = 20.0 * ((maxbin - minbin) / (maxbin * maxbin));
         var render_to = 'histogram' + idx;
         var bin_data = new Array();
         for (i=0; i<nbinsx; ++i) {
            for (var j=0; j<nbinsy; ++j) {
               var bin_content = histo.GetBinContent(i, j);
               if (bin_content > minbin) {
                  var point = {
                     color:null,
                     events:null,
                     id:null,
                     marker:{
                        enabled:true,
                        fillColor:fillcolor,
                        lineColor:linecolor,
                        lineWidth:1,
                        radius:Math.sqrt(bin_content*scalebin),
                        states:null,
                        symbol:"square" // null
                     },
                     name:bin_content,
                     sliced:false,
                     x:histo['fXaxis']['fXmin'] + (i*scalex),
                     y:histo['fYaxis']['fXmin'] + (j*scaley)
                  };
                  bin_data.push(point);
               }
            }
         }
         chart = new Highcharts.Chart({
            chart: {
               renderTo:render_to,
               defaultSeriesType: 'scatter',
               backgroundColor:'#eee',
               borderWidth:1,
               borderColor:'#ccc',
               plotBackgroundColor:'#fff',
               plotBorderWidth:1,
               plotBorderColor:'#ccc',
               height:400,
               reflow:true,
               zoomType: "xy"
            },
            credits: { enabled: false },
            exporting: { enabled: true },
            title: { text: histo['fTitle'] },
            legend: { enabled: false },
            title: { text: histo['fTitle'] },
            legend: {
               enabled: legend_stats.length > 1 ? true : false,
               layout: 'vertical',
               backgroundColor: '#FFFFFF',
               floating: true,
               align: 'right',
               verticalAlign: 'top',
               x: -10,
               y: 45,
               labelFormatter: function() {
                  return legend_stats;
               }
            },
            xAxis: {
               type: xaxis_type,
               title: {
                  text: histo['fXaxis']['fTitle'],
                  style: { fontWeight: 'normal' }
               },
               gridLineColor:'#e9e9e9',
               gridLineWidth:1,
               minPadding:0,
               maxPadding:0,
               offset: 0.2,
               startOnTick:true,
               endOnTick: true,
               tickLength:5,
               tickColor:'#ccc',
               showLastLabel: true
            },
            yAxis: {
               type: yaxis_type,
               title: {
                  text: histo['fYaxis']['fTitle'],
                  style: { fontWeight: 'normal' }
               },
               gridLineColor:'#e9e9e9',
               lineColor:'#ccc',
               maxPadding:0,
               minPadding:0,
               tickWidth:1,
               tickLength:5,
               tickColor:'#ccc'
            },
            tooltip: {
               enabled:true,
               borderWidth:1,
               formatter: function() {
                  var x_val = new Number(this.x);
                  var y_val = new Number(this.y);
                  return 'X Value: ' + x_val.toPrecision(6) + '<br/>' +
                         'Y Value: ' + y_val.toPrecision(6) + '<br/>' +
                         'Entries: ' + this.point.name;
               }
            },
            series: [{
               name: 'Bins',
               animation: false,
               //color: null, //fillcolor,
               data: bin_data,
               pointStart: histo['fXaxis']['fXmin'],
               stickyTracking: false
            }]
         });
      }
   };

   JSROOTPainter.displayGraph = function(graph, idx, options) {
      var logx = false, logy = false, logz = false, grix = true, gridy = true;
      var scalex = 1, scaley = 1;
      if (typeof(options) != 'undefined') {
         logx = options[logx];
         logy = options[logy];
         logz = options[logz];
         gridx = options[gridx];
         gridx = options[gridy];
      }
      // check for axis scale format, and convert if required
      // (highcharts time unit is in milliseconds)
      var xaxis_type = logx ? 'logarithmic' : 'linear';
      if (graph['fHistogram']['fXaxis']['fTimeDisplay']) {
         scalex = 1000; xaxis_type = 'datetime';
      }
      var yaxis_type = logy ? 'logarithmic' : 'linear';
      if (graph['fHistogram']['fYaxis']['fTimeDisplay']) {
         scaley = 1000; yaxis_type = 'datetime';
      }
      if (graph['_typename'] == 'JSROOTIO.TGraph') {
         var graph_data = new Array();
         var seriesType = 'line';
         var showMarker = true;
         if (graph['_options']) {
            if ((graph['_options'].indexOf('P') == -1) &&
                (graph['_options'].indexOf('p') == -1) &&
                (graph['_options'].indexOf('*') == -1)) {
               showMarker = false;
            }
            if ((graph['_options'].indexOf('L') == -1) &&
                (graph['_options'].indexOf('l') == -1) &&
                (graph['_options'].indexOf('C') == -1) &&
                (graph['_options'].indexOf('c') == -1)) {
               seriesType = 'scatter';
            }
         }
         for (var i=0; i<graph['fNpoints']; ++i) {
            var point = {
               color:null,
               events:null,
               id:null,
               marker:{
                  enabled:showMarker,
                  fillColor:root_colors[graph['fMarkerColor']],
                  lineColor:root_colors[graph['fMarkerColor']],
                  lineWidth:1,
                  radius:1+(5*graph['fMarkerSize']),
                  states:null,
                  symbol:root_markers[graph['fMarkerStyle']]
               },
               name:'',
               sliced:false,
               x:graph['fX'][i] * scalex,
               y:graph['fY'][i] * scaley
            };
            graph_data.push(point);
         }
         var render_to = 'histogram' + idx;
         chart = new Highcharts.Chart({
            chart: {
               renderTo:render_to,
               defaultSeriesType:seriesType,
               backgroundColor:'#eee',
               borderWidth:1,
               borderColor:'#ccc',
               plotBackgroundColor:'#fff',
               plotBorderWidth:1,
               plotBorderColor:'#ccc',
               height:400,
               reflow:true,
               zoomType: "xy"
            },
            credits: { enabled: false },
            exporting: { enabled: true },
            title: { text: graph['fTitle'] },
            legend: { enabled: false },
            tooltip: {
               enabled:true,
               borderWidth:1,
               formatter:function() {
                  var x_val = new Number(this.x);
                  var y_val = new Number(this.y);
                  return graph['fTitle'] + '<br/>' +
                         '<b>X Value:</b> ' + x_val.toPrecision(6) + '<br/>' +
                         '<b>Y Value:</b> ' + y_val.toPrecision(6);
               }
            },
            plotOptions: {
               line: {
                  color: root_colors[graph['fLineColor']],
                  lineWidth: graph['fLineWidth']
               }
            },
            xAxis: {
               type: xaxis_type,
               title: {
                  text: graph['fHistogram']['fXaxis']['fTitle'],
                  style: { fontWeight: 'normal' }
               },
               gridLineColor:'#e9e9e9',
               gridLineWidth:1,
               lineColor:'#ccc',
               minPadding:0,
               offset: 0.2,
               //startOnTick:true,  // <-- To bypass a bug on Safari :(
               endOnTick: true,
               tickLength:5,
               tickColor:'#ccc',
               showLastLabel: true
            },
            yAxis: {
               type: yaxis_type,
               title: {
                  text: graph['fHistogram']['fYaxis']['fTitle'],
                  style: { fontWeight: 'normal' }
               },
               gridLineColor:'#e9e9e9',
               gridLineWidth:1,
               lineColor:'#ccc',
               minPadding:0,
               tickWidth:1,
               tickLength:5,
               tickColor:'#ccc'
            },
            series: [{
               data: graph_data,
               animation: false,
               borderRadius: 0,
               pointStart: graph['fHistogram']['fXaxis']['fXmin'],
               shadow: false,
               stickyTracking: false
            }]
         });
      }
   };

   JSROOTPainter.displayListOfKeyDetails = function(keys, container) {
      delete key_tree;
      var content = "<p><a href='javascript: key_tree.openAll();'>open all</a> | <a href='javascript: key_tree.closeAll();'>close all</a></p>";
      key_tree = new dTree('key_tree');
      key_tree.add(0, -1, 'File Content');
      var k = 1;
      var pattern_th1 = /TH1/g;
      var pattern_th2 = /TH2/g;
      var tree_link = "";
      for (var i=0; i<keys.length; ++i) {
         tree_link = "";
         if (keys[i]['className'] == "TDirectory")
            tree_link = "javascript: showDirectory('"+keys[i]['name']+"',"+keys[i]['cycle']+","+(i+1)+");";
         else if (keys[i]['name'] == "StreamerInfo")
            tree_link = "javascript: displayStreamerInfos(gFile.fStreamerInfo.fStreamerInfos);";
         else
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
         if (keys[i]['name'] != "" && keys[i]['className'] != "TFile")
            key_tree.add(k, 0, keys[i]['name']+";"+keys[i]['cycle'], tree_link, keys[i]['className']); k++;
      }
      // key details (take time with files containing many keys)
      for (var j=0; j<keys.length; ++j) {
         if (keys[j]['name'] == "" || keys[j]['className'] == "TFile") continue;
         var d_t = new Date(keys[j]['datime']['year'], keys[j]['datime']['month'],
                            keys[j]['datime']['day'], keys[j]['datime']['hour'],
                            keys[j]['datime']['min'], keys[j]['datime']['sec'], 0);
         key_tree.add(k, j+1, "Title:       " + keys[j]['title']); ++k;
         key_tree.add(k, j+1, "ClassName:   " + keys[j]['className']); ++k;
         key_tree.add(k, j+1, "Cycle:       " + keys[j]['cycle']); ++k;
         key_tree.add(k, j+1, d_t.toString()); ++k;
         key_tree.add(k, j+1, "Offset:      " + keys[j]['offset']); ++k;
         key_tree.add(k, j+1, "Key Length:  " + keys[j]['keyLen']); ++k;
         key_tree.add(k, j+1, "Data Offset: " + keys[j]['dataoffset']); ++k;
         key_tree.add(k, j+1, "N Bytes:     " + keys[j]['nbytes']); ++k;
         key_tree.add(k, j+1, "Obj Length:  " + keys[j]['objLen']); ++k;
      }
      content += key_tree;
      $(container).append(content);
   };

   JSROOTPainter.displayListOfKeys = function(keys, container) {
      delete key_tree;
      var content = "<p><a href='javascript: key_tree.openAll();'>open all</a> | <a href='javascript: key_tree.closeAll();'>close all</a></p>";
      key_tree = new dTree('key_tree');
      key_tree.config.useCookies = false;
      key_tree.add(0, -1, 'File Content');
      var k = 1;
      var tree_link = "";
      for (var i=0; i<keys.length; ++i) {
//         tree_link = "javascript: //class "+keys[i]['className'];
         var message = "Coming soon... " + keys[i]["className"]+" is not yet implemented.";
         tree_link = "javascript:  alert('" + message + "')";
         var node_img = source_dir+'img/page.gif';
         if (keys[i]['className'].match(/\bTH1/)  ||
             keys[i]['className'].match(/\bTH2/)  ||
             keys[i]['className'] == 'TGraph') {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            if (keys[i]['className'].match(/\bTProfile/))
                tree_link = "javascript:  alert('" + message + "')";
            node_img = source_dir+'img/graphical.png';
         }
         else if (keys[i]['className'] ==  'TProfile') {
            node_img = source_dir+'img/graphical.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['name'] == "StreamerInfo") {
            tree_link = "javascript: displayStreamerInfos(gFile.fStreamerInfo.fStreamerInfos);";
            node_img = source_dir+'img/question.gif';
         }
         else if (keys[i]['className'] == "TDirectory") {
            tree_link = "javascript: showDirectory('"+keys[i]['name']+"',"+keys[i]['cycle']+","+(i+1)+");";
            node_img = source_dir+'img/folder.gif';
         }
         else if (keys[i]['className'].match('TTree') ||
                  keys[i]['className'].match('TNtuple')) {
//            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/tree_t.png';
         }
         else if (keys[i]['className'].match('TGeoManager') ||
                  keys[i]['className'].match('TGeometry')) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/folder.gif';
         }
         else if (keys[i]['className'].match('TCanvas')) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_title = keys[i]['name'];
         }
         if (keys[i]['name'] != "" && keys[i]['className'] != "TFile")
            if (keys[i]['className'] == "TDirectory")
               key_tree.add(k, 0, keys[i]['name']+";"+keys[i]['cycle'], tree_link, keys[i]['name'], '', node_img, 
                            source_dir+'img/folderopen.gif');
            else
               key_tree.add(k, 0, keys[i]['name']+";"+keys[i]['cycle'], tree_link, keys[i]['name'], '', node_img);
            k++;
      }
      content += key_tree;
      $(container).append(content);
   };

   JSROOTPainter.addDirectoryKeys = function(keys, container, dir_id) {
      var pattern_th1 = /TH1/g;
      var pattern_th2 = /TH2/g;
      var tree_link = "";
      var content = "<p><a href='javascript: key_tree.openAll();'>open all</a> | <a href='javascript: key_tree.closeAll();'>close all</a></p>";
      var k = key_tree.aNodes.length;
      //var dir_name = key_tree.aNodes[dir_id]['name'];
      var dir_name = key_tree.aNodes[dir_id]['title'];
      for (var i=0; i<keys.length; ++i) {
         var disp_name = keys[i]['name'];
         keys[i]['name'] = dir_name + "/" + keys[i]['name'];
//         tree_link = "javascript: //class " + keys[i]['className'];
         var message = "Coming soon... " + keys[i]["className"]+" is not yet implemented.";
         tree_link = "javascript:  alert('" + message + "')";
         var node_img = source_dir+'img/page.gif';
         var node_title = keys[i]['className'];
         if (keys[i]['className'].match(/\bTH1/) ||
             keys[i]['className'].match(/\bTH2/) ||
             keys[i]['className'] == 'TGraph') {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/graphical.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'] ==  'TProfile') {
            node_img = source_dir+'img/graphical.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['name'] == "StreamerInfo") {
            tree_link = "javascript: displayStreamerInfos(gFile.fStreamerInfo.fStreamerInfos);";
            node_img = source_dir+'img/question.gif';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'] == "TDirectory") {
            tree_link = "javascript: showDirectory('"+keys[i]['name']+"',"+keys[i]['cycle']+","+k+");";
            node_img = source_dir+'img/folder.gif';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'].match('TTree') ||
                  keys[i]['className'].match('TNtuple')) {
            node_img = source_dir+'img/tree_t.png';
         }
         else if (keys[i]['className'].match('TCanvas')) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_title = keys[i]['name'];
         }
         if (keys[i]['name'] != "" && keys[i]['className'] != "TFile") {
            if (keys[i]['className'] == "TDirectory")
               key_tree.add(k, dir_id, disp_name+";"+keys[i]['cycle'], tree_link, node_title, '', node_img, 
                            source_dir+'img/folderopen.gif');
            else
               key_tree.add(k, dir_id, disp_name+";"+keys[i]['cycle'], tree_link, node_title, '', node_img);
            k++;
         }
      }
      content += key_tree;
      $(container).append(content);
      key_tree.openTo(dir_id, true);
   }

   JSROOTPainter.displayStreamerInfos = function(streamerInfo, container) {

      delete d;
      var content = "<p><a href='javascript: d.openAll();'>open all</a> | <a href='javascript: d.closeAll();'>close all</a></p>";
      d = new dTree('d');
      d.config.useCookies = false;
      d.add(0, -1, 'Streamer Infos');

      var k = 1;
      var pid = 0;
      var cid = 0;
      var key;
      for (key in streamerInfo) {
         var entry = streamerInfo[key]['name'];
         d.add(k, 0, entry); k++;
      }
      var j=0;
      for (key in streamerInfo) {
         if (typeof(streamerInfo[key]['checksum']) != 'undefined')
            d.add(k, j+1, "Checksum: " + streamerInfo[key]['checksum']); ++k;
         if (typeof(streamerInfo[key]['classversion']) != 'undefined')
            d.add(k, j+1, "Class Version: " + streamerInfo[key]['classversion']); ++k;
         if (typeof(streamerInfo[key]['title']) != 'undefined')
            d.add(k, j+1, "Title: " + streamerInfo[key]['title']); ++k;
         if (typeof(streamerInfo[key]['elements']) != 'undefined') {
            d.add(k, j+1, "Elements"); pid=k; ++k;
            for (var l=0; l<streamerInfo[key]['elements']['array'].length; ++l) {
               if (typeof(streamerInfo[key]['elements']['array'][l]['element']) != 'undefined') {
                  d.add(k, pid, streamerInfo[key]['elements']['array'][l]['element']['name']); cid=k; ++k;
                  d.add(k, cid, streamerInfo[key]['elements']['array'][l]['element']['title']); ++k;
                  d.add(k, cid, streamerInfo[key]['elements']['array'][l]['element']['typename']); ++k;
               }
               else if (typeof(streamerInfo[key]['elements']['array'][l]['name']) != 'undefined') {
                  d.add(k, pid, streamerInfo[key]['elements']['array'][l]['name']); cid=k; ++k;
                  d.add(k, cid, streamerInfo[key]['elements']['array'][l]['title']); ++k;
                  d.add(k, cid, streamerInfo[key]['elements']['array'][l]['typename']); ++k;
               }
            }
         }
         else if (typeof(streamerInfo[key]['array']) != 'undefined') {
            for (var l=0; l<streamerInfo[key]['array'].length; ++l) {
               d.add(k, j+1, streamerInfo[key]['array'][l]['str']); ++k;
            }
         }
         ++j;
      }
      content += d;
      $(container).html(content);
   };

})();


// JSROOTPainter.js ends
