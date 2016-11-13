/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use strict';

/* globals Net:true */
Net = function () {
    var hosts = [],
        links = [],
        last_ev = 0;
    return {
        hosts       : hosts,
        links       : links,
        last_ev     : last_ev,
        init        : init,
        save        : save,
        clear       : clear,
        add         : add,
        del         : del,
        draw        : draw,
        getHostByID : getHostByID,
        getLinkByID : getLinkByID,
        positions   : positions,
        appList     : appList,
        newPos      : newPos,
        newDevPos   : newDevPos,
        getPosition : getInfo,
        getLinkByPort : getLinkByPort,
    };

    function getInfo (response) {
        if (!response.ID)
            return;

        var obj = getHostByID(response.ID);
        obj.name = response.display_name;

        if (response.x_coord < 0 || response.y_coord < 0) {
            var coords;
            if (obj.type === 'router')
                coords = newPos();
            else {
                var sw_obj = (obj.links[0].host1 != obj ? obj.links[0].host1 : obj.links[0].host2);
                coords = newDevPos(sw_obj.id);
            }
            obj.x = coords.x;
            obj.y = coords.y;
        }
        else {
            obj.fixed_pos = true;
            obj.x = response.x_coord;
            obj.y = response.y_coord;
        }
    }

    function newPos () {
        var res = new Object(),
            cx = Math.round(UI.centerX),
            cy = Math.round(UI.centerY),
            pi = 3.14,
            count = 0,
            rad = Math.min(UI.centerX/2, UI.centerY/2);

        if (hosts.length == 0) {
            res.x = cx;
            res.y = cy;
            return res;
        }

        for (var i = 0; i < hosts.length; i++)
            if (hosts[i].type === "router")
                ++count;

        for (var i = 0; i < count-1; i++) {
            if (!hosts[i].fixed_pos) {
                hosts[i].x = Math.round(rad * Math.cos( 2*pi*i/count - pi/2 ) + cx);
                hosts[i].y = Math.round(rad * Math.sin( 2*pi*i/count - pi/2 ) + cy);
            }
        }
        res.x = Math.round(rad * Math.cos (2*pi*(count-1)/count - pi/2) + cx);
        res.y = Math.round(rad * Math.sin (2*pi*(count-1)/count - pi/2) + cy);
        return res;
    }

    function newDevPos (switch_id) {
        var res = new Object(),
            sw = getHostByID(switch_id),
            rad = 150,
            cx = Math.round(UI.centerX),
            cy = Math.round(UI.centerY),
            pi = 3.14,
            angle = 0,
            rand = Math.floor(Math.random() * 8) * pi / 4;

        angle = Math.atan( (sw.y - cy) / (sw.x - cx) );

        res.x = Math.round(rad * Math.cos( angle + rand ) + sw.x );
        res.y = Math.round(rad * Math.sin( angle + rand) + sw.y );
        return res;
    }

    function appList (response) {
        var ahtml = '', shtml = '';
        var srv_elem = document.getElementsByName('srv')[0];
        var apps_elem = document.getElementsByName('apps')[0];
        var body_class = document.title.toLowerCase();
        for (var app in response) {
            var page = (response[app].page == "none" ? "#" : response[app].page);
            var cur = (response[app].name.toLowerCase().indexOf(body_class) >= 0 || body_class.indexOf(response[app].name.toLowerCase()) >= 0 ? true : false);
            if (response[app].type == "Application")
				ahtml += '<a ' + (cur == true ? 'class = "current"' : '') + ' href="' + page + '">' + response[app].name + '</a>';
			if (response[app].type == "Service")
				shtml += '<a ' + (cur == true ? 'class = "current"' : '') + ' href="' + page + '">' + response[app].name + '</a>';
        }
        srv_elem.innerHTML = shtml;
        apps_elem.innerHTML = ahtml;
    }

    function init () {
        Server.ajax('GET', '/timeout/switch-manager&topology&host-manager&flow-manager/' + last_ev, setNet);

        function setNet (response) {
            var _switches = [],
                _hosts    = [],
                _links    = [],
                _flow     = [];
            var events = response["events"];
            last_ev = response["last_event"]

            if (events["switch-manager"])
                _switches = events["switch-manager"];
            if (events["topology"])
                _links = events["topology"];
            if (events["host-manager"])
                _hosts = events["host-manager"];
            if (events["flow"])
                _flow = events["flow-manager"];

            var i, len;
            for (i = 0, len = _switches.length; i < len; ++i) {
                if (_switches[i].type == "Add") {
                    var h = newHost({
                        id: _switches[i].obj_id,
                        name: _switches[i].obj_info.DPID,
                        icon: 'router',
                    });
                    Server.ajax('GET', '/api/webui/webinfo/' + _switches[i].obj_id, getInfo);
                }
                else if (_switches[i].type == "Delete") {
                    var obj = getHostByID(_switches[i].obj_id);
                    for (var j = 0, llen = obj.links.length; j < llen; ++j) {
                        Net.del(obj.links[j]);
                    }
                    Net.del(obj);
                }
            }

            for (i = 0, len = _links.length; i < len; ++i) {
                if (_links[i].type == "Add")
                newLink({
                    id: _links[i].obj_id,
                    host1: _links[i].obj_info.connect[0].src_id,
                    host2: _links[i].obj_info.connect[1].dst_id,
                    host1_p: _links[i].obj_info.connect[0].src_port,
                    host2_p: _links[i].obj_info.connect[1].dst_port,
                });
                else if (_links[i].type == "Delete") {
                    var obj = getLinkByID(_links[i].obj_id);
                    Net.del(obj);
                }
            }

            for (i = 0, len = _hosts.length; i < len; ++i) {
                if (_hosts[i].type == "Add") {
                    newHost({
                        id: _hosts[i].obj_id,
                        name: _hosts[i].obj_info.mac,
                        icon: 'aimac',
                    });
                    Server.ajax('GET', '/api/webui/webinfo/' + _hosts[i].obj_id, getInfo);
                    newLink({
                        id: Math.random() * (2000 - 1000) + 1000,
                        host1: _hosts[i].obj_id,
                        host2: _hosts[i].obj_info.switch_id,
                        host2_p: _hosts[i].obj_info.switch_port,
                    });
                }
                else if (_hosts[i].type == "Delete") {
                    var obj = getHostByID(_hosts[i].obj_id);
                    Net.del(obj);
                }
            }

            for (i = 0, len = _flow.length; i < len; ++i) {
                var sw = getHostByID(_flow[i].obj_info.switch_id)
                if (sw == null || sw.type != 'router') {
                    window.console.error('Not a switch');
                    continue;
                }

                if (_flow[i].type == "Add") {
                    var eth_src = (_flow[i].obj_info.eth_src != "00:00:00:00:00:00" ? _flow[i].obj_info.eth_src : ''),
                        eth_dst = (_flow[i].obj_info.eth_dst != "ff:ff:ff:ff:ff:ff" ? _flow[i].obj_info.eth_dst : 'BROADCAST'),
                        out_port = (_flow[i].obj_info.out_port != -4 ? _flow[i].obj_info.out_port : 'ALL'),
                        in_port = _flow[i].obj_info.in_port,
                        ip_src = _flow[i].obj_info.ip_src,
                        ip_dst = _flow[i].obj_info.ip_dst,
                        set_field = _flow[i].obj_info.set_field;

                    var actions = "";
                    var j, len2;
                    for (j = 0, len2 = out_port.length; j < len2; ++j){
                        if (out_port[j] == -3){
                            out_port[j] = "to-controller";
                        } else if (out_port[j] == -5){
                            out_port[j] = "FLOOD";
                        }
                    }
                    if (out_port.length == 0)
						out_port = 'drop';
                    else
						out_port = 'output: ' + out_port;
					actions += out_port;

					if (set_field.length != 0) {
						set_field = set_field[0];
						for (var key in set_field) {
							actions += "<br>set_field: " + set_field[key] + " -> " + key;
						}
					}

                    eth_dst = (eth_dst != "00:00:00:00:00:00" ? eth_dst : '');
                    ip_src = (ip_src != "0.0.0.0" ? ip_src : '');
                    ip_dst = (ip_dst != "0.0.0.0" ? ip_dst : '');

                    var rule = {id: _flow[i].obj_id,
                                in_port: in_port,
                                mac_src: eth_src,
                                mac_dst: eth_dst,
                                ip_src: ip_src,
                                ip_dst: ip_dst,
                                action: actions
                               }
                    sw.routingRules.set(_flow[i].obj_id, rule);

                }
                else if (_flow[i].type == "Delete") {
                    sw.routingRules.delete(_flow[i].obj_id);
                }
            }

        }
        setTimeout(init, 1000);
    }

    function add (obj) {
        if (obj.type === 'host' || obj.type === 'router') {
            this.hosts.push(obj);
        } else if (obj.type === 'link') {
            this.links.push(obj);
        }
    }

    function del (obj) {
		if (obj == null) return;
		if (obj.type === 'host' || obj.type === 'router') {
	    	for (var i = 0, len = hosts.length; i < len; ++i) {
	    	    if (hosts[i].id == obj.id) {
	    	        hosts.splice(i, 1);
	    	        return;
	    	    }
	    	}
		} else if (obj.type === 'link') {
	    	for (var i = 0, len = links.length; i < len; ++i) {
	    	    if (links[i].id == obj.id) {
	    	        if (links[i].host1.type == "host") {
	    	            del(links[i].host1);
	    	        }
	    	        if (links[i].host2.type == "host") {
	    	            del(links[i].host2);
	    	        }
	    	        links.splice(i, 1);
	    	        return;
	    	    }
	    	}
		}
    }

    function draw () {
        var i;
        CTX.save();
        for (i = Net.links.length-1; i >=0; --i) {
            links[i].draw();
        }
        for (i = Net.hosts.length-1; i >=0; --i) {
            hosts[i].draw();
        }
        CTX.restore();
    }

    function positions () {
        var i;
        for (i = Net.hosts.length-1; i >=0; --i) {
            window.console.info(Net.hosts[i].name, Net.hosts[i].x,Net.hosts[i].y);
        }
    }

    function getLinkByID (id) {
        var i, len;
        for (i = 0, len = links.length; i < len; ++i) {
            if (links[i].id === id) {
                return links[i];
            }
        }
    }

    function getLinkByPort(sw_id, port_no) {
        var sw = getHostByID(sw_id);
        if (sw && sw.type === 'router') {
            for (var i = 0, len = sw.links.length; i < len; ++i) {
                if ((sw.links[i].host1.id == sw_id && sw.links[i].host1_p == port_no) ||
                    (sw.links[i].host2.id == sw_id && sw.links[i].host2_p == port_no))
                     return sw.links[i];
            }
        }
    }

    function getHostByID (id) {
        var i, len;
        for (i = 0, len = hosts.length; i < len; ++i) {
            if (hosts[i].id === id) {
                return hosts[i];
            }
        }
        return null;
    }

    function save () {
        var i, len, j, jlen, t,
            json = {
                hosts: [],
                links: [],
            };
        for (i = 0, len = hosts.length; i < len; ++i) {
            json.hosts.push({
                id: hosts[i].id,
                name: hosts[i].name,
                icon: hosts[i].icon,
                x: hosts[i].x,
                y: hosts[i].y,
                ip: hosts[i].ip,
                s: hosts[i].s,
            });
        }
        for (i = 0, len = links.length; i < len; ++i) {
            json.links.push({
                id: links[i].id,
                host1: links[i].host1.id,
                host2: links[i].host2.id,
                bandwidth: links[i].bandwidth,
            });
        }
        Server.ajax('POST', 'dummy', json);
    }

    function clear() {
		hosts.length = 0;
		links.length = 0;
		last_ev = 0;
		if (UI.menu)
			UI.menu.style.display = 'none';
		if (UI.aux)
			UI.aux.style.display = 'none';
	}

}();
