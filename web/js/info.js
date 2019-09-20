/*

Copyright 2019 Applied Research Center for Computer Networks

*/


'use strict';

Info = function () {
    return {
        init            : init,
        get_ctrl_info   : get_ctrl_info,
        get_switches_info : get_switches_info,
        updateRXCtrlStats : updateRXCtrlStats,
        updateTXCtrlStats: updateTXCtrlStats,
        updateSwitchStats: updateSwitchStats,

        controller : {
            /* controller info */
            port      : 6653,
            threads   : 12,
            coreMask  : 0xF,
            services  : ['Device Manual',
                         'Link Discovery',
                         'Topology',
                         'Forwarding'],
            apps      : ['OF-Server', 'Topology', 'Recovery'],
            logLevel  : 'HIGH',
            uptime    : 0,
            switchN   : 0,
            packetIn  : [57,22,43,17,69],
            switches  : [5,2,4,1,6],
            activity  : {'superlongnameforapp1': 43, 'app2': 2, 'app3': 55},
            load      : [25,8,88,69],
            start_time: '----',

            /* controller stats */
            rx_packets: 0,
            rx_packets_freq: 0,
            pkt_in_packets: 0,
            pkt_in_packets_freq: 0,
            tx_packets: 0,
            tx_packets_freq: 0,

            last_update: '----',
        },

        ctrl_info  : false,
        switches_info : false,
        update_interval : 1000,

        // for controller charts
        rx_data : [],
        pkt_in_data : [],
        tx_data : [],

        // for switches charts
        switches_dpid_flag : false,
        switches_stats : [], // results of current request
        switches_data : [], // data for charts (100 points)

        // for charts
        rx_ymax : 0,
        tx_ymax : 0,
        sw_ymax : 0
    };

    function init () {
        Info.ctrl_info = document.getElementById('ctrl_info');
        Info.switches_info = document.getElementById('switches_info');

        getServerCtrlData();
        getServerCtrlNetData();

        setInterval(update, Info.update_interval);
    }

    function update () {
        getServerCtrlData();
        getServerCtrlNetData();

        // for CONTROLLER STATS
        var rx = Number(Info.controller.rx_packets_freq);
        var pkt_in = Number(Info.controller.pkt_in_packets_freq);
        var tx = Number(Info.controller.tx_packets_freq);

        // for RX ctrl chart scaling
        var rx_max = Math.max(rx, pkt_in);
        if (Info.rx_ymax == 0 || Info.rx_ymax < 1.05 * rx_max) {
            Info.rx_ymax = 1.05 * rx_max;
        }

        // rx statistics
        if (Info.rx_data.length > 0)
            Info.updateRXCtrlStats(rx,
                                 pkt_in,
                                 Info.rx_data[Info.rx_data.length-1][1],
                                 Info.pkt_in_data[Info.pkt_in_data.length-1][1],
                                 1);
        else
            Info.updateRXCtrlStats(rx, pkt_in);

        // for TX ctrl chart scaling
        if (Info.tx_ymax == 0 || Info.tx_ymax < 1.05 * tx) {
            Info.tx_ymax = 1.05 * tx;
        }

        // tx statistics
         if (Info.tx_data.length > 0)
            Info.updateTXCtrlStats(tx, Info.tx_data[Info.tx_data.length-1][1], 1);
        else
            Info.updateTXCtrlStats(tx);

        // FOR SWITCHES STATS
        if (Info.switches_stats.length > 0)
        {
            for (var i = 0; i < Info.switches_stats.length; ++i) {

                var sw_rx = Number(Info.switches_stats[i].rx_freq);
                var sw_pkt_in = Number(Info.switches_stats[i].pkt_in_freq);
                var sw_tx = Number(Info.switches_stats[i].tx_freq);

                var max = Math.max(sw_rx, sw_pkt_in, sw_tx);
                    if (Info.sw_ymax == 0 || Info.sw_ymax < 1.05 * max) {
                    Info.sw_ymax = 1.05 * max;
                }

                if (Info.switches_data[i].rx_freq_data.length > 0) {
                    Info.updateSwitchStats(
                       Info.switches_data[i].dpid,
                       sw_rx,
                       sw_pkt_in,
                       sw_tx,
                       Info.switches_data[i].rx_freq_data[Info.switches_data[i].rx_freq_data.length-1][1],
                       Info.switches_data[i].pkt_in_freq_data[Info.switches_data[i].pkt_in_freq_data.length-1][1],
                       Info.switches_data[i].tx_freq_data[Info.switches_data[i].tx_freq_data.length-1][1],
                       1 );
                } else {
                    Info.updateSwitchStats(
                        Info.switches_data[i].dpid,
                        sw_rx,
                        sw_pkt_in,
                        sw_tx);
                }
            }
        }
    }

    function ctrlOn () {
        if (Net.settings.ctrl_status)
            document.body.querySelector('.ctrl-status').classList.toggle('ctrl-status_down', false);
    }

    function ctrlOff () {
        if (Net.settings.ctrl_status)
            document.body.querySelector('.ctrl-status').classList.toggle('ctrl-status_down', true);
    }

    function getServerCtrlData () {
        Server.ajax('GET', '/of-server/info/', response, ctrlOff);

        function response(responseData) {
            ctrlOn();

            Info.controller.start_time = responseData.ctrl_start_time_t;
            Info.controller.uptime = responseData.ctrl_uptime_sec;
            Info.controller.switchN = responseData.ctrl_switches;
            // rx
            Info.controller.rx_packets_freq = responseData.ctrl_rx_ofpackets - Info.controller.rx_packets;
            Info.controller.rx_packets = responseData.ctrl_rx_ofpackets;
            // packet-in
            Info.controller.pkt_in_packets_freq = responseData.ctrl_pkt_in_ofpackets - Info.controller.pkt_in_packets;
            Info.controller.pkt_in_packets = responseData.ctrl_pkt_in_ofpackets;
            // tx
            Info.controller.tx_packets_freq = responseData.ctrl_tx_ofpackets - Info.controller.tx_packets;
            Info.controller.tx_packets = responseData.ctrl_tx_ofpackets;

            Info.controller.last_update = getCurrentTime();
            Info.ctrl_info.innerHTML = Info.get_ctrl_info();
        }
    }

    function getServerCtrlNetData() {
        Server.ajax('GET', '/switches/controlstats/', response);

        function response(responseData) {

            if (!Info.switches_dpid_flag) {

                for (var i = 0; i < responseData._size; ++i) {
                    Info.switches_stats[i] = new Array();
                     // common data
                    Info.switches_stats[i].dpid = responseData.control_stats[i].dpid;
                    Info.switches_stats[i].peer = responseData.control_stats[i].peer;
                    Info.switches_stats[i].uptime = responseData.control_stats[i].uptime;
                    //Info.switches_stats[i].connection_status = responseData.control_stats[i].connection-status;
                    Info.switches_stats[i].rx = responseData.control_stats[i].rx_ofpackets;
                    Info.switches_stats[i].rx_freq = responseData.control_stats[i].rx_ofpackets;
                    Info.switches_stats[i].tx = responseData.control_stats[i].tx_ofpackets;
                    Info.switches_stats[i].tx_freq = responseData.control_stats[i].tx_ofpackets;
                    Info.switches_stats[i].pkt_in = responseData.control_stats[i].pkt_in_ofpackets;
                    Info.switches_stats[i].pkt_in_freq = responseData.control_stats[i].pkt_in_ofpackets;

                    // for charts
                    Info.switches_data[i] = new Array();
                    Info.switches_data[i].dpid = responseData.control_stats[i].dpid;
                    Info.switches_data[i].rx_freq_data = new Array();
                    Info.switches_data[i].pkt_in_freq_data = new Array();
                    Info.switches_data[i].tx_freq_data = new Array();
                }

                Info.switches_info.innerHTML = Info.get_switches_info();
                Info.switches_dpid_flag = true;
            }

            for (var i = 0; i < responseData._size; ++i) {

                for (var j = 0; j < Info.switches_stats.length; ++j) {

                    if (Info.switches_stats[j].dpid == responseData.control_stats[i].dpid) {

                        Info.switches_stats[j].peer = responseData.control_stats[i].peer;
                        Info.switches_stats[j].uptime = responseData.control_stats[i].uptime;
                        //Info.switches_stats[i].connection_status = responseData.control_stats[i].connection-status;

                        Info.switches_stats[j].rx_freq = responseData.control_stats[i].rx_ofpackets - Info.switches_stats[j].rx;
                        Info.switches_stats[j].rx = responseData.control_stats[i].rx_ofpackets;

                        Info.switches_stats[j].pkt_in_freq = responseData.control_stats[i].pkt_in_ofpackets - Info.switches_stats[j].pkt_in;
                        Info.switches_stats[j].pkt_in = responseData.control_stats[i].pkt_in_ofpackets;

                        Info.switches_stats[j].tx_freq = responseData.control_stats[i].tx_ofpackets - Info.switches_stats[j].tx;
                        Info.switches_stats[j].tx = responseData.control_stats[i].tx_ofpackets;
                        break;
                    }
                }
            }
        }
    }       // end of getServerCtrlNetData()

    function getCurrentTime() {
        var date = new Date();
        var options = {
            hour: 'numeric',
            minute: 'numeric',
            second: 'numeric'
        };
        return date.toLocaleString("ru", options);
    }

    function get_ctrl_info() {
        var html = '';
        var i, len, us, max = 0;

        html += '<table class="arccn">';
            html += '<thead><tr>';
            html += '<th colspan="2">RUNOS Configuration </th>';
            html += '</tr></thead>';
            html += '<tr><td> START TIME   </td> <td>' + Info.controller.start_time + '</td></tr>';
            html += '<tr><td> UPTIME       </td> <td>' + Info.controller.uptime + ' sec</td></tr>';
            html += '<tr><td> PORT         </td> <td>' + Info.controller.port + '</td></tr>';
            html += '<tr><td> THREADS      </td> <td>' + Info.controller.threads + '</td></tr>';
            html += '<tr><td> SWITCHES     </td> <td>' + Info.controller.switchN + '</td></tr>';
            html += '<tr><td> APPLICATIONS </td> <td>'
                for (i = 0, len = Info.controller.apps.length; i < len; ++i) {
                    html += '<b>'+ Info.controller.apps[i] +', </b>';
                }
            html += '</td></tr>';
        html += '</table>'

        html += '<br>';
        html += '<table class="arccn">';
            html += '<thead><tr>';
            html += '<th colspan="2"> RUNOS Loading Statistics </th>';
            html += '</tr></thead>';
            // rx
            html += '<tr><td> RX PACKETS            </td> <td>' + Info.controller.rx_packets + '</td></tr>';
            html += '<tr><td> RX PACKETS FREQUENCY  </td> <td>' + (parseInt(Info.controller.rx_packets_freq * 100)) / 100 + ' packets per the last second</td></tr>';
            // pkt_in
            html += '<tr><td> PACKET-IN PACKETS     </td> <td>' + Info.controller.pkt_in_packets + '</td></tr>';
            html += '<tr><td> PACKET-IN PACKETS FREQUENCY  </td> <td>' + (parseInt(Info.controller.pkt_in_packets_freq * 100)) / 100 + ' packets per the last second</td></tr>';
            // tx
            html += '<tr><td> TX PACKETS            </td> <td>' + Info.controller.tx_packets + '</td></tr>';
            html += '<tr><td> TX PACKETS FREQUENCY  </td> <td>' + (parseInt(Info.controller.tx_packets_freq * 100)) / 100 + ' packets per the last second</td></tr>';

            html += '<tr><td> LAST UPDATE           </td> <td>' + Info.controller.last_update + '</td></tr>';
        html += '</table>'

        return html;
    }

    function get_switches_info() {
        var html = '';

        html += '<table width=700>';
            for (var i = 0; i < Info.switches_stats.length; ++i) {
                html += '<tr height="30"><td align=left> </td></tr>';
                html += '<tr height="400"><td align=left>';
                html += '<div id="sw_chart' + Info.switches_stats[i].dpid + '" class="sw_chart"></div>';
                html += '</td></tr>';
            }
        html += '</table>'

        return html;
    }

    function updateRXCtrlStats (new_rx, new_pkt_in, old_rx, old_pkt_in, iter) {
        var upd_count  = 10;
        var chart = document.body.querySelector('.rx_chart');

        if (Info.rx_data.length == 0) {
            Info.rx_data.push([0, new_rx]);
            Info.pkt_in_data.push([0, new_pkt_in]);
            printChartRX(chart, Info.rx_data, Info.pkt_in_data);
            return;
        }

        if (iter == upd_count) return;

        // for rx_data
        var xval  = Info.rx_data.length,
            yval_rx  = old_rx + (iter * (new_rx - old_rx)/(upd_count-1));

        if (xval > 10 * upd_count) {
            Info.rx_data.splice(0,1);
            for (var i = 0, l = Info.rx_data.length; i < l; i++) {
                Info.rx_data[i][0]--;
            }
        }
        Info.rx_data.push([xval, yval_rx]);

        // for pkt_in_data
        var pkt_in_xval  = Info.pkt_in_data.length,
            pkt_in_yval  = old_pkt_in + (iter * (new_pkt_in - old_pkt_in)/(upd_count-1));

        if (pkt_in_xval > 10 * upd_count) {
            Info.pkt_in_data.splice(0,1);
            for (var i = 0, l = Info.pkt_in_data.length; i < l; i++) {
                Info.pkt_in_data[i][0]--;
            }
        }
        Info.pkt_in_data.push([pkt_in_xval, pkt_in_yval]);

        printChartRX(chart, Info.rx_data, Info.pkt_in_data);

        setTimeout(function() {
            updateRXCtrlStats(new_rx, new_pkt_in, old_rx, old_pkt_in, ++iter);
        }, 1000/(upd_count));
    }

    function updateTXCtrlStats (new_tx, old_tx, iter) {
     var upd_count  = 10;
        var tx_chart = document.body.querySelector('.tx_chart');

        if (Info.tx_data.length == 0) {
            Info.tx_data.push([0, new_tx]);
            printChartTX(tx_chart, Info.tx_data);
            return;
        }

        if (iter == upd_count) return;

        var xval  = Info.tx_data.length,
            yval_tx  = old_tx + (iter * (new_tx - old_tx)/(upd_count-1));

        if (xval > 10 * upd_count) {
            Info.tx_data.splice(0,1);
            for (var i = 0, l = Info.tx_data.length; i < l; i++) {
                Info.tx_data[i][0]--;
            }
        }
        Info.tx_data.push([xval, yval_tx]);
        printChartTX(tx_chart, Info.tx_data);

        setTimeout(function() {
            updateTXCtrlStats(new_tx, old_tx, ++iter);
        }, 1000/(upd_count));
    }

    // SWITCHES IN PROGRESS
    function updateSwitchStats (dpid, new_rx, new_pkt_in, new_tx, old_rx, old_pkt_in, old_tx, iter) {

        var upd_count  = 10;
        var sw_chart = document.querySelector('#sw_chart' + dpid);

        for (var i = 0; i < Info.switches_data.length; ++i) {

            if (Info.switches_data[i].dpid == dpid) {

                    if (Info.switches_data[i].rx_freq_data.length == 0) {

                        Info.switches_data[i].rx_freq_data.push([0, new_rx]);
                        Info.switches_data[i].pkt_in_freq_data.push([0, new_pkt_in]);
                        Info.switches_data[i].tx_freq_data.push([0, new_tx]);

                        printSwitchChart(sw_chart,
                                         Info.switches_data[i].rx_freq_data,
                                         Info.switches_data[i].pkt_in_freq_data,
                                         Info.switches_data[i].tx_freq_data,
                                         dpid);
                        return;
                    }

                    if (iter == upd_count) return;

                    // calculate (x,y) for rx
                    var xval_rx  = Info.switches_data[i].rx_freq_data.length,
                        yval_rx  = old_rx + (iter * (new_rx - old_rx)/(upd_count-1));

                    if (xval_rx > 10 * upd_count) {
                        Info.switches_data[i].rx_freq_data.splice(0,1);
                        for (var j = 0, l = Info.switches_data[i].rx_freq_data.length; j < l; j++) {
                            Info.switches_data[i].rx_freq_data[j][0]--;
                        }
                    }
                    Info.switches_data[i].rx_freq_data.push([xval_rx, yval_rx]);

                    // calculate (x,y) for pkt_in
                    var xval_pkt_in  = Info.switches_data[i].pkt_in_freq_data.length,
                        yval_pkt_in = old_pkt_in + (iter * (new_pkt_in - old_pkt_in)/(upd_count-1));

                    if (xval_pkt_in > 10 * upd_count) {
                        Info.switches_data[i].pkt_in_freq_data.splice(0,1);
                        for (var j = 0, l = Info.switches_data[i].pkt_in_freq_data.length; j < l; j++) {
                            Info.switches_data[i].pkt_in_freq_data[j][0]--;
                        }
                    }
                    Info.switches_data[i].pkt_in_freq_data.push([xval_pkt_in, yval_pkt_in]);

                    // calculate (x,y) for tx
                    var xval_tx  = Info.switches_data[i].tx_freq_data.length,
                        yval_tx  = old_tx + (iter * (new_tx - old_tx)/(upd_count-1));

                    if (xval_tx > 10 * upd_count) {
                        Info.switches_data[i].tx_freq_data.splice(0,1);
                        for (var j = 0, l = Info.switches_data[i].tx_freq_data.length; j < l; j++) {
                            Info.switches_data[i].tx_freq_data[j][0]--;
                        }
                    }
                    Info.switches_data[i].tx_freq_data.push([xval_tx, yval_tx]);

                    // print chart
                    printSwitchChart(sw_chart,
                                     Info.switches_data[i].rx_freq_data,
                                     Info.switches_data[i].pkt_in_freq_data,
                                     Info.switches_data[i].tx_freq_data,
                                     dpid);
                break;
            }
        }

        setTimeout(function() {
            updateSwitchStats( dpid, new_rx, new_pkt_in, new_tx, old_rx, old_pkt_in, old_tx, ++iter);
        }, 1000/(upd_count));
    }

    function printChartRX (chart, ctrl_rx_data, ctrl_pkt_in_data) {
        var upd_count  = 10;

        Flotr.draw(chart, [ {data: ctrl_rx_data, label: 'RX'}, {data: ctrl_pkt_in_data, label: 'PKT_IN'}], {
            HtmlText : false,
            title : "RUNOS RX/Packet-In OpenFlow Messages Frequency",
            colors : ['#00A8F0', '#00FF00'],
            xaxis : {
                title: "Time (10 sec)",
                showLabels: true,
                min : 0,
                max : 10 * upd_count + 1,
            },
            yaxis : {
                title: "Load (pps)",
                max : Info.rx_ymax,
                min : 0,
            },
            mouse : {
              track : true,
              sensibility: 10
            }
        });
    }

    function printChartTX (chart, ctrl_tx_data) {
        var upd_count  = 10;
        Flotr.draw(chart, [ {data: ctrl_tx_data, label: 'TX'}], {
            HtmlText : false,
            title : "RUNOS TX OpenFlow Messages Frequency",
            colors : ['#DD1772'],
            xaxis : {
                title: "Time (10 sec)",
                showLabels: true,
                min : 0,
                max : 10 * upd_count + 1,
            },
            yaxis : {
                title: "Load (pps)",
                max : Info.tx_ymax,
                min : 0,
            },
            mouse : {
              track : true,
              sensibility: 10
            }
        });
    }

    // CHART for SWITCHES
    function printSwitchChart (sw_chart, sw_rx_freq_data, sw_pkt_in_freq_data, sw_tx_freq_data, dpid) {
        var upd_count  = 10;

        Flotr.draw(sw_chart, [ {data: sw_rx_freq_data, label: 'RX'},
                            {data: sw_pkt_in_freq_data, label: 'PKT_IN'},
                            {data: sw_tx_freq_data, label: 'TX'}], {
            HtmlText : false,
            title : "RX/Packet-In/TX OpenFlow Messages Frequency for Switch DPID = " + dpid,
            colors : ['#00A8F0', '#00FF00', '#DD1772'],
            xaxis : {
                title: "Time (10 sec)",
                showLabels: true,

                min : 0,
                max : 10 * upd_count + 1,
            },
            yaxis : {
                title: "Load (pps)",
                max : Info.sw_ymax,
                min : 0,
            },
            mouse : {
              track : true,
              sensibility: 10
            }
        });
    }
}();
