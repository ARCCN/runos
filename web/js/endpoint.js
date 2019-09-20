/*

Copyright 2019 Applied Research Center for Computer Networks

*/

StaticEndpoint = (function() {
    return {
        showMenuStatistics : showMenuStatistics
    };

    function showMenuStatistics(name, port) {

        var html = getStatisticsHtml();
        var menu = UI.createMenu(html, {name: "endpoint_stats_" + name, pos: "center", width: "80%"});
        var endpoint = {
            int : {},
            max : {},
            cur : {},
            hasData : false,
            dataIn : [],
            dataOut : [],
            pchart : menu.element('chart')
        };

        getStat();

        function getStatisticsHtml() {
            var html = '';
            html += '<div id="chart" class="chart" style="display: block"></div>';

            html += '<div class="info" ' +
                        'style="display: flex; ' +
                            'justify-content: space-around; ' +
                            'position: relative; ' +
                            'width: 50%"' +
                            'float: right' +
                '>';
            html += '<div style="width:44%; display: block; position: relative;">';
            html += '<table class="arccn" style="width:100%">';
            html += '<thead><tr>';
            html += '<th colspan="2">Info</th>';
            html += '</tr></thead>';
            html += '<tbody>';
            if (port) {
                html += '<tr><td>Switch DPID</td><td>' + port.router.id + '</td></tr>';
                html += '<tr><td>Port ID</td><td>' + port.of_port + '</td></tr>';
                html += '<tr><td>Port name</td><td>' + port.name + '</td></tr>';
            }
            html += '<tr><td>Endpoint name</td><td>'+name+'</td></tr>';
            html += '</tbody>';
            html += '</table>';

            html += '</div>';

            html += '<div style="width:44%; display: block; position: relative;">';
            html += '<table class="arccn" style="width: 100%">';
            html += '<thead><tr>';
            html += '<th colspan="2">Summary</th>';
            html += '</tr></thead>';
            html += '<tbody>';
            html += '<tr><td>Packets IN</td><td id="p_sum_in">xxx</td></tr>';
            html += '<tr><td>Bytes IN</td><td id="b_sum_in">xxx</td></tr>';
            html += '<tr><td>Flows IN</td><td id="f_sum_in">xxx</td></tr>';
            html += '<tr><td>Packets OUT</td><td id="p_sum_out">xxx</td></tr>';
            html += '<tr><td>Bytes OUT</td><td id="b_sum_out">xxx</td></tr>';
            html += '<tr><td>Flows OUT</td><td id="f_sum_out">xxx</td></tr>';
            html += '</tbody>';
            html += '</table>';
            html += '</div>';
            html += '</div>';

            return html;
        }

        function getStat() {
            if (!menu.parentElement || !port || !port.router || port.status == 'down' || !Net.getPort(port.router, port.of_port))
                return;

            Server.ajax("GET", "/bucket/"+name+"-in/", callBackFabric('in'));
            Server.ajax("GET", "/bucket/"+name+"-out/", callBackFabric('out'));

            setTimeout(function() {
                 getStat();
            }, 1000);
            endpoint.hasData && drawChart();
        }

        function callBackFabric(type) {
            return function(response) {
                callbackStats(response, type);
            }
        }

        function callbackStats(response, type) {
            var int = response["integral"],
                cur = response["current-speed"],
                max = response["max"];

            if (int) {
                endpoint.int["packets_" + type] = Number(int["packets"]);
                endpoint.int["flows_" + type] = Number(int["flows"]);
                endpoint.int["bytes_" + type]   = Number(int["bytes"]);
                endpoint.int["Kb_" + type]   = 8*Number(int["bytes"])/1000;
            }
            if (cur) {
                endpoint.cur["packets_" + type] = Number(cur["packets"]);
                endpoint.cur["flows_" + type] = Number(cur["flows"]);
                endpoint.cur["bytes_" + type]   = Number(cur["bytes"]);
                endpoint.cur["Kb_" + type]   = 8*Number(cur["bytes"])/1000;

                endpoint.hasData = true;
            }
            if (max) {
                endpoint.max["packets_" + type] = Number(cur["packets"]);
                endpoint.max["flows_" + type] = Number(cur["flows"]);
                endpoint.max["bytes_" + type]   = Number(cur["bytes"]);
                endpoint.max["Kb_" + type]   = 8*Number(cur["bytes"])/1000;
            }

            if (menu) {
                menu.element('p_sum_' + type).innerHTML = endpoint.int["packets_" + type];
                menu.element('b_sum_' + type).innerHTML = utils.parseNetworkValue(endpoint.int["Kb_" + type] * 1000, true);
                menu.element('f_sum_' + type).innerHTML = endpoint.int["flows_" + type];
                /* is current stats needed? todo
                menu.element('p_cur_' + type).innerHTML = endpoint.cur["packets_" + type];
                menu.element('f_cur_' + type).innerHTML = endpoint.cur["flows_" + type];
                menu.element('b_cur_' + type).innerHTML = utils.parseNetworkValue(endpoint.cur["Kb_" + type] * 1000, true);
                //*/
            }
        }

        function drawChart() {
            var kbMax = Math.max(endpoint.max["Kb_in"], endpoint.max["Kb_out"]);
            if (!endpoint.ymax || endpoint.ymax == 0 || endpoint.ymax < 1.05 * kbMax) {
                endpoint.ymin = -kbMax / 20;
                endpoint.ymax = 1.05 * kbMax;
            }
            if (endpoint.dataIn.length > 0) {
                updateChartData(endpoint,
                    endpoint.cur["Kb_in"],
                    endpoint.cur["Kb_out"],
                    endpoint.dataIn[endpoint.dataIn.length - 1][1],
                    endpoint.dataOut[endpoint.dataOut.length - 1][1], 1);
            } else {
                updateChartData(endpoint, endpoint.cur["Kb_in"], endpoint.cur["Kb_out"], 0, 0, 1);
            }
        }

        function updateChartData(endpoint, newInVal, newOutVal, oldInVal, oldOutVal, iter) {

            if (!menu.parentElement) return;

            var upd_count  = 10;
            if (endpoint.dataIn.length == 0) {
                endpoint.dataIn.push([0, endpoint.cur["Kb_in"]]);
                endpoint.dataOut.push([0, endpoint.cur["Kb_out"]]);
                printChart(endpoint);
                return;
            }

            if (iter == upd_count) return;

            var xval  = endpoint.dataIn.length,
                yvalIn  = oldInVal + (iter * (newInVal - oldInVal)/(upd_count-1)),
                yvalOut  = oldOutVal + (iter * (newOutVal - oldOutVal)/(upd_count-1));

            if (xval > 10 * upd_count) {
                endpoint.dataIn.splice(0,1);
                endpoint.dataOut.splice(0,1);
                for (var i = 0, l = endpoint.dataIn.length; i < l; i++) {
                    endpoint.dataIn[i][0]--;
                    endpoint.dataOut[i][0]--;
                }
            }
            endpoint.dataIn.push([xval, yvalIn]);
            endpoint.dataOut.push([xval, yvalOut]);
            printChart(endpoint);
            setTimeout(function() {
                updateChartData(endpoint, newInVal, newOutVal, oldInVal, oldOutVal, ++iter);
            }, 1000/(upd_count));
        }

        function printChart(endpoint) {
            var upd_count  = 10;
            Flotr.draw(endpoint.pchart, [
                {data: endpoint.dataIn, label: 'Kb_in'},
                {data: endpoint.dataOut, label: 'Kb_out'}], {
                HtmlText : false,
                title : "Endpoint Statistics",
                colors : ['#00A8F0', '#DD1772'],
                xaxis : {
                    title: "time (10 sec)",
                    showLabels: false,
                    min : 0,
                    max : 10 * upd_count + 1,
                },
                yaxis : {
                    title: "load (Kb/s)",
                    max : endpoint.ymax || 1,
                    min : endpoint.ymin
                },
                mouse : {
                    track : true,
                    sensibility: 10
                }
            });
        }
    }



}());
