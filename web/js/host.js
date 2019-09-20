/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';

/* globals newHost:true */
newHost = function () {
    return function (_) {
        _ = _ || {};
        var host = {
            type: (_.icon === 'router3' || _.icon === 'atm' || _.icon === 'atm2' || _.icon === 'cisco3') ? 'router' : 'host',
            mode: _.mode || "UNKNOWN",   /// 'AR', 'DR', 'SOC', 'CISCO', 'AUX', 'UNKNOWN'
            id:   _.id   || 'DEFAULT',
            peer: _.peer || '',
            icon: _.icon || 'aimac',
            name: Server.getCookie("host_name_" + _.id) || _.name || 'host',
            x:    _.x,
            y:    _.y,
            sx: 0,
            sy: 0,
            flies: 0,
            static_pos: false,
            status: _.status || 'up',
            color: 'white',
            isSelected: false,
            links: [],
            ports: [],

            sw_dpid: _.sw_dpid,
            sw_port: _.sw_port,
            counter: 0,

            maintenance: false,
            setMaintenance : setMaintenance,

            computePos: computePos,
            flyAway: flyAway,

            requestPorts: requestPorts,
            getPort: getPort,

            down_count : 0,
            draw: draw,
            isOver: isOver,

            delLinks : delLinks,
            delPorts : delPorts,

            showChangeNameMenu : showChangeNameMenu,
            changeRole : changeRole,

            nx: _.nx || Net.settings.scale,
            ny: _.ny || Net.settings.scale,

            hint_up   : "Switch " + _.id + " connected",
            hint_down : "Switch " + _.id + " disconnected",
        };

        Net.add(host);

        if (host.type === 'router') {
            host.bridgeDomains = new Map();
            host.routingRules  = new Map();
            host.lag           = new Map();
            host.mirrors       = new Map();
            host.ports_pos     = {};
            host.ports_pos.up = 0;
            host.ports_pos.down = 0;
            host.ports_pos.left = 0;
            host.ports_pos.right = 0;
            host.requestPorts();
            host.selectionType = false; /// 'exact', 'include', 'exclude'
            host.show_inband = false;
            Server.ajax('GET', '/switches/role/'+host.id+'/', callbackCreateSwitch);
            Server.ajax('GET', '/switches/'+host.id+'/maintenance/', callbackMaintenance);
        }

        if (host.mode == 'CISCO') {
            if (!host.x || !host.y) {
                host.computePos();
            }
        }

        function callbackCreateSwitch(response){
            var fly = false;

            var sw = Net.getHostByID(response["dpid"]);
            if (!sw)
                return;

            sw.changeRole(response["role"]);

            if (!Net.soc && sw.mode == 'DR') {
                Net.soc = newHost({
                    id:     '123456',
                    name:   GlobalStore['123456_name'] || 'SOC',
                    mode:   'SOC',
                    x:      GlobalStore['pos_123456_x'] || Number(Server.getCookie("pos_123456_x")) || Math.round(UI.centerX),
                    y:      GlobalStore['pos_123456_y'] || Number(Server.getCookie("pos_123456_y")) || Net.settings.scale*2,
                    status: 'up',
                    icon:   'acloud512',
                    nx :    Net.settings.scale*3,
                    ny :    Net.settings.scale*3,
                });
            }

            if (!host.x || !host.y) {
                host.computePos();
            }
        }

        function callbackMaintenance (response) {
            if (response["maintenance"] == "true")
                host.maintenance = true;
        }

        function delay(f, ms) {
            return function() {
                var newArr = [].slice.call(arguments);
                setTimeout(function() {
                    f.apply(this, newArr);
                }, ms);
            };
        }

        function flyAway(h) {
            if ((h.x == h.sx && h.y == h.sy) || h.static_pos) {
                h.flies = 0;
                return;
            }

            var dx = Math.abs(h.x - h.sx),
                dy = Math.abs(h.y - h.sy),
                kx = 1, ky = 1;

            if (dx != 0 && dy != 0) {
                if (dx > dy) {
                    kx = (Math.round(dx/dy) < 10 ? Math.round(dx/dy) : 10);
                    ky = 1;
                }
                else {
                    ky = (Math.round(dy/dx) < 10 ? Math.round(dy/dx) : 10);
                    kx = 1;
                }
            }

            if (h.x != h.sx)
                h.x < h.sx ? h.x += kx : h.x -= kx;
            if (h.y != h.sy)
                h.y < h.sy ? h.y += ky : h.y -= ky;

            Net.need_draw = true;

            var fly = delay(flyAway, 10);
            if (h.flies > 1)
                h.flies--;
            else if(h.flies == 1)
                fly(h);
            else
                return;
        }

        function computePos() {
            var m = host.mode,
                num = 0,
                count = Net.counter(m),
                x_shift = Math.round(2 * UI.centerX / (count + 1)),
                y_shift = (m == "DR" ? 1 : (m == "AR" ? 2 : 3));

            host.x = Math.round(UI.centerX);
            host.y = Math.round(UI.centerY);

            for (var i = 0, len = Net.hosts.length; i < len; i++) {
                var cur = Net.hosts[i];
                if (cur.static_pos) continue;
                if (cur.mode == m) {
                    ++num;
                    cur.sx = x_shift * num,
                    cur.sy = Math.round(y_shift * UI.centerY / 2 + UI.centerY / 4);
                    cur.flies++;
                    flyAway(cur);
                }
            }
        }

        return host;
    };

    function requestPorts () {
        if (this.type != 'router')
            return;
        Server.ajax('GET', '/switches/' + this.id + '/', callbackSwitch);

        var sw = this;
        setTimeout(function () { // no ports
            if (sw.ports.length == 0 && sw.status != 'down')
                sw.requestPorts();
        }, 2000);

        function callbackSwitch(response) {
            var p = response["ports"],
                dpid = response["dpid"],
                sw = Net.getHostByID(dpid);
            sw.delPorts();

            var numRequests = 0, numResponse = 0;
            for (var i = 0, len = p.length; i < len; i++) {
                if (p[i] != 4294967294 && p[i] != 999) { // OFPP_LOCAL and OFPP_LOCAL for Lagopus

                    numRequests++;
                    Server.ajax("GET", '/switches/' + dpid + '/ports/' + p[i] + '/', function(response) {
                        numResponse++;
                        if (response["link"]["status"] != "down") {
                            if (!Net.getPort(dpid, response["number"])) {
                                newPort({router: dpid, of_port: response["number"]});
                            }
                        }
                        if (numResponse == numRequests) {
                            requestLag();
                            requestMirror();
                        }
                    });
                }
            }

            // request storm control
            Server.ajax('GET', '/storm-control/'+dpid+'/', function (response) {
                var s = response["array"],
                    size  = response["_size"];

                for (var i = 0; i < size; i++) {
                    var ss = s[i],
                        host = Net.getHostByID(ss["dpid"]);
                    if (host) {
                        var port = host.getPort(ss["port"]);
                        if (port)
                            port.storm = ss["level"];
                    }
                }
            });


            function requestLag() {
            // request lag
                Server.ajax('GET', '/switches/'+dpid+'/lag-ports/', function (response) {

                    var lags = response['lags'], size, dpid, error = response['error'];

                    if (error) {
                        UI.createHint(error.msg || error, {fail: true});
                        return;
                    }
                    if (!lags) {
                        return;
                    }

                    size = lags.length;
                    dpid = response['sw'];

                    for (var i = 0; i < size; i++) {
                        var lag = lags[i],
                            router = Net.getHostByID(dpid);
                        if (router && !router.lag[lag["id"]]) {
                            var l = new newLAG({id: lag["id"], router: router, ports: lag["ports"]});
                            UI.createHint(l.hint_up);
                        }
                    }
                });
            }
            function requestMirror() {
                Server.ajax('GET', '/mirrors/', function(response) {
                    var error = response['error'];
                    var mirrors = response['all_mirrors'];
                    if (error) {
                        UI.createHint(error, {fail: true});
                        return;
                    }
                    if (!mirrors) return;

                    for (var i = 0; i < mirrors.length; ++i) {
                        var mirror_obj = mirrors[i]['mirror'];
                        if (mirror_obj["from"] && mirror_obj["from"]["dpid"] != dpid)
                            continue;

                        var id = mirrors[i]['id'];
                        var to_ctrl = false, filename = false;
                        if (mirror_obj['to_controller'] == "true") {
                            to_ctrl = true;
                            filename = mirror_obj['controller_dump_file'];
                        }
                        var to_ctrl = (mirror_obj['to_controller'] == "true" ? true : false);

                        var router = Net.getHostByID(dpid);
                        if (router && !router.mirrors[id]) {
                            var mirror = newMirror({id: id,
                                            router   : router,
                                            from     : mirror_obj['from'],
                                            to       : mirror_obj['to'],
                                            to_ctrl  : to_ctrl,
                                            filename : filename,
                                            svlans   : mirror_obj['svlans'],
                                            cvlans   : mirror_obj['cvlans'],
                                            sgroups  : mirror_obj['sgroups'],
                                            cgroups  : mirror_obj['cgroups'],
                                            route_id : mirror_obj['route_id'],
                                            reserved : mirror_obj['reserved_vlan']});
                            UI.createHint(mirror.hint_up);
                        }
                    }
                });
            }
        } // end callbackSwitch
    }

    function draw () {
        /* jshint validthis:true */
        var x, y, s, r;
        CTX.save();
        CTX.translate(this.x, this.y);
        CTX.beginPath();
        if (this.type === 'router' && (this.status == 'down' || this.maintenance))
            CTX.globalAlpha = 0.5;

        // Scaling
        if (this.type == 'router') {
            if (Net.settings.show_ports) {
                this.nx = Math.max( Math.max(this.ports_pos.down, this.ports_pos.up) * Net.settings.scale/4, Net.settings.scale );
                this.ny = Math.max( Math.max(this.ports_pos.right, this.ports_pos.left) * Net.settings.scale/4, Net.settings.scale );
            } else {
                this.nx = Net.settings.scale;
                this.ny = Net.settings.scale;
            }
        } else {
            if (this.mode == 'SOC') {
                this.nx = Net.settings.scale*3;
                this.ny = Net.settings.scale*3;
            }
            else {
                this.nx = Net.settings.scale;
                this.ny = Net.settings.scale;
            }
        }

        CTX.drawImage(UI.icons[this.icon].img, -this.nx/2, -this.ny/2, this.nx, this.ny);
        if (this.isSelected) {
            CTX.lineWidth = 1;
            CTX.strokeStyle = Color.pink;
            CTX.strokeRect(-this.nx/2, -this.ny/2, this.nx, this.ny);
        }

        if (this.selectionType) { // include, exclude for switch routes
            CTX.lineWidth = 4;
            if (this.selectionType == 'include') {
                CTX.strokeStyle = Color.dgreen;
                CTX.moveTo(-this.nx/2, -this.ny);
                CTX.quadraticCurveTo(-5, -this.ny+10, 0, -this.ny/2);
                CTX.quadraticCurveTo(5, -this.ny, this.nx/2+10, -this.ny-20);
            } else if (this.selectionType == 'exclude') {
                CTX.strokeStyle = Color.red;
                CTX.moveTo(-this.nx/2, -this.ny/2);
                CTX.lineTo( this.nx/2,  this.ny/2);
                CTX.moveTo( this.nx/2, -this.ny/2);
                CTX.lineTo(-this.nx/2,  this.ny/2);
            } else if (this.selectionType == 'exact') {
                CTX.strokeStyle = Color.blue;
                var s = Math.min(this.nx, this.ny);
                CTX.arc(0, 0, s, 0, 2*Math.PI);
            }
            CTX.stroke();
        }

        if (this.maintenance) {
            CTX.lineWidth = 4;
            CTX.strokeStyle = Color.red;
            CTX.moveTo(-this.nx/2, -this.ny/2);
            CTX.lineTo( this.nx/2,  this.ny/2);
            CTX.moveTo( this.nx/2, -this.ny/2);
            CTX.lineTo(-this.nx/2,  this.ny/2);
            CTX.stroke();
        }

        if (Net.settings.show_names) {
            CTX.font = '16px PT Sans Narrow';
            CTX.fillStyle = Color.darkblue;
            CTX.lineWidth = 1;
            CTX.strokeStyle = 'white';
            CTX.strokeText(this.name, this.nx/2 + 10, 0);
            CTX.fillText(this.name, this.nx/2 + 10, 0);
        }
        CTX.restore();

        if (Net.settings.show_ports && (this.mode == 'CISCO' || this.mode == 'SOC')) {
            for (var i = 0, l = this.links.length; i < l; i++) {
                var p1 = Net.getPort(this.links[i].host1, this.links[i].host1_p),
                    p2 = Net.getPort(this.links[i].host2, this.links[i].host2_p);

                if (p1) {
                    this.links[i].host1.ports.forEach(function(p, i) {
                        p.computeDir();
                    });
                }
                else if (p2) {
                    this.links[i].host2.ports.forEach(function(p, i) {
                        p.computeDir();
                    });
                }
            }
        }

        // Draw multicast listener counter
        if(this.mode == 'IGMPlistener') {
            CTX.save();

            CTX.textAlign = "center";
            CTX.textBaseline = "middle";
            CTX.fillStyle='red';
            CTX.strokeStyle='green'
            CTX.font='20pt Arial';
            CTX.fillText(this.counter, this.x, this.y);
            CTX.strokeText(this.counter, this.x, this.y);

            CTX.restore();
        }
    }

    function getPort (of_port) {
        for (var i = 0, len = this.ports.length; i < len; i++) {
            if (this.ports[i].of_port == of_port) {
                return this.ports[i].lagPort || this.ports[i];
            }
        }
        return null;
    }

    function isOver (x, y, dx, dy) {
        /* jshint validthis:true */
        if (dx === undefined) {
            if (this.x-this.nx/2 <= x && x <= this.x+this.nx/2) {
                if (this.y-this.ny/2 <= y && y <= this.y+this.ny/2) {
                    this.isSelected = true;
                    return true;
                }
            }
        } else {
            if ((this.x <= Math.max(x, x+dx)) &&
                (this.x >= Math.min(x, x+dx)) &&
                (this.y <= Math.max(y, y+dy)) &&
                (this.y >= Math.min(y, y+dy))) {
                    this.isSelected = true;
                    return true;
            }
        }
        return false;
    }

    function delLinks () {
        this.links.forEach(function(link, i) {
            Net.del(link);
        });
        this.links.length = 0;
    }

    function delPorts () {
        this.ports.length  = 0;
        this.ports_pos.up    = 0;
        this.ports_pos.down  = 0;
        this.ports_pos.left  = 0;
        this.ports_pos.right = 0;
    }

    function showChangeNameMenu() {
        var menu, elements;
        var self = this;
        var html = getHtml();

        menu = UI.createMenu(html, {name: "endpoint_stats_" + name, pos: "left", width: "200px"});

        elements = {
            saveButton: menu.querySelector('.save'),
            nameInput: menu.querySelector('.input')
        };

        initEvents();

        function initEvents() {
            elements.saveButton.onclick = onClickSave;
        }

        function getHtml() {
            var html = '';

            html += '<p>Input name: </p>';
            html += '<input type="text" class="input" placeholder="name" value="' + self.name + '">';
            html += '<button class="save half blue">Save</button>';

            return html;
        }

        function onClickSave() {
            self.name = elements.nameInput.value;
            Server.setCookie("host_name_" + self.id, self.name);
            menu.remove();
        }
    }

    function changeRole (new_role) {
        this.mode = new_role;

        if (this.mode == "UNDEFINED")
            this.icon = 'cisco3';
        else if (this.mode == "DR")
            this.icon = 'atm2';
        else if (this.mode == "AR")
            this.icon = 'atm';
    }

    function setMaintenance (val) {
        this.maintenance = val;
        var path = '/switches/'+this.id+'/maintenance/';
        Server.ajax('PUT', path, {"maintenance": val});
        Net.need_draw = true;
    }

}();
