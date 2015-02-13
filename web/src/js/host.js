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

/* globals newHost:true */
newHost = function () {
    return function (_) {
        _ = _ || {};
        var host = {
            type: (_.icon === 'router') ? 'router' : 'host',
            id:   _.id   || 'DEFAULT',
            icon: _.icon || 'aimac',
            ip:   _.ip   || '',
            name: _.name || 'host',
            s:    _.s    || 64,
            x:    _.x    || Math.floor(Math.random() * 800),
            y:    _.y    || Math.floor(Math.random() * 400),
            color: 'white',
            isSelected: false,
            links: [],
            computeLoad: computeLoad,
            draw: draw,
            isOver: isOver,
        };

        if (host.type === 'router') {
            host.firewall     = false;
            host.loadBalancer = false;
            host.balancingRules = [];
            host.routingRules = [];
        }

        Net.add(host);
        return host;
    };

    function draw () {
        /* jshint validthis:true */
        var x, y, s, r;
        CTX.save();
        CTX.translate(this.x, this.y);
        CTX.beginPath();
        if (this.icon === 'router') {
            // <- DISPLAY BALANCING
            // <- DISPLAY FIREWALL
            CTX.fillStyle = this.color;
            CTX.arc(0, 0, this.s/2, 0, 2*Math.PI);
            CTX.fill();
        }
        CTX.drawImage(UI.icons[this.icon].img, -this.s/2, -this.s/2, this.s, this.s);
        if (this.isSelected) {
            CTX.lineWidth = 1;
            CTX.strokeStyle = Color.pink;
            CTX.strokeRect(-this.s/2, -this.s/2, this.s, this.s);
        }
        if (this.firewall || this.loadBalancer) {
            s = this.s/3;
            r = Math.sqrt(s/2*s/2 + s/2*s/2);
            x = -this.s/2 + r/2;
            y = -this.s/2 + r/2;
            CTX.font = s + 'px FontAwesome';
            CTX.fillStyle = Color.blue;
            CTX.strokeStyle = Color.dblue;
            CTX.lineWidth = 3;
            if (this.firewall) {
                CTX.beginPath();
                CTX.arc(x, y, r, 0, 2*Math.PI);
                CTX.fill();
                CTX.stroke();
            }
            if (this.loadBalancer) {
                CTX.beginPath();
                CTX.arc(x+this.s-r, y, r, 0, 2*Math.PI);
                CTX.fill();
                CTX.stroke();
            }
            CTX.fillStyle = 'white';
            CTX.textAlign = 'center';
            if (this.firewall)     { CTX.fillText('\uf132', x,          y+s/2-2); }
            if (this.loadBalancer) { CTX.fillText('\uf0ec', x+this.s-r, y+s/2-3); }
        }
        if (HCI.key.n) {
            CTX.font = '16px PT Sans Narrow';
            CTX.fillStyle = Color.darkblue;
            CTX.lineWidth = 1;
            CTX.strokeStyle = 'white';
            CTX.strokeText(this.name +' '+ this.ip, this.s/2, 0);
            CTX.fillText(this.name +' '+ this.ip, this.s/2, 0);
        }
        CTX.restore();
    }

    function computeLoad () {
        /* jshint validthis:true */
        var i, link;
        var load = 0;
        var bandwidth = 0;
        if (this.icon === 'router') {
            for (i = this.links.length-1; i >=0; --i) {
                link = this.links[i];
                load += link.load;
                bandwidth += link.bandwidth;
            }
            this.color = load/bandwidth > 0.9 ? '#DF3A01' :
                         load/bandwidth > 0.7 ? '#DF7401' :
                         load/bandwidth > 0.5 ? '#DBA901' :
                         load/bandwidth > 0.3 ? '#D7DF01' : '#A5DF00';
        }
    }

    function isOver (x, y, dx, dy) {
        /* jshint validthis:true */
        if (dx === undefined) {
            if (this.x-this.s/2 <= x && x <= this.x+this.s/2) {
                if (this.y-this.s/2 <= y && y <= this.y+this.s/2) {
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
}();
