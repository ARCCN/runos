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
            html += '<li class="edit"><input type="text" value="' + hovered.name + '"></li>';
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
            html += '<li class="edit"><input type="text" value="' + hovered.name + '"></li>';
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
        UI.aux.querySelector('button.remove').onmouseout = hideRemoveRuleButton;
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
        UI.aux.querySelector('button.remove').onmouseout = hideRemoveRuleButton;
        UI.menu.style.display = 'none';
        UI.aux.style.display = 'block';
    }

    function showDetails () {
        var i, len, t,
            html = '',
            hovered = HCI.getHovered();
        html += '<h1>' + hovered.name + '</h1>';
        html += '<h2>' + hovered.ip + '</h2>';
        // html += '<h3>Информация</h3>';
        // html += '<p>количество пакетов, прошедших через это правило</p><p>суммарный размер в байтах этих пакетов</p>';
        html += '<h3>Network Switch Table</h3>';
        html += '<table>';
        html += '<thead><tr>';
            html += '<th>in_port</th>';
            html += '<th>mac_src</th>';
            html += '<th>mac_dst</th>';
            html += '<th>vlan_id</th>';
            html += '<th>vlan_prio</th>';
            html += '<th>ip_src</th>';
            html += '<th>ip_dst</th>';
            html += '<th>ip_proto</th>';
            html += '<th>ip_tos</th>';
            html += '<th>src_port</th>';
            html += '<th>dst_port</th>';
            html += '<th>action</th>';
        html += '</tr></thead>';
        html += '<tbody>';
            for (i = 0, len = hovered.routingRules.length; i < len; ++i) {
                t = hovered.routingRules[i];
                html += '<tr>';
                    html += '<td>' + (t.in_port   ? t.in_port   : '') + '</td>';
                    html += '<td>' + (t.mac_src   ? t.mac_src   : '') + '</td>';
                    html += '<td>' + (t.mac_dst   ? t.mac_dst   : '') + '</td>';
                    html += '<td>' + (t.vlan_id   ? t.vlan_id   : '') + '</td>';
                    html += '<td>' + (t.vlan_prio ? t.vlan_prio : '') + '</td>';
                    html += '<td>' + (t.ip_src    ? t.ip_src    : '') + '</td>';
                    html += '<td>' + (t.ip_dst    ? t.ip_dst    : '') + '</td>';
                    html += '<td>' + (t.ip_proto  ? t.ip_proto  : '') + '</td>';
                    html += '<td>' + (t.ip_tos    ? t.ip_tos    : '') + '</td>';
                    html += '<td>' + (t.src_port  ? t.src_port  : '') + '</td>';
                    html += '<td>' + (t.dst_port  ? t.dst_port  : '') + '</td>';
                    html += '<td>' + (t.action    ? t.action    : '') + '</td>';
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
        UI.aux.className = 'hostinfo';
        t = UI.aux.querySelectorAll('tbody tr');
        for (i = 0, len = t.length; i < len; ++i) {
            t[i].onmouseover = showRemoveRuleButton;
            t[i].onmouseout = hideRemoveRuleButton;
        }
        UI.aux.querySelector('button.add').onclick = showRowToAdd;
        UI.aux.querySelector('button.remove').onclick = removeRule;
        UI.aux.querySelector('button.remove').onmouseout = hideRemoveRuleButton;
        UI.menu.style.display = 'none';
        UI.aux.style.display = 'block';
    }

    function showRowToAdd () {
        var i, len,
            html = '',
            t = UI.aux.querySelector('tbody');
        html += '<tr>';
            if (UI.aux.className === 'hostinfo') {
                html += '<td><input type="text" placeholder="in_port"></td>';
                html += '<td><input type="text" placeholder="mac_src"></td>';
                html += '<td><input type="text" placeholder="mac_dst"></td>';
                html += '<td><input type="text" placeholder="vlan_id"></td>';
                html += '<td><input type="text" placeholder="vlan_pri"></td>';
                html += '<td><input type="text" placeholder="ip_src"></td>';
                html += '<td><input type="text" placeholder="ip_dst"></td>';
                html += '<td><input type="text" placeholder="ip_proto"></td>';
                html += '<td><input type="text" placeholder="ip_tos"></td>';
                html += '<td><input type="text" placeholder="src_port"></td>';
                html += '<td><input type="text" placeholder="dst_port"></td>';
                html += '<td><input type="text" placeholder="action"></td>';
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
        t = UI.aux.querySelectorAll('tbody tr');
        for (i = 0, len = t.length; i < len; ++i) {
            t[i].onmouseover = showRemoveRuleButton;
            t[i].onmouseout = hideRemoveRuleButton;
        }
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
                    HCI.getHovered().routingRules.splice(i, 1);
                } else if (UI.aux.className === 'firewall') {
                    HCI.getHovered().firewallRules.splice(i, 1);
                } else if (UI.aux.className === 'loadBalancer') {
                    HCI.getHovered().balancingRules.splice(i, 1);
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
