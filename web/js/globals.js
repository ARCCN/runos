/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';
/* exported Color */
/* exported CTX */
/* exported HCI */
/* exported Logger */
/* exported Net */
/* exported newHost */
/* exported newLink */
/* exported Rand */
/* exported Routes */
/* exported UI */
/* globals Color:true */
/* globals CTX:true */
/* globals HCI:true */
/* globals Logger:true */
/* globals Net:true */
/* globals newHost:true */
/* globals newLink:true */
/* globals Rand:true */
/* globals Routes:true */
/* globals Recovery:true */
/* globals Info:true */
/* globals UI:true */
var Color = { fullred: '#FF0000',
              fullgreen: '#00FF00',
              fullblue: '#0000FF',
              lpink: '#F5A9F2',
              pink: '#DD1772',
              darkblue: '#281A71',
              lightblue: '#0093DD',
              blue: '#1B7EE0',
              dblue: '#1560AB',
              lblue: '#1F90FF',
              red: '#E01B53',
              green: '#C6FF40',
              dgreen: '#0B3B0B',
              yellow: '#debe1f' };

var CTX;
var ProxySettings;
var HCI;
var Logger;
var Net;
var Routes;
var Recovery;
var Info;
var Background;
var StaticEndpoint;
var newHost;
var newLink;
var newPort;
var newDomain;
var newLAG;
var newMirror;
var staticMirror;
var Rand = function (max, min) { min = min || (max -= 1, 0); return Math.floor(Math.random() * (max-min+1) + min); };
var UI = {};
var GlobalStore = {};
var isProxyUp = false;
var PortSpeed = {'10Mb-half-duplex'   : 10000,
                 '10Mb-full-duplex'   : 10000,
                 '100Mb-half-duplex'  : 100000,
                 '100Mb-full-duplex'  : 100000,
                 '1Gb-half-duplex'    : 1000000,
                 '1Gb-full-duplex'    : 1000000,
                 '10Gb-full-duplex'   : 10000000,
                 '40Gb-full-duplex'   : 40000000,
                 '100Gb-full-duplex'  : 100000000,
                 '1Tb-full-duplex'    : 1000000000,
                 'custom-rate'        : 0 };
var utils = {
    // @param value
    parseNetworkValue : function(value, isBit) {
        value = parseFloat(value);

        var b  = value.toFixed(1),
            kb = (b / 1000).toFixed(1),
            mb = (kb / 1000).toFixed(1);
        if (isBit) {
            return (mb > 0 ? mb + ' Mb' : (kb > 0 ? kb + ' Kb' : b + ' b'));
        } else {
            return (mb > 0 ? mb + ' MB' : (kb > 0 ? kb + ' KB' : b + ' B'));
        }
    },

    checkMaxInt : function(value) {
        var MAX_INT = 1844674487309551615;
        return isNaN(parseFloat(value)) || Number(value) >= MAX_INT;
    },

    checkArrayEqual : function(array1, array2) {
        if (!array1 && !array2) return true;
        if (!array1 || !array2 || array1.length != array2.length) return false;

        for (var i = 0; i < array1.length; ++i) {
            if (array1[i] != array2[i]) {
                return false;
            }
        }

        return true;
    },

    intToIP : function (value) {
        var part1 = value & 255;
        var part2 = (value >> 8) & 255;
        var part3 = (value >> 16) & 255;
        var part4 = (value >> 24) & 255;

        return part1 + "." + part2 + "." + part3 + "." + part4;
    },

    intToMAC : function (value) {
        function zeroPad(s, len) {
            if (s.length < len) {
                s = ('0000000000' + s).slice(-len);
            }
            return s;
        }

        var mac_hexValue = zeroPad( parseInt(value, 10).toString(16), 12 ).toUpperCase();

        var mac_address_array = [];
        for (var i=mac_hexValue.length-2; i >=0 ; i=i-2) {
            mac_address_array.push(mac_hexValue.substr(i,2));
        }
        return mac_address_array.join(':');
    },

    cutLength : function (value, max_symbols) {
        var ret = value.substr(0, max_symbols);
        if (ret != value) // cutted
            ret += '...';
        return ret;
    }
};

var flow_parser = {
    eth_type : {
        '2048'  : 'IP',
        '2054'  : 'ARP',
      //'33024' : 'DOT1Q',
        '35020' : 'LLDP'
    },
    ip_proto : {
        '2'  : 'IGMP',
        '4'  : 'IPv4',
        '6'  : 'TCP',
        '17' : 'UDP',
    },
    OF_ports : {
        0xFFFFFF00 : 'MAX',
        0xFFFFFFF8 : 'IN_PORT',
        0xFFFFFFF9 : 'TABLE',
        0xFFFFFFFA : 'NORMAL',
        0xFFFFFFFB : 'FLOOD',
        0xFFFFFFFC : 'ALL',
        0xFFFFFFFD : 'CTRL',
        0xFFFFFFFE : 'LOCAL',
        0xFFFFFFFF : 'ANY'
    },
    OF_VLAN_PRESENT : 4096,

    parsing : function (field, value) {
        if (field == 'instructions' && !value) return 'drop';
        else if (!value) return '*';

        switch(field) {
            case 'in_port'  : return parse_in_port(value);
            case 'vlan_vid' : return parse_vlan(value);
            case 'eth_type' : return parse_eth_type(value);
            case 'ip_proto' : return parse_ip_proto(value);
            case 'eth_dst'  : return parse_eth_addr(value);
            case 'eth_src'  : return parse_eth_addr(value);
            case 'metadata' : return parse_metadata(value);
            case 'cookie'   : return parse_cookie(value);
            case 'instructions' : return parse_instructions(value);
            case 'packet_count' : return parse_counters(value);
            case 'byte_count'   : return parse_counters(value);

            default: return value;
        }

        function parse_in_port(in_port) {
            if (flow_parser.OF_ports.hasOwnProperty(in_port)) {
                return flow_parser.OF_ports[in_port];
            }
            return in_port;
        }

        function parse_counters(counter) {
            return (utils.checkMaxInt(counter) ? 'NaN' : counter);
        }

        function parse_metadata(metadata) {
            return metadata.value + ' / ' + Number(metadata.mask).toString(16);
        }
        function parse_eth_addr(eth_addr) {
            if (eth_addr.value && eth_addr.mask) {
                return eth_addr.value + ' / ' + eth_addr.mask;
            }
            return eth_addr;
        }
        function parse_vlan (vlan_vid) {
            vlan_vid = Number(vlan_vid);
            return ((vlan_vid & flow_parser.OF_VLAN_PRESENT) == 0 ? 'NONE' :
                (vlan_vid  == flow_parser.OF_VLAN_PRESENT ? 'PRES' :
                vlan_vid & ~flow_parser.OF_VLAN_PRESENT));
        }

        function parse_eth_type (type) {
            if (flow_parser.eth_type.hasOwnProperty(type)) return flow_parser.eth_type[type];
            return "0x" + Number(type).toString(16);
        }

        function parse_cookie (cookie) {
            return "0x" + Number(cookie).toString(16).toUpperCase();
        }

        function parse_ip_proto (proto) {
            if (flow_parser.ip_proto.hasOwnProperty(proto)) {
                return flow_parser.ip_proto[proto];
            }
            return proto;
        }

        function parse_instructions (instr_list) {
            var ret = '';
            for (var key in instr_list) {
                if (key == "apply_actions") {
                    var act = instr_list[key];
                    for (var akey in act) {
                        ret += '(apply) ' + flow_parser.parse_action(akey, act[akey]);
                    }
                }
                else if (key == "write_actions") {
                    var act = instr_list[key];
                    for (var akey in act) {
                        ret += '(write) ' + flow_parser.parse_action(akey, act[akey]);
                    }
                }
                else if (key == "goto_table") {
                    ret += key + ': ' + instr_list[key].table_id + '<br>';
                }
                else if (key == "write_metadata") {
                    ret += key + ': ' + instr_list[key].metadata + '/' + (+instr_list[key].metadata_mask).toString(16) + '<br>';
                }
                else if (key == "meter") {
                    ret += '<a class="active_element" info-meter="'+instr_list[key].meter_id
                                    +'">' + key + ': ' + instr_list[key].meter_id  + '</a><br>';
                }
                else if (key == "clear_actions") {
                    ret += key + '<br>';
                }
            }

            return ret;
        }
    },

    parse_action : function (key, value) {
        var ret = '';
        if (key == "output") {
            value.forEach(function (curr) {
                ret += key + ': ' + parse_in_port(curr.port)  + '<br>';
            });
        }
        else if (key == "group") {
            value.forEach(function (curr) {
                ret += '<a class="active_element" info-group="'+curr.group_id
                    +'">' + key + ': ' + curr.group_id  + '</a><br>';
            });
        }
        else if (key == "push_vlan") {
            ret += key + ': ' + parse_eth_type(value.ethertype) + '<br>';
        }
        else if (key == "pop_vlan") {
            ret += key  + '<br>';
        }
        else if (key == "set_queue") {
            ret += key + ': ' + value.queue_id + '<br>';
        }
        else if (key == "set_field") {
            var set_field = value;
            for (var skey in set_field) {
                ret += key + ': ' + flow_parser.parsing(skey, set_field[skey]) + ' -> ' + skey + '<br>';
            }
        }

        return ret;

        function parse_in_port(in_port) {
            if (flow_parser.OF_ports.hasOwnProperty(in_port)) {
                return flow_parser.OF_ports[in_port];
            }
            return in_port;
        }

        function parse_eth_type (type) {
            if (flow_parser.eth_type.hasOwnProperty(type)) return flow_parser.eth_type[type];
            return "0x" + Number(type).toString(16);
        }
    }
};

var aux_devices_pool = ["adatabase", "adatabase2", "adatabase3",
                        "amacpro", "aimac", "aboss",
                        "aplanet128", "acloudapp128", "cisco1",
                        "www", "cloud3", "database",
                        "firewall", "firewall2", "hub",
                        "runos_vermont", "runos_cluster", "runos_controller"];

var Hash = function (name) {
    var hash = 0;
    if (name.length == 0) return hash;
    hash += name.charCodeAt(0)*100000;

    for (var i = 0, l = name.length; i < l; i++) {
        hash += name.charCodeAt(i);
    }

    return hash;
};
