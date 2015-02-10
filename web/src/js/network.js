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
    };
    
    function appList (response) {
		if (response.applications) {
			var html = '';
			var apps = document.getElementsByName('apps')[0];
			response.applications.forEach(function(item) { 
				html += '<a href="' + item.page + '">' + item.name + '</a>';
			});
			apps.innerHTML = html;
		}
    }

    function init () {
        Server.ajax('GET', '/timeout/switch-manager&topology&host-manager/' + last_ev, setNet);

        function setNet (response) {        
            var timeout = response && response.timeout || [],
                _switches, _hosts, _links;
            timeout.forEach(function(item) { 
                if (item["switch-manager"])
                    _switches = item["switch-manager"];
                else if (item.topology)
                    _links = item.topology;
                else if (item["host-manager"])
                    _hosts = item["host-manager"];
            });

            var i, len;
            for (i = 0, len = _switches.length; i < len; ++i) {
                if (_switches[i].type == "Add") {
                    var h = newHost({
                        id: _switches[i].obj_id,
                        name: _switches[i].obj_info[0].DPID,
                        icon: 'router',
                    });                 
                }
                else if (_switches[i].type == "Delete") {
                    var obj = getHostByID(_switches[i].obj_id);
                    for (var j = 0, llen = obj.links.length; j < llen; ++j) {
                        Net.del(obj.links[j]);
                    }               
                    Net.del(obj);
                }
                if (last_ev < _switches[i].event_id)
                    last_ev = _switches[i].event_id;
            }

            for (i = 0, len = _links.length; i < len; ++i) {
                if (_links[i].type == "Add")
                newLink({
                    id: _links[i].obj_id,
                    host1: _links[i].obj_info[0].connect[0],
                    host2: _links[i].obj_info[0].connect[1],
                    //bandwidth: links[i].obj_info[0].bandwidth,
                });
                else if (_links[i].type == "Delete") {
                    var obj = getLinkByID(_links[i].obj_id);
                    Net.del(obj);
                }

               if (last_ev < _links[i].event_id)
                    last_ev = _links[i].event_id;
            }

            for (i = 0, len = _hosts.length; i < len; ++i) {
                if (_hosts[i].type == "Add") {
                    newHost({
                        id: _hosts[i].obj_id,
                        name: _hosts[i].obj_info[0].mac,
                        icon: 'aimac',
                    });
                    newLink({
                        id: Math.random() * (2000 - 1000) + 1000,
                        host1: _hosts[i].obj_id,
                        host2: _hosts[i].obj_info[0].switch_id,
                        //bandwidth: links[i].obj_info[0].bandwidth,
                    });
                }
                else if (_hosts[i].type == "Delete") {
                    var obj = getHostByID(_hosts[i].obj_id);
                    Net.del(obj);
                }

                if (last_ev < _hosts[i].event_id)
                    last_ev = _hosts[i].event_id;
            }
        }

        setTimeout(init, 2000);
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

    function getHostByID (id) {
        var i, len;
        for (i = 0, len = hosts.length; i < len; ++i) {
            if (hosts[i].id === id) {
                return hosts[i];
            }
        }
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
        // setTimeout(save, 5000);
    }
    
    function clear() {
		hosts.length = 0;
		links.length = 0;
		last_ev = 0;
	}
	
}();
