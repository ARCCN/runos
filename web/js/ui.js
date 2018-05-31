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

/* globals UI:true */
UI = function () {
    return {
        centerX: false,
        centerY: false,
        icons: {},
        aux    : false,
        canvas : false,
        init        : init,
        showDetails : showDetails,
        showMenu    : showMenu,
        setStyle    : setStyle,
        addRule		: addRule,
    };

    function init () {
        UI.aux    = document.getElementsByClassName('aux')[0];
        UI.canvas = document.getElementsByClassName('map')[0];
        UI.menu   = document.getElementsByClassName('menu')[0];
        UI.menu.addEventListener('contextmenu', function(event) { event.preventDefault(); });
        UI.canvas.addEventListener('contextmenu', function(event) { event.preventDefault(); });
    }

    function showMenu (hovered, x, y) {
        var i, len, t,
            html = '';
        if (hovered.type === 'router') {
            html += '<li class="edit"><input type="text" value="' + hovered.name + '"><i class="off"></i></li>';
            // html += '<li>' + hovered.name + '</li>';
            html += '<li>' + hovered.ip + '</li>';
            html += '<li class="separator"></li>';
            html += '<li class="action settings">Router Settings</li>';
            html += '<li class="separator"></li>';
            html += '<li class="action firewall">Firewall<i class="' + (hovered.firewall ? 'on' : 'off') + '"></i></li>';
            html += '<li class="action loadBalancer">Load Balancer<i class="' + (hovered.loadBalancer ? 'on' : 'off') + '"></i></li>';
        } else if (hovered.type === 'link') {
            if (HCI.state.balanceLoad) {
                html += '<li class="action loadBalancer">Balance Load</li>';
            } else {
                if (hovered.load) {
                    html += '<li>Load: ' + (hovered.load / hovered.bandwidth * 100).toFixed(2) + '%</li>';
                }
                html += '<li>Bandwidth: ' + (hovered.bandwidth / 1e9) + 'Gb</li>';
                html += '<li class="action settings">Сhannel Settings</li>';
            }
        } else if (hovered.type === 'host') {
            html += '<li class="edit"><input type="text" value="' + hovered.name + '"><i class="off"></i></li>';
            // html += '<li>' + hovered.name + '</li>';
            html += '<li>' + hovered.ip + '</li>';
            html += '<li class="separator"></li>';
            html += '<li class="action settings">Host Settings</li>';
        }
        if (hovered.type !== 'link' && hovered.type !== 'route') {
            // html += '<li class="separator"></li>';
            // html += '<li class="action from">Route from here</li>';
        }
        UI.menu.innerHTML = html;
        UI.menu.switch_id = hovered.id;
        if (UI.menu.querySelector('.edit'))          { UI.menu.querySelector('.edit').onkeypress       = nameChanged; }
        if (UI.menu.querySelector('.settings'))      { UI.menu.querySelector('.settings').onclick      = showDetails; }
        if (UI.menu.querySelector('.delete'))        { UI.menu.querySelector('.delete').onclick        = deleteThis; }
        if (UI.menu.querySelector('.firewall'))      { UI.menu.querySelector('.firewall').onclick      = showFirewall; }
        if (UI.menu.querySelector('.loadBalancer'))  { UI.menu.querySelector('.loadBalancer').onclick  = showLoadBalancer; }
        t = UI.menu.querySelectorAll('.action i, .action.port');
        for (i = 0, len = t.length; i < len; ++i) {
            t[i].onclick = toggleFeature;
        }
        UI.menu.style.display = 'block';
        t = UI.menu.querySelector('input');
        if (t)  {
            t.onchange  = function () {
                var t = document.createElement('span');
                t.innerHTML = this.value;
                this.parentNode.parentNode.insertBefore(t, this.parentNode);
                this.style.width = (t.offsetWidth + 16) + 'px';
                this.parentNode.parentNode.removeChild(t);
                hovered.name = this.value;
                Net.save();
            };
            t.onkeyup   = t.onchange;
            t.onpaste   = t.onchange;
            t.onchange();
        }
        UI.menu.style.left = ((x + UI.menu.offsetWidth + UI.canvas.getBoundingClientRect().left < window.innerWidth) ? x : window.innerWidth - UI.menu.offsetWidth - UI.canvas.getBoundingClientRect().left) + 'px';
        UI.menu.style.top = ((y + UI.menu.offsetHeight + UI.canvas.getBoundingClientRect().top < window.innerHeight) ? y : window.innerHeight - UI.menu.offsetHeight - UI.canvas.getBoundingClientRect().top) + 'px';

        function deleteThis () {
            /* jshint validthis:true */
            UI.menu.style.display = 'none';
            Net.save();
        }

        function toggleFeature () {
            /* jshint validthis:true */
            var t,
                hovered = HCI.getHovered();
            if (this.className.indexOf('port') >= 0) {
                this.querySelector('i').onclick();
            } else {
                this.className = (this.className === 'on' ? 'off' : 'on');
                if (this.parentNode.className.indexOf('firewall') >= 0) {
                    hovered.firewall = !hovered.firewall;
                } else if (this.parentNode.className.indexOf('loadBalancer') >= 0) {
                    hovered.loadBalancer = !hovered.loadBalancer;
                }
            }
            event.stopPropagation();
        }

        function nameChanged () {
            var hovered = HCI.getHovered();
            this.querySelector('i').style.display = 'block';
            this.querySelector('i').className = 'on';
            this.querySelector('i').onclick = sendNewName;
        }

        function sendNewName () {
            var hovered = HCI.getHovered(),
                req = new Object();
            req.name = hovered.name;
            Server.ajax('PUT', 'api/webui/name/' + hovered.id, req);
            this.className = 'off';
            this.onclick = '';
            this.style.display = 'none';
        }
    }


    function showFirewall () {
        var i, len, t,
            html = '',
            hovered = HCI.getHovered();
        html += '<h1>' + hovered.name + '</h1>';
        html += '<h2>' + hovered.ip + '</h2>';
        html += '<h3>Firewall Settings</h3>';
        html += '<table>';
        html += '<thead><tr>';
            html += '<th>action</th>';
            html += '<th>src_ip</th>';
            html += '<th>proto</th>';
            html += '<th>dest_ip</th>';
            html += '<th>dest_port</th>';
        html += '</tr></thead>';
        html += '<tbody>';
            for (i = 0, len = hovered.firewallRules.length; i < len; ++i) {
                t = hovered.firewallRules[i];
                html += '<tr>';
                    html += '<td>' + (t.action    ? t.action    : '') + '</td>';
                    html += '<td>' + (t.src_ip    ? t.src_ip    : '') + '</td>';
                    html += '<td>' + (t.proto     ? t.proto     : '') + '</td>';
                    html += '<td>' + (t.dest_ip   ? t.dest_ip   : '') + '</td>';
                    html += '<td>' + (t.dest_port ? t.dest_port : '') + '</td>';
                html += '</tr>';
            }
        html += '</tbody>';
        html += '<tfoot>';
            html += '<tr><td>';
                html += '<button type="button" class="add">Add Rule</button>';
            html += '</td></tr>';
        html += '</tfoot>';
        html += '</table>';
        html += '<button type="button" class="remove"></button>';
        UI.aux.innerHTML = html;
        UI.aux.className = 'firewall';
        t = UI.aux.querySelectorAll('tbody tr');
        for (i = 0, len = t.length; i < len; ++i) {
            t[i].onmouseover = showRemoveRuleButton;
            t[i].onmouseout = hideRemoveRuleButton;
        }
        UI.aux.querySelector('button.add').onclick = showRowToAdd;
        UI.aux.querySelector('button.remove').onclick = removeRule;
        UI.menu.style.display = 'none';
        UI.aux.style.display = 'block';
    }

    function showLoadBalancer () {
        var i, len, t,
            html = '',
            hovered;
        if (HCI.state.balanceLoad) {
            hovered = HCI.state.balanceLoad;
        } else {
            hovered = HCI.getHovered();
        }
        html += '<h1>' + hovered.name + '</h1>';
        html += '<h2>' + hovered.ip + '</h2>';
        html += '<h3>Load Balancer Settings</h3>';
        html += '<table>';
        html += '<thead><tr>';
            html += '<th>protocol</th>';
            html += '<th>src_ips</th>';
            html += '<th>dest_ips</th>';
            html += '<th>dest_ports</th>';
        html += '</tr></thead>';
        html += '<tbody>';
            for (i = 0, len = hovered.balancingRules.length; i < len; ++i) {
                t = hovered.balancingRules[i];
                html += '<tr>';
                    html += '<td>' + (t.protocol   ? t.protocol   : '') + '</td>';
                    html += '<td>' + (t.src_ips    ? t.src_ips    : '') + '</td>';
                    html += '<td>' + (t.dest_ips   ? t.dest_ips   : '') + '</td>';
                    html += '<td>' + (t.dest_ports ? t.dest_ports : '') + '</td>';
                html += '</tr>';
            }
        html += '</tbody>';
        html += '<tfoot>';
            html += '<tr><td>';
                html += '<button type="button" class="add">Add Rule</button>';
            html += '</td></tr>';
        html += '</tfoot>';
        html += '</table>';
        html += '<button type="button" class="remove"></button>';
        UI.aux.innerHTML = html;
        UI.aux.className = 'loadBalancer';
        t = UI.aux.querySelectorAll('tbody tr');
        for (i = 0, len = t.length; i < len; ++i) {
            t[i].onmouseover = showRemoveRuleButton;
            t[i].onmouseout = hideRemoveRuleButton;
        }
        UI.aux.querySelector('button.add').onclick = showRowToAdd;
        UI.aux.querySelector('button.remove').onclick = removeRule;
        UI.menu.style.display = 'none';
        UI.aux.style.display = 'block';
    }

    function showDetails () {
        var i, len, t,
            html = '',
            hovered = HCI.getHovered();
        html += '<h1>' + hovered.name + '</h1>';
        html += '<h2>' + hovered.ip + '</h2>';
        html += '<h3>Network Switch Table</h3>';
        html += '<table>';
        html += '<thead><tr>';
            html += '<th width=5%>in_port</th>';
            html += '<th width=15%>mac_src</th>';
            html += '<th width=15%>mac_dst</th>';
            //html += '<th>vlan_id</th>';
            //html += '<th>vlan_prio</th>';
            html += '<th width=12%>ip_src</th>';
            html += '<th width=12%>ip_dst</th>';
            html += '<th width=5%>ip_proto</th>';
            html += '<th width=5%>ip_tos</th>';
            html += '<th width=5%>src_port</th>';
            html += '<th width=5%>dst_port</th>';
            html += '<th width=21%>action</th>';
        html += '</tr></thead>';
        html += '<tbody>';

        function fillRules(value, key, map) {
            t = value;
                html += '<tr>';
                    html += '<td width=5%>' + (t.in_port   ? t.in_port   : '') + '</td>';
                    html += '<td width=15%>' + (t.mac_src   ? t.mac_src   : '') + '</td>';
                    html += '<td width=15%>' + (t.mac_dst   ? t.mac_dst   : '') + '</td>';
                    //html += '<td>' + (t.vlan_id   ? t.vlan_id   : '') + '</td>';
                    //html += '<td>' + (t.vlan_prio ? t.vlan_prio : '') + '</td>';
                    html += '<td width=12%>' + (t.ip_src    ? t.ip_src    : '') + '</td>';
                    html += '<td width=12%>' + (t.ip_dst    ? t.ip_dst    : '') + '</td>';
                    html += '<td width=5%>' + (t.ip_proto  ? t.ip_proto  : '') + '</td>';
                    html += '<td width=5%>' + (t.ip_tos    ? t.ip_tos    : '') + '</td>';
                    html += '<td width=5%>' + (t.src_port  ? t.src_port  : '') + '</td>';
                    html += '<td width=5%>' + (t.dst_port  ? t.dst_port  : '') + '</td>';
                    html += '<td width=21%>' + (t.action    ? t.action    : '') + '</td>';
                html += '</tr>';
        }
        hovered.routingRules.forEach(fillRules);

        html += '</tbody>';
        html += '<tfoot>';
            html += '<tr><td>';
                html += '<button type="button" class="add">Add Rule</button>';
            html += '</td></tr>';
        html += '</tfoot>';
        html += '</table>';
        html += '<button type="button" class="remove"></button>';
        html += '<button type="button" class="new"></button>';
        UI.aux.innerHTML = html;
        UI.aux.className = 'hostinfo';
        t = UI.aux.querySelectorAll('tbody tr');
        for (i = 0, len = t.length; i < len; ++i) {
            t[i].onmouseover = showRemoveRuleButton;
            t[i].onmouseout = hideRemoveRuleButton;
        }
        UI.aux.querySelector('button.add').onclick = showRowToAdd;
        UI.aux.querySelector('button.remove').onclick = removeRule;
        UI.menu.style.display = 'none';
        UI.aux.style.display = 'block';
    }

    function addNewActionLine () {
        var new_line = document.getElementsByName("action_line")[0].cloneNode(true);
        document.getElementById("actions_container").appendChild(new_line);
        var last = document.getElementsByName("action_line").length - 1;
        var act = document.getElementsByName("action")[last];

        act.onchange = function () {
            if (act.value == "output") {
                document.getElementsByName("mod_field")[last].style.display = 'none';
                document.getElementsByName("mod_field_value")[last].style.display = 'none';
                document.getElementsByName("outport")[last].style.display = 'inline';
            }
            if (act.value == "set_field") {
                document.getElementsByName("outport")[last].style.display = 'none';
                document.getElementsByName("mod_field")[last].style.display = 'inline';
                document.getElementsByName("mod_field")[last].onchange = function () {
                    document.getElementsByName("mod_field_value")[last].style.display = 'inline';
                }
            }
            if (act.value == "drop") {
                document.getElementsByName("outport")[last].style.display = 'none';
                document.getElementsByName("mod_field")[last].style.display = 'none';
                document.getElementsByName("mod_field_value")[last].style.display = 'none';
            }
        };
        document.getElementsByName("action_line")[last].style.display = 'block';
    }

    function showRowToAdd () {
        var i, len,
            html = '',
            t = UI.aux.querySelector('tbody');
        html += '<tr>';
            if (UI.aux.className === 'hostinfo') {
                html += '<td width=5%><input type="text" placeholder="in_port"></td>';
                html += '<td width=15%><input type="text" placeholder="mac_src"></td>';
                html += '<td width=15%><input type="text" placeholder="mac_dst"></td>';
                //html += '<td><input type="text" placeholder="vlan_id"></td>';
                //html += '<td><input type="text" placeholder="vlan_pri"></td>';
                html += '<td width=12%><input type="text" placeholder="ip_src"></td>';
                html += '<td width=12%><input type="text" placeholder="ip_dst"></td>';
                html += '<td width=5%><input type="text" placeholder="ip_proto"></td>';
                html += '<td width=5%><input type="text" placeholder="ip_tos"></td>';
                html += '<td width=5%><input type="text" placeholder="src_port"></td>';
                html += '<td width=5%><input type="text" placeholder="dst_port"></td>';

                html += '<td width=21%><div id="actions_container" style="display:inline; position:relative">';
                    html += '<input type="submit" style="width:auto" id="add_new_line" value="New action...">';
                    html += '<div name="action_line" style="display:none; position:relative">';
                    html += '<select size="1" name="action">';
                        html += '<option disabled selected  > action </option>';
                        html += '<option value="output"> output </option>';
                        html += '<option value="set_field"> set_field </option>';
                        html += '<option value-"drop"> drop </option>';
                    html += '</select>';
                    html += '<input type="text" placeholder="port" name="outport" style="display:none; width:40px">';

                    html += '<select name="mod_field" style="display:none">';
                        html += '<option disabled selected  > field </option>';
                        html += '<option value="eth_src"> eth_src </option>';
                        html += '<option value="eth_dst"> eth_dst </option>';
                        html += '<option value="ip_src"> ip_src </option>';
                        html += '<option value="ip_dst"> ip_dst </option>';
                    html += '</select>';

                    html += '<input type="text" placeholder="value" name="mod_field_value" style="display:none; width:auto">';
                    html += '</div>';
                html += '</div></td>';

            } else if (UI.aux.className === 'firewall') {
                html += '<td><input type="text" placeholder="action"></td>';
                html += '<td><input type="text" placeholder="src_ip"></td>';
                html += '<td><input type="text" placeholder="proto"></td>';
                html += '<td><input type="text" placeholder="dest_ip"></td>';
                html += '<td><input type="text" placeholder="dest_port"></td>';
            } else if (UI.aux.className === 'loadBalancer') {
                html += '<td><input type="text" placeholder="protocol"></td>';
                html += '<td><input type="text" placeholder="src_ips"></td>';
                html += '<td><input type="text" placeholder="dest_ips"></td>';
                html += '<td><input type="text" placeholder="dest_ports"></td>';
            }
        html += '</tr>';
        t.innerHTML += html;

        document.getElementById("add_new_line").onclick = addNewActionLine;
        addNewActionLine();

        t = UI.aux.querySelectorAll('tbody tr');
        len = t.length;

        var button = UI.aux.querySelector('button.new');
        button.style.top = (t[len-1].getBoundingClientRect().top - UI.aux.getBoundingClientRect().top)-1 + 'px';
        button.style.left = '10px';
        button.style.display = 'block';
        button.onclick = addRule;

        var button = UI.aux.querySelector('button.remove');
        button.style.top = (t[len-1].getBoundingClientRect().top - UI.aux.getBoundingClientRect().top)-1 + 'px';
            button.style.left = (t[len-1].getBoundingClientRect().right - UI.aux.getBoundingClientRect().left) + 'px';
        button.style.display = 'block';

        var button = UI.aux.querySelector('button.add');
        button.style.display = 'none';
    }

    function addRule () {
        var rows = UI.aux.querySelectorAll('tbody tr'),
            i, row = rows[rows.length - 1],
            hovered = HCI.getHovered(),
            items = row.querySelectorAll('td');

        if (UI.aux.className == "hostinfo") {
            var mac_regex = /^([a-f0-9]{2}(:|-)){5}[a-f0-9]{2}$/ig;
            var ip_regex = /^(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$/g

            var in_port = items[0].querySelector('input').value,
                mac_src = items[1].querySelector('input').value.toLowerCase(),
                mac_dst = items[2].querySelector('input').value.toLowerCase(),
                //vlan_id = items[3].querySelector('input').value,
                //vlan_prio = items[4].querySelector('input').value,
                ip_src = items[3].querySelector('input').value,
                ip_dst = items[4].querySelector('input').value,
                ip_proto = items[5].querySelector('input').value,
                ip_tos = items[6].querySelector('input').value,
                src_port = items[7].querySelector('input').value,
                dst_port = items[8].querySelector('input').value,
                action = document.getElementsByName("action");

                // Validations
                if (!(in_port || mac_src || mac_dst || ip_src || ip_dst || ip_proto || ip_tos || src_port || dst_port || action)) {
                    return;
                }
                if (mac_src && mac_src.search(mac_regex) == -1) {
                    alert('Incorrect mac_src: ' + mac_src);
                    return;
                }
                if (mac_dst && mac_dst.search(mac_regex) == -1) {
                    alert('Incorrect mac_dst: ' + mac_dst);
                    return;
                }
                if (ip_src && ip_src.search(ip_regex) == -1) {
                    alert('Incorrect ip_src: ' + ip_src);
                    return;
                }
                if (ip_dst && ip_dst.search(ip_regex) == -1) {
                    alert('Incorrect ip_dst: ' + ip_dst);
                    return;
                }

                if (isNaN(Number(in_port))) {
                    alert('Incorrect in_port: ' + in_port);
                    return;
                }
                if (isNaN(Number(src_port))) {
                    alert('Incorrect src_port: ' + src_port);
                    return;
                }
                if (isNaN(Number(dst_port))) {
                    alert('Incorrect dst_port: ' + dst_port);
                    return;
                }

                // Parsing action
                var actions_list = new Object();
                for (var l = 0; l < action.length; l++) {
                    if (action[l].value == "output") {
                        var port = document.getElementsByName("outport")[l].value;
                        if (isNaN(Number(port))) {
                            alert('Incorrect value: ' + port);
                            return;
                        }
                        actions_list[action[l].value] = port;
                    }
                    else if (action[l].value == "set_field") {
                        var field = document.getElementsByName("mod_field")[l].value,
                            value = document.getElementsByName("mod_field_value")[l].value;

                            if ((field == "eth_src" || field == "eth_dst") && value.search(mac_regex) == -1) {
                                alert('Incorrect mac address: ' + value);
                                return;
                            }

                            if ((field == "ip_src" || field == "ip_dst") && value.search(ip_regex) == -1) {
                                alert('Incorrect ip address: ' + value);
                                return;
                            }
                            actions_list[field] = value;
                    }
                    else if (action[l].value == "drop") {
                        //actions_list.clear();
                        actions_list[action[l].value] = true;
                    }
                }

                // Display new flow entry to table
                items[0].innerHTML = in_port;
                items[1].innerHTML = mac_src;
                items[2].innerHTML = mac_dst;
                //items[3].innerHTML = vlan_id;
                //items[4].innerHTML = vlan_prio;
                items[3].innerHTML = ip_src;
                items[4].innerHTML = ip_dst;
                items[5].innerHTML = ip_proto;
                items[6].innerHTML = ip_tos;
                items[7].innerHTML = src_port;
                items[8].innerHTML = dst_port;
                items[9].innerHTML = "";
                for (var elem in actions_list) {
                    if (elem == "output") {
                        items[9].innerHTML += elem + ": " + actions_list[elem] + "<br>";
                    }
                    else if (elem == "drop") {
						items[9].innerHTML += "drop<br>";
					}
                    else {
                        items[9].innerHTML += "set_field: " + actions_list[elem] + " -> " + elem + "<br>";
                    }
                }

            // Forming new flow request and sending to the controller
            var req = new Object();
            req.in_port = Number(in_port);
            req.eth_src = mac_src;
            req.eth_dst = mac_dst;
            //req.vlan_id = vlan_id;
            //req.vlan_prio = vlan_prio;
            req.ip_src = ip_src;
            req.ip_dst = ip_dst;
            req.ip_proto = ip_proto;
            req.ip_tos = ip_tos;
            req.src_port = Number(src_port);
            req.dst_port = Number(dst_port);

            if (!isNaN(Number(actions_list["output"])))
                req.out_port = Number(actions_list["output"]);

            if (actions_list["eth_src"])
                req.modify_eth_src = actions_list["eth_src"];
            if (actions_list["eth_dst"])
                req.modify_eth_dst = actions_list["eth_dst"];
            if (actions_list["ip_src"])
                req.modify_ip_src = actions_list["ip_src"];
            if (actions_list["ip_dst"])
                req.modify_ip_dst = actions_list["ip_dst"];

            Server.ajax('POST', '/api/static-flow-pusher/newflow/' + UI.menu.switch_id, req);
        }

        UI.aux.querySelector('button.new').style.display = 'none';
        UI.aux.querySelector('button.remove').style.display = 'none';
        UI.aux.querySelector('button.add').style.display = 'block';
    }

    function removeRule () {
        /* jshint validthis:true */
        var i,len, t,
            rows = UI.aux.querySelectorAll('tbody tr'),
            rect = this.getBoundingClientRect();
        for (i = 0, len = rows.length; i < len; ++i) {
            t = rows[i].getBoundingClientRect();
            t = t.top + ((t.bottom - t.top) / 2);
            if (t < rect.bottom && t > rect.top) {
                if (UI.aux.className === 'hostinfo') {
                    var rulesEntries = HCI.getHovered().routingRules.entries();
                    for (var k = 0; k < i; k++)
                        rulesEntries.next();

                    var tmp = rulesEntries.next().value;
                    if (tmp) {
                        var flow_id = tmp[0],
                            switch_id = UI.menu.switch_id;
                        HCI.getHovered().routingRules.delete(flow_id);
                        Server.ajax('DELETE', '/api/flow-manager/' + switch_id + '/' + flow_id);
                    }
                    else {
                        UI.aux.querySelector('button.new').style.display = 'none';
                        UI.aux.querySelector('button.remove').style.display = 'none';
                        UI.aux.querySelector('button.add').style.display = 'block';
                    }
                    //HCI.getHovered().routingRules.splice(i, 1);
                } else if (UI.aux.className === 'firewall') {
                    //HCI.getHovered().firewallRules.splice(i, 1);
                } else if (UI.aux.className === 'loadBalancer') {
                    //HCI.getHovered().balancingRules.splice(i, 1);
                }
                rows[i].parentNode.removeChild(rows[i]);
                return;
            }
        }
    }

    function showRemoveRuleButton () {
        /* jshint validthis:true */
        var button = UI.aux.querySelector('button.remove');
        button.style.top = (this.getBoundingClientRect().top - UI.aux.getBoundingClientRect().top)-1 + 'px';
        button.style.left = (this.getBoundingClientRect().right - UI.aux.getBoundingClientRect().left) + 'px';
        button.style.display = 'block';
    }

    function hideRemoveRuleButton () {
        var button = UI.aux.querySelector('button.remove'),
            rect = button.getBoundingClientRect();
        if (event.fromElement === button) {
            if (event.y < rect.top || event.y > rect.bottom || event.x > rect.right) {
                button.style.display = 'none';
            }
        } else {
            if (event.y < rect.top || event.y > rect.bottom || event.x+1 < rect.left || event.x > rect.right) {
                button.style.display = 'none';
            }
        }
    }

    function setStyle (obj, prop, val) {
        obj.style[prop] = val;
        if (obj.style[prop].indexOf(val) < 0) { obj.style[prop] = '-webkit-' + val; }
        if (obj.style[prop].indexOf(val) < 0) { obj.style[prop] = '-moz-' + val; }
        if (obj.style[prop].indexOf(val) < 0) { obj.style[prop] = '-o-' + val; }
        if (obj.style[prop].indexOf(val) < 0) { window.console.error('Can not style', obj, prop, val); }
    }
}();
