/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';

/* globals newLink:true */
newLink = function () {
    var MaxBandwidth = 10e9;
    var MaxD = 5;

    return function (ex) {
        ex = ex || {};
        var link = {
            type      : 'link',
            id        : ex.id                     || 'DEFAULT',
            bandwidth : ex.bandwidth              || 0,
            host1     : Net.getHostByID(ex.host1) || false,
            host2     : Net.getHostByID(ex.host2) || false,
            host1_p   : ex.host1_p                || false,
            host2_p   : ex.host2_p                || false,
            to_host   : ex.to_host                || false,
            isSelected  : false,
            isRoute     : false, // for exact routes
            load        : false,
            draw            : draw,
            other           : other,
            isOver          : isOver,

            weight    : "-1",
            
            hint_up   : "",
            hint_down : "",
        };

        if (!link.host1 || !link.host2)
            return null;

        Net.add(link);
        link.host1.links.push(link);
        link.host2.links.push(link);
        var p1 = Net.getPort(link.host1, link.host1_p),
            p2 = Net.getPort(link.host2, link.host2_p);

        if (p1) {
            if (p1.link) Net.del(p1.link);
            p1.link = link;
            link.bandwidth = Math.max(link.bandwidth, p1.curr_speed); // TODO: what to do if ports have different curr_speed
        }
        if (p2) {
            if (p2.link) Net.del(p2.link);
            p2.link = link;
            link.bandwidth = Math.max(link.bandwidth, p2.curr_speed); // TODO: what to do if ports have different curr_speed
        }
        
        link.hint_up = "Link discovered: " + link.host1.id+":"+link.host1_p + " <--> " + link.host2.id+":"+link.host2_p;
        link.hint_down   = "Link broken: " + link.host1.id+":"+link.host1_p + " <--> " + link.host2.id+":"+link.host2_p;

        return link;
    };

    function draw () {
        /* jshint validthis:true */
        if (!this.host1 || !this.host2) {
            window.console.error('Empty link');
            return;
        }
        var port1 = this.host1.getPort(this.host1_p),
            port2 = this.host2.getPort(this.host2_p);
        if (!port1 )
            port1 = this.host1;
        if (!port2 )
            port2 = this.host2;
        if (!Net.settings.show_ports) {
            port1 = this.host1;
            port2 = this.host2;
        }

        CTX.save();

        var line_width_koef = 2.5;
        if (HCI.key.alt && HCI.key.t) {
            var x = (port1.x+port2.x)/2,
                y = (port1.y+port2.y)/2;

            CTX.textAlign = "center";
            CTX.textBaseline = "middle";
            CTX.font = Math.round(Net.settings.scale/4) + 'px PT Sans Narrow';
            CTX.fillStyle = Color.darkblue;
            CTX.lineWidth = 1;
            CTX.strokeStyle = 'white';
            CTX.strokeText(this.weight, x, y);
            CTX.fillText(this.weight, x, y);
            line_width_koef = 1;
        }

        CTX.beginPath();
        CTX.moveTo(port1.x, port1.y);
        CTX.lineTo(port2.x, port2.y);
        if (this.host1.status == 'down' || this.host2.status == 'down') {
            CTX.globalAlpha = 0.5;
        }
        if (this.isSelected) {
            CTX.lineWidth = 15;
            CTX.strokeStyle = Color.blue;
            CTX.stroke();
        }
        if (this.isRoute) {
            CTX.save();
            CTX.lineWidth = 9;
            CTX.strokeStyle = Color.blue;
            CTX.stroke();
            CTX.restore();
        }
        CTX.lineWidth = line_width_koef * Net.settings.scale / 64;
        CTX.strokeStyle = this.load ?
                          this.load/this.bandwidth > 0.9 ? '#DF3A01' :
                          this.load/this.bandwidth > 0.7 ? '#DF7401' :
                          this.load/this.bandwidth > 0.5 ? '#DBA901' :
                          this.load/this.bandwidth > 0.3 ? '#D7DF01' : '#A5DF00'
                          : 'black';

        if ((port1.isLAG) || (port2.isLAG)) {
            CTX.strokeStyle = Color.blue;
        }
        CTX.stroke();
        CTX.restore();
    }

    function other (host) {
        /* jshint validthis:true */
        return host === this.host1 ? this.host2 : this.host1;
    }

    function isOver (x, y) {
        /* jshint validthis:true */
        var port1 = this.host1.getPort(this.host1_p) || this.host1,
            port2 = this.host2.getPort(this.host2_p) || this.host2;
        var x1 = port1.x,
            x2 = port2.x,
            y1 = port1.y,
            y2 = port2.y,
            a = y1 - y2,
            b = x2 - x1,
            c = x1*(y2-y1) - y1*(x2-x1),
            d = Math.abs(a*x + b*y + c) / Math.sqrt(a*a + b*b);

        if (d < MaxD) {
            if ((x < MaxD+Math.max(x1, x2)) && (x > Math.min(x1, x2) - MaxD) && (y < MaxD+Math.max(y1, y2)) && (y > Math.min(y1, y2) - MaxD)) {
                this.isSelected = true;
                return true;
            }
        }
        return false;
    }
}();
