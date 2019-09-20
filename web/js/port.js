/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';

/* globals newPort:true */
newPort = function () {
    var stats_duration = 10; // sec
    var stats_fps      = 20; // updates per sec

    return function (ex) {
        ex = ex || {};
        var port = {
            type       : 'port',
            id         : ex.id          || 'DEFAULT',
            name       : ex.name        || 'none',
            hw_addr    : ex.hw_addr     || false,
            router     : ex.router      || false,
            of_port    : ex.of_port     || 0,
            status     : false,
            link       : null,
            end_point  : [],
            storm      : false,

            storm_is_active : false,
            hint_storm_is_active : "Storm has started on "+ex.router+":"+ex.of_port+"",
            hint_storm_is_not_active : "Storm has ended on "+ex.router+":"+ex.of_port+"",

            port_down  : false,
            no_recv    : false,
            no_fwd     : false,
            no_pkt_in  : false,
            curr_speed : false,
            bitrate    : false,
            maintenance: false,

            q_disc     : false,
            new_q_disc : false,
            new_queues : [],
            export_queues : [],
            queues2    : [],
            queues     : [],
            queue_stat : [],
            getQueuesStat : getQueuesStat,

            //stats
            updateStats: updateStats,
            clearView  : clearView,
            showQ      : false,
            uptime     : 0,
            int        : {},
            cur        : {},
            max        : {},
            intDelta   : {
                tx_packets : 0,
                rx_packets : 0,
                tx_bytes : 0,
                rx_bytes : 0,
                tx_dropped : 0,
                rx_dropped : 0
            },

            qdata      : [],
            qmax       : 0,

            //drawing
            x          : 0,
            y          : 0,
            s          : 0,
            position   : 0,
            direction  : 'none',  /// 'up', 'down', 'left', 'right'

            isVisible  : true,
            isSelected : false,
            isDomainOn : false,
            isLAG      : false,
            lagPort    : false,
            draw       : draw,
            isOver     : isOver,
            computeXY  : computeXY,
            computeDir : computeDir,
            computeShift : computeShift,
            changeDir  : changeDir,
            other      : other,

            getPortInfo      : getPortInfo,
            getPortQueues    : getPortQueues,
            applyQueues      : applyQueues,

            setDeltaStats    : setDeltaStats,
            dropDeltaStats   : dropDeltaStats,

            setMaintenance   : setMaintenance,

            stormStatus : stormStatus,

            rx         : 0,
            ry         : 0,

            pstats_window : false,
            pchart        : false,

            qstats_window : false,
            qchart        : false,

            portStats : {
                rx_data    : [],
                tx_data    : [],
                ymin       : 0,
                ymax       : 0,
                prev_rx : false,
                prev_tx : false,
                writing_session : 0,
            }
        };

        Logger.info('topology', 'Port ' + port.of_port + ' added on switch ' + port.router);

        if (typeof port.router === 'string') {
            port.router = Net.getHostByID(port.router);
        }

        Net.domains.forEach(function(domain) {
            domain.ports.forEach(function(p){
                if (port.router.id == p.router.id && p.of_port == port.of_port) {
                    port.isDomainOn = true;
                    port.end_point = p.end_point;
                }
            });
        });

        port.router.ports.push(port);
        port.link = Net.getLinkByPort(port.router.id, port.of_port);
        //setInterval(function(){port.computeShift()}, 2000);
        port.getPortInfo();
        //setInterval(function(){port.getPortInfo()},  2000);

        //Links to SOC
        if (!port.link && port.router.mode == 'DR' && Net.soc) {
            newLink({
                host1:   port.router.id,
                host1_p: port.of_port,
                host2:   Net.soc.id,
                host2_p: 'none'
            });
        }

        if (port.link) {
            port.link.to_host = (port.other(port.link) == null ? true : false);
        }

        if (port.router.mode == 'AR')
            port.changeDir('down');
        else if (port.router.mode == 'DR')
            port.changeDir('up');
        else // if mode == 'NONE'
            port.changeDir('down');

        if (Net.settings.show_ports)
            port.computeDir();

        return port;
    };

    function getPortQueues () {
        var port = this;
        Server.ajax("GET", '/switches/' + this.router.id + '/port/' + this.of_port + '/queues', callbackQueues);

        function callbackQueues (response) {
            var disc = response["type"],
                arr = response["queues"],
                size = Number(response["_size"]);

            port.queues2.length = 0;
            //port.queues.length = 0;
            port.q_disc = disc;
            if (size > 0)
                arr.forEach(function(q) {
                    port.queues2.push(q);
                    //port.queues.push(q["id"]);
                });
        }
    }

    function applyQueues () {
        if (this.export_queues.length > 0 && this.new_queues.length > 0) {
            window.console.error("export_queues && new_queues not empty");
            return;
        }

        if (this.export_queues.length == 0 && this.new_queues.length == 0) {
            window.console.error("nothing to apply");
            return;
        }

        var ret = {};
        ret["type"] = this.new_q_disc;
        ret["queues"] = [];

        if (this.export_queues.length > 0) {
            ret["queues"] = this.export_queues;
        }
        if (this.new_queues.length > 0) {
            ret["queues"] = this.queues2;
            this.new_queues.forEach(function(q) {
                ret["queues"].push(q);
            });
        }

        var port = this;
        Server.ajax('POST', '/switches/' + this.router.id + '/port/' + this.of_port + '/queues', ret, function (response) {
            port.new_queues.length = 0;
            port.export_queues.length = 0;
            port.new_q_disc = false;
            Net.import_queue = {};
        });
    }

    function getPortInfo () {
        var port = this;
        Server.ajax("GET", '/switches/' + this.router.id + '/ports/' + this.of_port + '/', callbackPort);
        Server.ajax("GET", '/switches/' + this.router.id + '/ports/' + this.of_port + '/maintenance/', callbackMaintenance);

        function callbackPort (response) {
            var config      = response["config"],
                eth         = response["ethernet"],
                features    = eth["features"];

            port.name       = response["name"];
            port.hw_addr    = response["hw-addr"];
            port.port_down  = config["port-down"];
            port.no_recv    = config["no-recv"];
            port.no_fwd     = config["no-fwd"];
            port.no_pkt_in  = config["no-packet-in"];
            port.queues     = response["queues"];
            port.curr_speed = Number(eth["curr-speed"]);

            for (var sp in features) {
                if (features[sp].indexOf("current") > -1) {
                    if (sp == "custom-rate")
                        port.curr_speed = Number(eth["curr-speed"]);
                    else {
                        port.curr_speed = PortSpeed[sp];
                        port.bitrate = sp;
                    }
                    break;
                }
            }

            if (port.link) {
                port.link.bandwidth = Math.max(port.curr_speed, port.link.bandwidth); // TODO: what to do if ports have different curr_speed
            }
        }

        function callbackMaintenance (response) {
            if (response["maintenance"] == "true")
                port.maintenance = true;
        }
    }

    function getQueuesStat() {
        this.queue_stat.length = 0;
        this.qdata.length = 0;
        for (var i = 0, len = this.queues.length; i < len; i++) {
                this.queue_stat.push({});
            }
        for (var i = 0, len = this.queues.length; i < len; i++) {
            QStat(this, this.queues[i]);
        }
    }

    function QStat(port, queue_id) {
        if (!port.router)
            return;
        if (!port.showQ)
            return;
        Server.ajax("GET", '/switches/'+ port.router.id + '/ports/' + port.of_port + '/queues/' + queue_id + '/stats/', callbackQueues);
        setTimeout(function() {
            QStat(port, queue_id);
        }, 1000);

        function callbackQueues(response) {
            var int = response["integral"],
                cur = response["current-speed"],
                max = response["max"];

            if (!int || !cur || !max)
                return;

            var int_Kb = Math.floor(8*Number(int["tx-bytes"])/1000),
                cur_Kb = Math.floor(8*Number(cur["tx-bytes"])/1000),
                max_Kb = Math.floor(8*Number(max["tx-bytes"])/1000),
                prev_Kb = port.queue_stat[queue_id]["cur_tx_Kb"],
                int_Drops = Math.floor(int['tx-errors']);

            port.queue_stat[queue_id]["cur_tx_Kb"] = cur_Kb;
            port.queue_stat[queue_id]["cur_tx_packets"] = Math.floor(Number(cur["tx-packets"]));
            port.qmax = Math.max(cur_Kb, port.qmax);

            if (port.showQ && port.qstats_window) {
                port.qstats_window.element('q'+queue_id+'_bytes').innerHTML = int_Kb;
                port.qstats_window.element('q'+queue_id+'_drops').innerHTML = int_Drops;
                updateQChartData(port, queue_id, cur_Kb, prev_Kb, 0);
            }
        }
    }

    function updateQChartData(port, queue_id, new_val, old_val, iter) {
        var upd_count  = 10;

        if (old_val == undefined)
            old_val = 0;

        if (iter == upd_count) return;

        var val = old_val + iter*(new_val-old_val)/upd_count;
        var dat = [Number(queue_id), val];
        port.qdata[queue_id] = [];
        port.qdata[queue_id].push(dat);

        printQChart(port);
        setTimeout(function() {
            updateQChartData(port, queue_id, new_val, old_val, ++iter);
        }, 1000/(upd_count));
    }

    function printQChart(port) {
        var ticks = [];
        for (var i = 0, l = port.qdata.length; i < l; i++)
            ticks.push(i);
        Flotr.draw(port.qchart,  port.qdata, {
            HtmlText : false,
            title : "Queues Statistics",
            bars : {
                show : true,
                horizontal : false,
                shadowSize : 0,
                barWidth : 1
            },
            mouse : {
                track : true,
                relative : true
            },
            xaxis : {
                title: "queues",
                ticks : ticks,
                min : -1,
                max : port.qdata.length,
            },
            yaxis : {
                title: "load (Kb/s)",
                min : 0,
                max : port.qmax,
                autoscaleMargin : 1
            }
        });
    }

    function updateStats (stats) {
        var int = stats["integral"],
            cur = stats["current-speed"],
            max = stats["max"];
        if (!int || !cur || !max) return;

        this.int["rx_packets"] = Number(int["rx-packets"]);
        this.int["tx_packets"] = Number(int["tx-packets"]);
        this.int["rx_bytes"]   = Number(int["rx-bytes"]);
        this.int["tx_bytes"]   = Number(int["tx-bytes"]);
        this.int["rx_dropped"]   = Number(int["rx-dropped"]);
        this.int["tx_dropped"]   = Number(int["tx-dropped"]);
        this.int["rx_errors"]   = Number(int["rx-errors"]);
        this.int["tx_errors"]   = Number(int["tx-errors"]);
        this.int["rx_Kb"]   = 8*Number(int["rx-bytes"])/1000;
        this.int["tx_Kb"]   = 8*Number(int["tx-bytes"])/1000;

        this.cur["rx_packets"] = Number(cur["rx-packets"]);
        this.cur["tx_packets"] = Number(cur["tx-packets"]);
        this.cur["rx_bytes"]   = Number(cur["rx-bytes"]);
        this.cur["tx_bytes"]   = Number(cur["tx-bytes"]);
        this.cur["rx_Kb"]   = (8*Number(cur["rx-bytes"])/1000).toFixed(2);
        this.cur["tx_Kb"]   = (8*Number(cur["tx-bytes"])/1000).toFixed(2);

        this.max["rx_packets"] = Number(max["rx-packets"]);
        this.max["tx_packets"] = Number(max["tx-packets"]);
        this.max["rx_bytes"]   = Number(max["rx-bytes"]);
        this.max["tx_bytes"]   = Number(max["tx-bytes"]);
        this.max["rx_Kb"]   = 8*Number(max["rx-bytes"])/1000;
        this.max["tx_Kb"]   = 8*Number(max["tx-bytes"])/1000;

        if (this.link) {
            if (!isNaN(this.cur["rx_Kb"]) && !isNaN(this.cur["tx_Kb"])) {
                this.link.load = Math.max(this.cur["rx_Kb"], this.cur["tx_Kb"]);
            }
        }

        if (this.pstats_window) {
            this.pstats_window.element('txp_sum').innerHTML = this.int["tx_packets"] - this.intDelta['tx_packets'];
            this.pstats_window.element('rxp_sum').innerHTML = this.int["rx_packets"] - this.intDelta['rx_packets'];
            this.pstats_window.element('txb_sum').innerHTML = this.int["tx_bytes"] - this.intDelta['tx_bytes'];
            this.pstats_window.element('rxb_sum').innerHTML = this.int["rx_bytes"] - this.intDelta['rx_bytes'];
            this.pstats_window.element('txd_sum').innerHTML = utils.checkMaxInt(this.int["tx_dropped"]) ? 'NaN' : this.int["tx_dropped"] - this.intDelta['tx_dropped'];
            this.pstats_window.element('rxd_sum').innerHTML = utils.checkMaxInt(this.int["rx_dropped"]) ? 'NaN' : this.int["rx_dropped"] - this.intDelta['rx_dropped'];
            this.pstats_window.element('txl_cur').innerHTML = utils.parseNetworkValue(this.cur["tx_Kb"] * 1000, true);
            this.pstats_window.element('rxl_cur').innerHTML = utils.parseNetworkValue(this.cur["rx_Kb"] * 1000, true);

            var max = Math.max(this.max["rx_Kb"], this.max["tx_Kb"]);
            if (this.portStats.ymax == 0 || this.portStats.ymax < 1.05 * max) {
                this.portStats.ymin = -Math.min(max, this.curr_speed) / 20;
                this.portStats.ymax = 1.05 * Math.min(max, this.curr_speed);
            }

            if (this.portStats.prev_rx !== false && this.portStats.prev_tx !== false) {
                updateChartData(this, this.cur["rx_Kb"], this.cur["tx_Kb"], this.portStats.prev_rx, this.portStats.prev_tx, 1, ++this.portStats.writing_session);
            }
            this.portStats.prev_rx = this.cur["rx_Kb"];
            this.portStats.prev_tx = this.cur["tx_Kb"];
        }
    }

    function updateChartData(port, new_rx, new_tx, old_rx, old_tx, iter, session) {
        if (iter == stats_fps || port.portStats.writing_session > session) {
            return;
        }

        var now = (new Date()).getTime();
        var prev_rx = [now, old_rx], // values for first iteration
            prev_tx = [now, old_tx];

        if (iter > 1) {
            prev_rx = port.portStats.rx_data[port.portStats.rx_data.length-1];
            prev_tx = port.portStats.tx_data[port.portStats.tx_data.length-1];
        }

        var curr_rx = Math.round((prev_rx[1] + (now - prev_rx[0]) * (new_rx - old_rx) / 1000) * 100) / 100;
        var curr_tx = Math.round((prev_tx[1] + (now - prev_tx[0]) * (new_tx - old_tx) / 1000) * 100) / 100;

        if (port.portStats.rx_data.length >= stats_fps * stats_duration) {
            port.portStats.rx_data.shift();
            port.portStats.tx_data.shift();
        }

        port.portStats.rx_data.push([now, curr_rx]);
        port.portStats.tx_data.push([now, curr_tx]);

        printChart(port);
        setTimeout(function() {
            updateChartData(port, new_rx, new_tx, old_rx, old_tx, ++iter, session);
        }, 1000/(stats_fps));
    }

    function printChart(port) {
        Flotr.draw(port.pchart, [ {data: port.portStats.rx_data, label: 'RX'},
                               {data: port.portStats.tx_data, label: 'TX'} ], {
            HtmlText : false,
            title : "Port Statistics",
            colors : ['#00A8F0', '#DD1772'],
            xaxis : {
                title: "time (" + stats_duration + " sec)",
                showLabels: false,
                min : port.portStats.rx_data[0][0],
                max : port.portStats.rx_data[0][0] + 1000 * (stats_duration + 1.5),
            },
            yaxis : {
                title: "load (Kb/s)",
                max : port.portStats.ymax,
                min : port.portStats.ymin,
            },
            mouse : {
              track : true,
              sensibility: 10
            }
        });
    }

    function changeDir(newpos) {
        var port = this;
        if (this.direction == newpos)
            return;
        if (newpos != 'down' && newpos != 'up'
            && newpos != 'left' && newpos != 'right')
            return;

        this.router.ports.forEach(function(p) {
            if (port.direction == p.direction && port.position < p.position) {
                p.position--;
            }
        });

        port.router.ports_pos[port.direction]--;
        port.direction = newpos;
        port.position = port.router.ports_pos[newpos]++;

        port.computeShift();
        this.router.ports.forEach(function(p) {
            if (port.direction == p.direction) {
                p.computeXY();
            }
        });
    }

    function computeShift() {
        var cur_dir = this.direction;
        var new_pos = this.position;
        var max = (cur_dir == 'down' ? this.router.ports_pos.down :
                             (cur_dir == 'up' ? this.router.ports_pos.up :
                             (cur_dir == 'left' ? this.router.ports_pos.left :
                              this.router.ports_pos.right)));
        for (var i = 0, l = this.router.ports.length; i < l; i++) {
            var p = this.router.ports[i];
            if (p.direction != this.direction) continue;
            if (p == this) continue;
            if (!this.link || !p.link) continue;

            var c_other = this.link.other(this.router),
                p_other = p.link.other(this.router);

            if (!c_other || !p_other) continue;
            if (c_other == p_other) continue;

            var x1 = this.router.x, y1 = this.router.y,
                x2 = c_other.x,     y2 = c_other.y,
                x3 = p_other.x,     y3 = p_other.y;

            if ((cur_dir == "up" || cur_dir == "down") && (x2-x1)/(y2-y1) >= (x3-x1)/(y3-y1)) {
                if (new_pos > p.position) {
                    new_pos = p.position;
                }
            }
            else if ((cur_dir == "left" || cur_dir == "right") && (y2-y1)/(x2-x1) <= (y3-y1)/(x3-x1)) {
                if (new_pos > p.position) {
                    new_pos = p.position;
                }
            }
        }

        var this_p = this;
        if (new_pos != this.position) {
            this.router.ports.forEach(function(p, i) {
                if (cur_dir == p.direction && new_pos <= p.position && new_pos != this_p.position && this_p != p) {
                    p.position = Math.min(max, ++p.position);
                }
            });
        }

        this.position = new_pos;
    }

    function computeDir() {
        if (this.link) {
            var r2 = this.link.other(this.router),
                nx = r2.x - this.router.x,
                ny = r2.y - this.router.y,
                sec = this.other(this.link);

            if      (nx >= ny && nx <= -ny) { this.changeDir('up');    if (sec) sec.changeDir('down'); }
            else if (nx >= ny && nx >= -ny) { this.changeDir('right'); if (sec) sec.changeDir('left'); }
            else if (nx <= ny && nx >= -ny) { this.changeDir('down');  if (sec) sec.changeDir('up'); }
            else if (nx <= ny && nx <= -ny) { this.changeDir('left'); if (sec) sec.changeDir('right'); }
            else { window.console.log("WTF?!"); }

            r2.ports.forEach(function(p, i) {
                p.computeXY();
            });
        }

        this.computeXY();
    }

    function computeXY () {
        if (!this.router) {
            window.console.error('No router!');
            return;
        }

        var r = this.router;
        this.s = Net.settings.scale / 6;
        this.rx = r.x;
        this.ry = r.y;

        var surf_num,
            in_surf_shift,
            s = this.s,
            x, y;

        if (this.direction == 'up') {
            surf_num = 0;
            in_surf_shift = 5 + Math.round((this.position + 1) * r.nx / (r.ports_pos.up + 1));
        } else if (this.direction == 'right') {
            surf_num = 1;
            in_surf_shift = Math.round((this.position + 1) * r.ny / (r.ports_pos.right + 1));
        } else if (this.direction == 'down') {
            surf_num = 2;
            in_surf_shift = 5 + Math.round((this.position + 1) * r.nx / (r.ports_pos.down + 1));
        } else if (this.direction == 'left') {
            surf_num = 3;
            in_surf_shift = -5 + Math.round((this.position + 1) * r.ny / (r.ports_pos.left + 1));
        } else {
            //window.console.error("port has bad direction", this);
            return;
        }

        switch (surf_num) {
        case 0:
            x = in_surf_shift; y = 0; break;
        case 1:
            x = r.nx; y = in_surf_shift; break;
        case 2:
            x = r.nx - in_surf_shift; y = r.ny; break;
        case 3:
            x = 0; y = r.ny - in_surf_shift; break;
        default:
            window.console.error("port position computation error");
        }

        this.x = x + r.x - r.nx/2;
        this.y = y + r.y - r.ny/2;
    }

    function draw () {
        if (!this.router) {
            window.console.error('No router!', this);
            return;
        }

        if (this.router.x != this.rx || this.router.y != this.ry) {
            this.computeDir();
        }
        if (this.s != Net.settings.scale / 6) {
            this.computeXY();
        }

        CTX.save();
        if (this.router.status == 'down' || this.router.maintenance || this.maintenance)
            CTX.globalAlpha = 0.5;
        CTX.translate(this.x, this.y);
        CTX.strokeStyle = Color.dblue;
        if (this.isMirrorInPort && this.isSelected) {
            CTX.fillStyle = Color.yellow;
        } else if (this.isSelected || this.isDomainOn)
            CTX.fillStyle = Color.pink;
        else if (this.storm && Net.settings.show_storm)
            CTX.fillStyle = 'red';
        else {
            if (this.router.mode == 'AR')
                CTX.fillStyle = Color.blue;
            else if (this.router.mode == 'DR') {
                CTX.fillStyle = 'green';
                CTX.strokeStyle = '#366423';
            } else { // if mode == 'NONE'
                CTX.fillStyle = '#0B2F3A';
                CTX.strokeStyle = '#000000';
            }
        }
        CTX.lineWidth = 2;

        if (this.isLAG && Net.settings.show_lag) {
            CTX.beginPath();
            CTX.arc(0, 0, 2*this.s/3, 0, 2*Math.PI, true);
            CTX.fill();
            CTX.stroke();
        } else {
            CTX.fillRect(-this.s/2, -this.s/2, this.s, this.s);
            CTX.strokeRect(-this.s/2, -this.s/2, this.s, this.s);
        }

        if (this.maintenance) {
            CTX.strokeStyle = Color.red;
            CTX.moveTo(-this.s/2, -this.s/2);
            CTX.lineTo( this.s/2,  this.s/2);
            CTX.moveTo( this.s/2, -this.s/2);
            CTX.lineTo(-this.s/2,  this.s/2);
            CTX.stroke();
        }

        if (HCI.key.p) {
            var x, y, s=2;
            if (this.direction == 'up') {
                x = 0; y = -this.s-s;
            } else if (this.direction == 'down') {
                x = 0; y = this.s+s;
            } else if (this.direction == 'left') {
                x = -this.s-s; y = 0;
            } else {
                x = this.s+s; y = 0;
            }
            CTX.textAlign = "center";
            CTX.textBaseline = "middle";
            CTX.font = Math.round(1.5*this.s) + 'px PT Sans Narrow';
            CTX.fillStyle = Color.darkblue;
            CTX.lineWidth = 1;
            CTX.strokeStyle = 'white';
            CTX.strokeText(this.of_port, x, y);
            CTX.fillText(this.of_port, x, y);
        }

        CTX.restore();
    }

    function isOver (x, y) {
        if (Math.abs(this.x - x) < this.s/2 && Math.abs(this.y - y) < this.s/2) {
            return true;
        }

        return false;
    }

    function other (link) {
        if (!link)
            return null;
        if (link.type != 'link')
            return null;

        var p1 = Net.getPort(link.host1, link.host1_p),
            p2 = Net.getPort(link.host2, link.host2_p);

        if (this != p1 && this != p2)
            return null;

        var ret = (this == p1 ? p2 : p1);
        return ret;
    }



    function clearView(type) {
        if (type == "port") {
            this.portStats.prev_rx = false;
            this.portStats.prev_tx = false;
            this.portStats.rx_data.length = 0;
            this.portStats.tx_data.length = 0;
            this.portStats.ymin    = 0;
            this.portStats.ymax    = 0;

            this.pstats_window = false;
            this.pchart = false;
        }
        else if (type == "queue") {
            this.showQ = false;

            this.qstats_window = false;
            this.qchart = false;
        }

        if (!this.pstats_window && !this.qstats_window)
            this.isSelected = false;

        Net.need_draw = true;
    }

    function setDeltaStats() {
        this.intDelta = Object.assign({}, this.int);
    }

    function dropDeltaStats() {
        this.intDelta =  {
            tx_packets : 0,
            rx_packets : 0,
            tx_bytes : 0,
            rx_bytes : 0,
            tx_dropped : 0,
            rx_dropped : 0
        };
    }

    function setMaintenance (val) {
        this.maintenance = val;
        var path = '/switches/'+this.router.id+'/ports/'+this.of_port+'/maintenance/';
        Server.ajax('PUT', path, {"maintenance": val});
        Net.need_draw = true;
    }

    function stormStatus (_storm_is_active) {
        if (_storm_is_active != this.storm_is_active) {
            UI.createHint(_storm_is_active ? hint_storm_is_active : hint_storm_is_not_active, {fail: _storm_is_active});
            this.storm_is_active = _storm_is_active;
        }
    }

}();

