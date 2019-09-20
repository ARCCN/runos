/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';

/* globals Net:true */
Net = function () {
    var hosts   = [],
        links   = [],
        routes  = [],
        domains = [],
        lag_group = [],
        soc     = false,
        need_draw,

        new_domain = false,
        new_domain_points = [],

        multicast_tree = false,

        import_queue = {},
        ctrl_stat = false,

        settings = {
            animation        : true,  // window animation
            scale            : 64,    // scaling
            switch_update    : 60,    // update timeout for switches
            show_ports       : true,  // show switch ports in topology
            show_lag         : true,  // show LAG interfaces
            show_storm       : true,  // show Storm control for interfaces
            show_link_load   : true,  // get information about port loading and draw it
            show_down_ports  : false, //
            show_down_sw     : true,  //
            show_soc         : true,  //
            show_names       : true,
            canvasTranslateX : 0,
            canvasTranslateY : 0,
            canvasScaleX     : 1,
            canvasScaleY     : 1,

            ctrl_status      : true,  // check connection to controller
        };
    return {
        hosts       : hosts,
        links       : links,
        routes      : routes,
        domains     : domains,
        need_draw   : need_draw,

        init        : init,
        clear       : clear,
        clearNew    : clearNew,

        add         : add,
        del         : del,
        draw        : draw,
        getHostByID : getHostByID,
        getPort     : getPort,
        positions   : positions,
        counter     : counter,
        getRoutesByHostId   : getRoutesByHostId,
        getRouteById        : getRouteById,
        getPassRoutesByHostId : getPassRoutesByHostId,
        getLinkByPort       : getLinkByPort,
        getLinkByDesc       : getLinkByDesc,
        getDomainByName     : getDomainByName,

        getZygoteDomain     : getZygoteDomain,
        redrawZygoteDomain  : redrawZygoteDomain,
        deleteZygoteDomain  : deleteZygoteDomain,

        setCanvasScale      : setCanvasScale,
        setCanvasTranslate  : setCanvasTranslate,

        new_domain          : new_domain,
        new_domain_points   : new_domain_points,

        import_queue        : import_queue,

        settings : settings,
        multicast: false,
        multicast_access_list_obj: false,
        new_query_ports: false,
        new_multicast_listeners: false
    };

    function setCanvasScale(x, y) {
        if (!isNaN(parseInt(x))) Net.settings.canvasScaleX = x;
        if (!isNaN(parseInt(y))) Net.settings.canvasScaleY = y;

        Net.need_draw = true;
    }

    function setCanvasTranslate(x, y) {
        if (!isNaN(parseInt(x))) Net.settings.canvasTranslateX = x;
        if (!isNaN(parseInt(y))) Net.settings.canvasTranslateY = y;

        Net.need_draw = true;
    }

    function updateWebInfo () {

        if (isProxyUp) return;

        for (var i = 0, l = Net.hosts.length; i < l; i++) {
            var dev = Net.hosts[i];
            Server.setCookie("pos_" + dev.id + "_x", dev.x);
            Server.setCookie("pos_" + dev.id + "_y", dev.y);
        }
    }

    function init () {
        this.need_draw = true;
        first_time = true;
        Server.ajax('GET', '/switches/', initSwitches);
        Server.ajax('GET', '/links/', initLinks);

        setInterval(updateTopo, 1000);
        setInterval(updateWebInfo, 2000);

        if (!Net.settings.ctrl_status) {
            document.body.querySelector('.ctrl-status').classList.toggle('ctrl-status_down', false);
        }

        function initSwitches (response) {
            var switches = response["array"],
                size     = response["_size"];

            for (var i = 0; i < size; i++) {
                var sw = switches[i];
                var name = sw["hardware"];

                newHost({
                    id:     sw["dpid"],
                    peer:   sw["peer"],
                    name:   GlobalStore[sw["dpid"]+"_name"] || name,
                    x:      Number(GlobalStore["pos_"+sw["dpid"]+"_x"]) || Number(Server.getCookie("pos_"+sw["dpid"]+"_x")),
                    y:      Number(GlobalStore["pos_"+sw["dpid"]+"_y"]) || Number(Server.getCookie("pos_"+sw["dpid"]+"_y")),
                    status: sw["connection-status"],
                    icon:   'cisco3',
                    nx: Net.settings.scale,
                    ny: Net.settings.scale
                });
            }
        }

        function initLinks (response) {
            var links = response["array"],
                size  = response["_size"];

            for (var i = 0; i < size; i++) {
                var link = links[i];
                newLink({
                    host1:   link["source_dpid"],
                    host1_p: link["source_port"],
                    host2:   link["target_dpid"],
                    host2_p: link["target_port"]
                });
            }
        }
    }

    function updateTopo () {
        Server.ajax('GET', '/switches/', updateSwitches, ctrlOff);
        Server.ajax('GET', '/switches/ports/stats/', updateMassPortStats);
        Server.ajax('GET', '/bridge_domains/', updateDomains);
        Server.ajax('GET', '/aux-devices/', updateDevices);
        Server.ajax('GET', '/links/', updateLinks);
        Server.ajax('GET', '/cisco_links/', updateCiscoLinks);
        if(UI.showMulticastTree !== undefined && UI.showMulticastTree != false)
            Server.ajax('GET', '/multicast_group/' + Net.multicast_tree["group-name"] + '/', updateMulticastTree);
        Server.ajax('GET', '/multicast_group_list/hosts/', updateMulticastLinks);
        Server.ajax('GET', '/multicast_access_list/0.0.0.0/', updateMulticastAccessList);
        Server.ajax('GET', '/storm-control/activity/', updateStormStatus);

        function ctrlOn () {
            if (Net.settings.ctrl_status)
                document.body.querySelector('.ctrl-status').classList.toggle('ctrl-status_down', false);
        }

        function ctrlOff () {
            if (Net.settings.ctrl_status)
                document.body.querySelector('.ctrl-status').classList.toggle('ctrl-status_down', true);
        }

        function updateSwitches (response) {
            ctrlOn();

            var switches = response["array"],
                size     = response["size_"];

            for (var i = 0, len = switches.length; i < len; i++) {
                var sw = switches[i];
                var router = getHostByID(sw["dpid"]);
                if (!router) {
                    var name = sw["hardware"];
                    var h = newHost({
                        id:     sw["dpid"],
                        peer:   sw["peer"],
                        name:   GlobalStore[sw["dpid"]+"_name"] || name,
                        x:      Number(GlobalStore["pos_"+sw["dpid"]+"_x"]) || Number(Server.getCookie("pos_"+sw["dpid"]+"_x")),
                        y:      Number(GlobalStore["pos_"+sw["dpid"]+"_y"]) || Number(Server.getCookie("pos_"+sw["dpid"]+"_y")),
                        status: sw["connection_status"],
                        icon:   'atm',
                        nx: Net.settings.scale,
                        ny: Net.settings.scale,
                    });
                    UI.createHint(h.hint_up);
                }
                else {
                    if (sw["connection-status"] == 'down') {
                        if (router.status == 'down')
                            continue;
                        router.down_count++;
                        if (router.down_count < 2)
                            continue;

                        router.delLinks();
                        router.delPorts();
                        UI.createHint(router.hint_down, {fail: true});
                        /*for (var j = 0, l = Net.domains.length; j < l; j++) {
                            var pos = Net.domains[j].hosts.findIndex(function(el, index, array) {
                                return el == router;
                            });
                            if (pos != -1)
                                Net.domains[j].hosts.splice(pos, 1);
                        }
                        Net.del(router);*/
                    } else if (sw["connection-status"] == 'up') {
                        router.down_count = 0;
                        if (router.status == 'down') {
                            UI.createHint(router.hint_up);
                            router.requestPorts();
                        }
                    } else {
                        window.console.error("Incorrect switch status:", sw["connection-status"]);
                        continue;
                    }
                    router.status = sw["connection-status"];
                }
            }
        }

        function updateMassPortStats (response) {
            var ports = response["array"],
                size  = response["_size"];

            for (var i = 0; i < size; i++) {
                var dpid = ports[i].dpid,
                    port = ports[i].port,
                    stat = ports[i].stats;

                var port = Net.getPort(dpid, port);
                if (port)
                    port.updateStats(stat);
            }
            Net.need_draw = true;
        }

        function updateLinks (response) {
            var links = response["array"],
                size  = response["_size"],
                tmp = [];

            for (var i = 0; i < size; i++) {
                var link = links[i];
                if (getLinkByDesc(link) == null) {
                    var l = newLink({
                        host1:   link["source_dpid"],
                        host1_p: link["source_port"],
                        host2:   link["target_dpid"],
                        host2_p: link["target_port"],
                    });
                    //UI.createHint(l.hint_up);
                }

                tmp.push(getLinkByDesc(link));
            }

            for (var j = 0; j < Net.links.length; j++) {
                var found = false;
                var l = Net.links[j];
                if (l.to_host) {
                    found = true;
                }
                for (var i = 0, len = tmp.length; i < len; i++) {
                    if (l == tmp[i]) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    window.console.log("DELETE LINK", l);
                    Net.del(l);
                    //UI.createHint(l.hint_down, {fail: true});
                }
            }
        }

        function updateCiscoLinks (response) {
            var links = response["array"],
                size  = response["_size"];

            for (var i = 0; i < size; i++) {
                var dev = links[i],
                    dev_id = 15000,
                    name = dev["device_id"],
                    ports = dev["ports"],
                    version = dev["software_version"],
                    platform = dev["platform"],
                    cap = dev["capabilities"];

                for (var j = 0, l = name.length; j < l; j++) {
                    dev_id += name.charCodeAt(j);
                }
                dev_id = dev_id.toString();
                var cisco = getHostByID(dev_id);
                if (cisco && cisco.mode != 'CISCO')
                    window.console.error("Cisco ID collision!");

                if (ports.length == 0) {
                    if (cisco) {
                        cisco.links.forEach(function(link) {
                            Net.del(link);
                        });
                    }
                } else {
                    if (!cisco) {
                        cisco = newHost({
                            id:     dev_id,
                            name:   GlobalStore[dev_id+"_name"] || name,
                            mode:   'CISCO',
                            x:      Number(GlobalStore["pos_"+dev_id+"_x"]) || Number(Server.getCookie("pos_"+dev_id+"_x")),
                            y:      Number(GlobalStore["pos_"+dev_id+"_y"]) || Number(Server.getCookie("pos_"+dev_id+"_y")),
                            icon:   'cisco4'
                        });
                        cisco.platform = platform;
                        cisco.version = version;
                        cisco.cap = cap;
                    }

                    cisco.links.filter(function(link) {
                        if (link.host1.type != 'router' && link.host2.type != 'router')
                            return true;

                        for (var j = 0, len = ports.length; j < len; j++) {
                            var l = getLinkByPort(ports[j]["dpid"], ports[j]["of_port"]);
                            if (l == link)
                                return true;
                        }
                        window.console.log("not found", link);
                        Net.del(link);
                        return false;
                    });

                    ports.forEach(function(p) {
                        var link = getLinkByPort(p["dpid"], p["of_port"]);
                        var sw = getHostByID(p["dpid"]);
                        var port = getPort(sw, p["of_port"]);
                        if (!sw || sw.status == 'down' || !port)
                            return;

                        if (!link || (link.host1 != cisco && link.host2 != cisco)) {
                            newLink({
                                host1: p["dpid"],
                                host1_p: p["of_port"],
                                host2: dev_id,
                                host2_p: p["name"],
                                to_host: true,
                            });
                        }
                    });
                }
            }
        }

        function updateDomains (response) {
            var domains = response["array"],
                _size   = response["_size"],
                error   = false;

            if (Net.domains.length > 0) {
                Net.domains.filter(function(domain) {
                    if(domain.zygote) return true;

                    for (var i = 0, len = domains.length; i < len; i++) {
                        if (domain.name == domains[i].name) {
                            // update routes
                            if (!utils.checkArrayEqual(domain.routesId, domains[i].routesId)) {
                                domain.routesId = domains[i].routesId; // update routes

                                /* update ui:domain_routes_menu */
                                var domainRoutesMenu = UI.getMenu('domain-routes');
                                if (domainRoutesMenu) {
                                    domainRoutesMenu.statics.update();
                                }
                                /* END update ui:domain_routes_menu */
                            }
                            // END update routes

                            // update deny
                            domain.deny = domains[i].deny;
                            return true;
                        }
                    }
                    UI.createHint(domain.hint_down);
                    Net.del(domain);
                    return false;
                });
            }

            for (var i = 0; i < _size; i++) {
                var bd = domains[i],
                    type = bd["type"],
                    name = bd["name"],
                    deny = bd["deny"],
                    ar  = (bd["auto_rerouting"] == "true" || bd["auto_rerouting"] == true ? true : false),
                    dr  = (bd["auto_dr_switching"] == "true" || bd["auto_dr_switching"] == true ? true : false),
                    error = false;

                if (Net.getDomainByName(name)) { ///exists
                    continue;
                }

                var host_list = [], port_list = [], end_list = [];
                bd["sw"].forEach(function(sw, i) {
                    var h = Net.getHostByID(sw["dpid"]);
                    if (!h) {
                        window.console.error("updateDomains(response)", "no switch", sw["dpid"]);
                        error = true;
                        return;
                    }
                    host_list.push(h);
                    sw["ports"].forEach(function(port, j) {
                        var p = Net.getPort(h, port["port_num"]);
                        if (p) {
                            port_list.push(p);
                            p.end_point.push(port["name"]);
                            end_list.push({ name: port["name"],
                                            port: p, s_vlan: port["stag"],
                                            c_vlan: port["ctag"],
                                            stag_start: port["stag_start"],
                                            stag_end: port["stag_end"]});
                        }
                        else {
                            return;
                        }
                    });
                });

                if (!error && host_list.length > 0) {
                    var d = newDomain({name: name,
                                       mode: type,
                                       hosts: host_list,
                                       ports: port_list,
                                       endpoints: end_list,
                                       routesId: bd.routesId,
                                       auto_rerouting: ar,
                                       auto_dr_switching: dr,
                                       deny : deny});
                    if (!first_time)
                        UI.createHint(d.hint_up);
                }
            }
            first_time = false;
        }

        function updateDevices (response) {
            var devices = response["array"],
                size  = response["_size"],
                exist_list = [];

            for (var i = 0; i < size; i++) {
                var device = devices[i];
                var dev_id = Hash(device.name).toString();
                var deviceName = decodeURIComponent(device.name);
                var found = getHostByID(dev_id);

                if (found && found.mode == 'AUX') {
                    exist_list.push(dev_id);
                    continue;
                }

                var port = getPort(device["sw_dpid"], device["sw_port"]);
                if (!port) continue;

                var new_dev = newHost({
                    id:     dev_id,
                    name:   GlobalStore[dev_id+"_name"] || deviceName,
                    mode:   'AUX',
                    x:      Number(GlobalStore["pos_"+dev_id+"_x"]) || Number(Server.getCookie("pos_"+dev_id+"_x")) || Rand(200, 600),
                    y:      Number(GlobalStore["pos_"+dev_id+"_y"]) || Number(Server.getCookie("pos_"+dev_id+"_y")) || Rand(200, 600),
                    icon:   device.icon
                });

                newLink({
                    host1: port.router.id,
                    host1_p: port.of_port,
                    host2: dev_id,
                    host2_p: 0,
                    to_host: true,
                });

                new_dev.desc = device["desc"];
                exist_list.push(dev_id);
            }

            for (var i = 0, l = Net.hosts.length; i <l; i++) {
                var dev = Net.hosts[i];
                if (!dev || dev.mode != 'AUX') continue;

                var found = exist_list.findIndex(function (el) {
                    return el == dev.id;
                });

                if (found == -1) {
                    Net.del(dev.links[0]);
                    Net.del(dev);
                }
            }
        }

        function updateMulticastTree (response) {
            multicast = response["multicast"];

            if(!multicast["multicast-groups"])
                return;

            // Check multicast tree for updates
            if(Net.multicast_tree !== undefined && Net.multicast_tree != false) {
                multicast["multicast-groups"].forEach (function (group) {
                    if(Net.multicast_tree["multicast-address"] == group["multicast-address"]) {
                        Net.multicast_tree = group;
                    }
                });
            }
        }

        function updateMulticastLinks (response) {
            multicast = response["multicast"];

            var groups = multicast["multicast-groups"];
            var new_server_links = [],
                old_server_links = [];
            var entity_change = false;

            // Clear server links
            for (var i = 0; i < Net.links.length; i++) {
                if (links[i].host1.id == "multicast_server_picture" || links[i].host2.id == "multicast_server_picture") {
                    old_server_links.push(links[i]);
                }
            }

            // Get multicast groups
            var listener_counter = {};
            for (var group_i = 0; group_i < groups.length; group_i++) {
                var group = groups[group_i];

                var group_name = group["group-name"],
                    multicast_address = group["multicast-address"],
                    multicast_servers = group["multicast-servers"],
                    multicast_listeners = group["multicast-listeners"];

                // Create multicast server hosts
                for (var server_i = 0; server_i < multicast_servers.length; server_i++) {
                    var server = multicast_servers[server_i];

                    var switch_dpid = server["switch"],
                        port = server["port"],
                        mac_address = server["mac-address"],
                        vlan_id = server["vlan-id"],
                        ip_address = server["ip-address"];

                    // Unique PC host id is mac,vlan
                    //var dev_id = switch_dpid.toString() + "," + port.toString();
                    var dev_id = "multicast_server_picture";

                    // Create new host if doesn't exist
                    var server_host = getHostByID(dev_id);
                    if (!server_host) {
                        entity_change = true;
                        server_host = newHost({
                            id:     dev_id,
                            sw_dpid: switch_dpid,
                            sw_port: port,
                            name:   "Server",
                            mode:   'IGMPserver',
                            s: 256,
                            x:      Number(GlobalStore["pos_"+dev_id+"_x"]) || Number(Server.getCookie("pos_"+dev_id+"_x")) || Rand(200, 600),
                            y:      Number(GlobalStore["pos_"+dev_id+"_y"]) || Number(Server.getCookie("pos_"+dev_id+"_y")) || Rand(200, 600),
                            icon:   'camera'
                        });
                    }
                    server_host.mac_address = mac_address;
                    server_host.vlan_id = vlan_id;
                    server_host.ip_address = ip_address;
                    // TODO: dpid in host class

                    // Create new link if doesn't exist
                    var server_link = getLinkByPort(switch_dpid, port);
                    var server_sw = Net.getHostByID(switch_dpid);
                    var server_port = getPort(server_sw, port);

                    if(server_sw && server_sw.status != "down")
                    {
                        // Creare server link
                        if (!server_link) {
                            newLink({
                                host1: switch_dpid,
                                host1_p: port,
                                host2: dev_id,
                                host2_p: 100,
                                to_host: true,
                            });
                        } else {
                            new_server_links.push(server_link);

                            // If router on the same port that server is behind router
                            if (server_link.host1.mode == 'CISCO' || server_link.host2.mode == 'CISCO') {
                                server_cisco = server_link.host1.mode == 'CISCO' ? server_link.host1 : server_link.host2;
                                var cisco_link = getLinkByPort(server_cisco.id, 'multicast_port');
                                new_server_links.push(cisco_link);
                                if (!cisco_link) {
                                    newLink({
                                        host1: server_cisco.id,
                                        host1_p: 'multicast_port',
                                        host2: dev_id,
                                        host2_p: 100,
                                        to_host: true,
                                    });
                                }
                            }
                            else {
                                // Refresh existed link (in order to prevent it link to the cloud)
                                if (server_link.host2.mode == 'SOC') {
                                    Net.del(server_link);
                                    newLink({
                                        host1: switch_dpid,
                                        host1_p: port,
                                        host2: dev_id,
                                        host2_p: 100,
                                        to_host: true,
                                    });
                                }
                            }
                        }
                    }
                }

                // Create multicast listener hosts
                for (var listener_i = 0; listener_i < multicast_listeners.length; listener_i++) {
                    var listener = multicast_listeners[listener_i];

                    var switch_dpid = listener["switch"],
                        port = listener["port"],
                        mac_address = listener["mac-address"],
                        vlan_id = listener["vlan-id"],
                        ip_address = listener["ip-address"];

                    // Unique PC host id is mac,vlan
                    //var dev_id = mac_address.toString() + "," + vlan_id.toString();
                    // Unique PC host id is dpid:port
                    var dev_id = switch_dpid.toString() + ":" + port.toString();

                    // Create new host if doesn't exist
                    if(listener_counter[dev_id] === undefined)
                        listener_counter[dev_id] = listener["counter"];
                    else
                        listener_counter[dev_id]++;

                    var listener_host = getHostByID(dev_id);
                    if (!listener_host) {
                        entity_change = true;
                        listener_host = newHost({
                            id:     dev_id,
                            sw_dpid: switch_dpid,
                            sw_port: port,
                            name:   "Host",
                            mode:   'IGMPlistener',
                            x:      Number(GlobalStore["pos_"+dev_id+"_x"]) || Number(Server.getCookie("pos_"+dev_id+"_x")) || Rand(200, 600),
                            y:      Number(GlobalStore["pos_"+dev_id+"_y"]) || Number(Server.getCookie("pos_"+dev_id+"_y")) || Rand(200, 600),
                            icon:   'tv'
                        });
                        listener_host.counter = listener_counter[dev_id];
                    } else {
                        listener_host.counter = listener_counter[dev_id];
                    }
                    listener_host.mac_address = mac_address;
                    listener_host.vlan_id = vlan_id;
                    listener_host.ip_address = ip_address;
                    // TODO: dpid in host class

                    // Create new link if doesn't exist
                    var listener_link = getLinkByPort(switch_dpid, port);
                    var listener_sw = Net.getHostByID(switch_dpid);
                    var listener_port = getPort(listener_sw, port);

                    if(listener_sw && listener_sw.status != "down")
                    {
                        // Creare listener link
                        if (!listener_link) {
                            newLink({
                                host1: switch_dpid,
                                host1_p: port,
                                host2: dev_id,
                                host2_p: 0,
                                to_host: true,
                            });
                        } else {
                            // If router on the same port that server is behind router
                            if (listener_link.host1.mode == 'CISCO' || listener_link.host2.mode == 'CISCO') {
                                listener_cisco = listener_link.host1.mode == 'CISCO' ? listener_link.host1 : listener_link.host2;
                                var cisco_link = getLinkByPort(listener_cisco.id, 'multicast_port');
                                if (!cisco_link) {
                                    newLink({
                                        host1: listener_cisco.id,
                                        host1_p: 'multicast_port',
                                        host2: dev_id,
                                        host2_p: 0,
                                        to_host: true,
                                    });
                                }
                            } else {
                                    // Refresh existed link (in order to prevent it link to the cloud)
                                    if (listener_link.host2.mode == "SOC") {
                                        Net.del(listener_link);
                                        newLink({
                                            host1: switch_dpid,
                                            host1_p: port,
                                            host2: dev_id,
                                            host2_p: 0,
                                            to_host: true,
                                        });
                                    }
                                }
                        }
                    }
                }
            }

            // Delete links that are not in ajax
            old_server_links.forEach(function(link) {
                var exists = false;

                for (var i = 0, len = new_server_links.length; i < len; i++) {
                    if(new_server_links[i].host1.id == link.host1.id) {
                        exists = true;
                    }
                }
                if(!exists)
                    Net.del(link);
            });

            // Delete listeners that are not in ajax requestPorts
            for (var host_i = 0; host_i < Net.hosts.length; host_i++) {
                var host = Net.hosts[host_i];

                if ( (host.type == 'host') && ((host.mode == 'IGMPlistener')||(host.mode == 'IGMPserver')) ) {
                    var found = false;

                    for (var group_i = 0; group_i < groups.length; group_i++) {
                        var group = groups[group_i];

                        var group_name = group["group-name"],
                            multicast_address = group["multicast-address"],
                            multicast_servers = group["multicast-servers"],
                            multicast_listeners = group["multicast-listeners"];

                        for (var ajax_host_j = 0; ajax_host_j < multicast_listeners.length; ajax_host_j++) {
                            ajax_host = multicast_listeners[ajax_host_j];
                            if ((ajax_host["mac-address"] == host.mac_address) &&
                                (ajax_host["vlan-id"] == host.vlan_id)) {
                                    found = true;
                                }
                        }
                        for (var ajax_host_j = 0; ajax_host_j < multicast_servers.length; ajax_host_j++) {
                            ajax_host = multicast_servers[ajax_host_j];
                            if ((ajax_host["mac-address"] == host.mac_address) &&
                                (ajax_host["vlan-id"] == host.vlan_id)) {
                                    found = true;
                                }
                        }
                    }

                    if (!found) {
                        entity_change = true;
                        host.links.forEach(function(link_to_delete, i) {
                            Net.del(link_to_delete);
                        });
                        Net.del(host);
                    }
                }
            }

            // Refresh multicast management table
            if (UI.multicastRefreshAll == true || entity_change == true) {
                UI.multicastRefreshAll = false;
                Server.ajax('GET', '/multicast_group_list/groups/', UI.refreshMulticastManagementMenu);
            } else {
                // UI.refreshMulticastManagementMenuTables();
            }
        }

        function updateMulticastAccessList (response) {
            Net.multicast_access_list_obj = response["multicast-subnets"];

             // Refresh multicast management table
            if (UI.multicastRefreshAccessList == true) {
                UI.multicastRefreshAccessList = false;
                UI.refreshAccessListTable();
            } else {
            }

            // Get multicast groups
            /*for (var subnet_i = 0; subnet_i < groups.length; subnet_i++) {
                var subnet = subnets[subnet_i];

                var subnet_ip = subnet["ip-address"],
                    subnet_mask = subnet["ip-mask"];


            }*/
        }

        function updateStormStatus (response) {
            var s = response["array"],
                size  = response["_size"];

            for (var i = 0; i < size; i++) {
                var ss = s[i],
                    host = Net.getHostByID(ss["dpid"]);
                if (host) {
                    var port = host.getPort(ss["port"]);
                    if (port) {
                        if(ss["active"] == "1")
                            port.stormStatus(true);
                        else
                            port.stormStatus(false);
                    }
                }
            }
        }

    } /// end updateTopo()

    function add (obj) {
        if (obj == null) return;
        if (obj.type === 'host' || obj.type === 'router') {
            this.hosts.push(obj);
            Logger.info('topology', 'New ' + obj.name + ' with name ' + obj.name + '(' + obj.id + ') founded');
        } else if (obj.type === 'link') {
            this.links.push(obj);
            Logger.info('topology', 'Link ' + obj.host1.name + '(' + obj.host1.id + ') -- ' +
                                              obj.host2.name + '(' + obj.host2.id + ') is up');
        } else if (obj.type === 'domain') {
            this.domains.push(obj);
            if (!obj.zygote)
                Logger.info('domain', 'Domain (' + obj.mode + ') ' + obj.name + ' was created');
        }

    }

    function del (obj) {
        if (obj == null) return;
        if (obj.type === 'host' || obj.type === 'router') {
            Logger.info('topology', obj.type + ' with name ' + obj.name + '(' + obj.id + ') was removed');

            for (var i = 0, len = hosts.length; i < len; ++i) {
                if (hosts[i].id == obj.id) {
                    if (hosts[i].type == 'router') {
                        hosts[i].links.forEach(function(link) { Net.del(link); });
                    }
                    hosts.splice(i, 1);
                    return;
                }
            }

        } else if (obj.type === 'link') {
            Logger.info('topology', 'Link ' + obj.host1.name + '(' + obj.host1.id + ') -- ' +
                                              obj.host2.name + '(' + obj.host2.id + ') is down');

            for (var i = 0, len = links.length; i < len; ++i) {
                if (links[i] == obj) {
                    if (links[i].host1.type == "host" && links[i].host1.links.length == 1) {
                        del(links[i].host1);
                    }
                    if (links[i].host2.type == "host" && links[i].host2.links.length == 1) {
                        del(links[i].host2);
                    }
                    var p1 = Net.getPort(obj.host1, obj.host1_p),
                        p2 = Net.getPort(obj.host2, obj.host2_p);
                        if (p1) p1.link = null;
                        if (p2) p2.link = null;
                    links.splice(i, 1);
                    return;
                }
            }
        } else if (obj.type === 'domain') {
            if (!obj.zygote)
                Logger.info('domain', 'Domain (' + obj.mode + ') ' + obj.name + ' was removed');
            for (var i = 0, len = domains.length; i < len; ++i) {
                if (domains[i] == obj) {
                    for (var ii = 0, ll = obj.ports.length; ii < ll; ii++) {
                        obj.ports[ii].isDomainOn = false;
                    }
                    domains.splice(i, 1);
                    UI.reloadDomainList();
                    return;
                }
            }
        }
    }

    function draw () {
        var i;
        CTX.save();

        Background.draw();

        for (i = Net.links.length-1; i >=0; --i) {
            links[i].draw();
        }
        var host_array = [];
        for (i = Net.hosts.length-1; i >=0; --i) {
            if(hosts[i].type != 'host') {
                hosts[i].draw();
            } else {
                host_array.push(hosts[i]);
            }
            if (hosts[i].type === 'router' && Net.settings.show_lag) {

                for (var key in hosts[i].lag) {
                    var lag = hosts[i].lag[key];
                    if (lag.isVisible)
                        lag.draw();
                }

                /*for (var lag of hosts[i].lag.values()) {
                    if (lag.isVisible)
                        lag.draw();
                }*/

                /*for (var j = hosts[i].lag.length-1; j >=0; --j) {
                    if (hosts[i].lag[j].isVisible)
                        hosts[i].lag[j].draw();
                }*/
            }

            if (hosts[i].type === 'router' && Net.settings.show_ports) {
                for (var j = hosts[i].ports.length-1; j >=0; --j) {
                    if (hosts[i].ports[j].isVisible)
                        hosts[i].ports[j].draw();
                }
            }
        }
        for (i = Net.domains.length-1; i >=0; --i) {
            domains[i].draw();
        }

        UI.drawMulticastTree();
        for (i = host_array.length-1; i >=0; --i) {
            host_array[i].draw();
        }

        Routes.draw();

        CTX.restore();
        Net.need_draw = false;
    }

    function positions () {
        for (var i = Net.hosts.length-1; i >=0; --i) {
            window.console.info(Net.hosts[i].name, Net.hosts[i].x,Net.hosts[i].y);
        }
    }

    function getLinkByDesc (link_desc) {
        for (var i = 0, len = Net.links.length; i < len; ++i) {
            var cur = Net.links[i];
            if ( (cur.host1.id == link_desc["source_dpid"] && cur.host1_p == link_desc["source_port"]
                 && cur.host2.id == link_desc["target_dpid"] && cur.host2_p == link_desc["target_port"]) ||
                 (cur.host2.id == link_desc["source_dpid"] && cur.host2_p == link_desc["source_port"]
                 && cur.host1.id == link_desc["target_dpid"] && cur.host1_p == link_desc["target_port"]) )
            return cur;
        }
        return null;
    }

    function getLinkByPort(sw_id, port_no) {
        var sw = getHostByID(sw_id);
        if (sw) {
            for (var i = 0, len = Net.links.length; i < len; ++i) {
                if ((Net.links[i].host1.id == sw_id && Net.links[i].host1_p == port_no) ||
                    (Net.links[i].host2.id == sw_id && Net.links[i].host2_p == port_no))
                     return Net.links[i];
            }
        }
        return null;
    }

    function getHostByID (id) {
        for (var i = 0, len = hosts.length; i < len; ++i) {
            if (hosts[i].id === id) {
                return hosts[i];
            }
        }
        return null;
    }

    function getPort (sw, port_num) {
        if (typeof sw === "string")
            sw = getHostByID(sw);
        if (!sw || sw.type != 'router') {
            //window.console.error("not a switch!");
            return null;
        }

        for (var i = 0, len = sw.ports.length; i < len; i++) {
            if (Number(sw.ports[i].of_port) == Number(port_num))
                return sw.ports[i];
        }

        return null;
    }

    function getDomainByName (name) {
        for (var i = 0, len = Net.domains.length; i < len; i++) {
            if (Net.domains[i].name == name)
                return Net.domains[i];
        }

        return null;
    }

    function getZygoteDomain () {
        for (var i = 0, len = Net.domains.length; i < len; i++) {
            if (Net.domains[i].zygote)
                return Net.domains[i];
        }

        return null;
    }

    function redrawZygoteDomain () {
        var domain = getZygoteDomain();
        if (domain == null)
            domain = newDomain({zygote: true});

        // Get new endpoints
        var host_list = [], port_list = [], end_list = [];
        Net.new_domain_points.forEach(function(sw, i) {
            var h = Net.getHostByID(sw["dpid"]);
            if (!h) {
                window.console.error("redrawZygoteDomain("+domain_old_name+")", "no switch", sw["dpid"]);
                error = true;
                return;
            }
            host_list.push(h);
            sw["ports"].forEach(function(port, j) {
                var p = Net.getPort(h, port["port_num"]);
                if (p) {
                    port_list.push(p);
                    p.end_point.push(port["name"]);
                    end_list.push({ name: port["name"],
                                    port: p, s_vlan: port["stag"],
                                    c_vlan: port["ctag"],
                                    stag_start: port["stag_start"],
                                    stag_end: port["stag_end"]});
                }
                else {
                    return;
                }
            });
        });

        domain.hosts = host_list;
        domain.hosts_to_draw = host_list;
        domain.ports = port_list;
        domain.endpoints = end_list;
    }

    function deleteZygoteDomain () {
        Net.del(getZygoteDomain());
    }

    function getRoutesByHostId(id) {
        var host = Net.getHostByID(id);
        return routes.filter(function(el) {
            return el.startHost == host || el.endHost == host;
        }) || null;
    }
    function getRouteById(id) {
        return routes.find(function(el) {
            return el.id == id;
        }) || null;
    }
    function getPassRoutesByHostId(id) {
        var host = getHostByID(id);
        return routes.filter(function(el) {
            return (checkPath(el.paths.main_route) ||
                    checkPath(el.paths.alt_route));
        }) || null;

        function checkPath(pathObj) {
            var path = pathObj.path;
            var pathsLength = path.length;

            for (var i = 1; i < pathsLength - 1; ++i) {
               if (path[i][0] == id) {
                   return true;
               }
            }
            return false;
        }
    }

    function counter (mode) {
        var count = 0;
        for (var i = 0, len = hosts.length; i < len; i++) {
            if (hosts[i].mode == mode && hosts[i].static_pos == false)
                count++;
        }

        return count;
    }

    function clear() {
        for (var i = 0, l = hosts.length; i < l; i++) {
            for (var j = 0, k = hosts[i].ports.length; j < k; j++) {
                hosts[i].ports[j].router = false;
            }
        }
        hosts.length = 0;
        links.length = 0;
        domains.length = 0;
        soc = false;
        if (UI.menu)
            UI.menu.style.display = 'none';
        clearNew();
    }

    function clearNew() {
        Net.new_domain = false;
        Net.new_domain_points.length = 0;

        Net.new_lag = false;
        Net.new_mirror = false;

        Net.new_query_ports = false;

        for (var i = 0, len = Net.hosts.length; i < len; i++) {
            for (var i2 = 0, len2 = Net.hosts[i].ports.length; i2 < len2; i2++) {
                Net.hosts[i].ports[i2].isSelected = false;
                Net.hosts[i].ports[i2].isMirrorInPort = null;
            }
        }
        Net.need_draw = true;
    }

}();
