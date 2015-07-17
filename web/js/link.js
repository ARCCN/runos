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

/* globals newLink:true */
newLink = function () {
    var MaxBandwidth = 4e8;
    var MaxD = 5;

    return function (ex) {
        ex = ex || {};
        var link = {
            type      : 'link',
            id        : ex.id                     || 'DEFAULT',
            bandwidth : ex.bandwidth              || 2e8,
            host1     : Net.getHostByID(ex.host1) || false,
            host2     : Net.getHostByID(ex.host2) || false,
            host1_p   : ex.host1_p                || false,
            host2_p   : ex.host2_p                || false,
            isSelected  : false,
            load        : false,
            draw            : draw,
            other           : other,
            isOver          : isOver,
        };

        Net.add(link);
        link.host1.links.push(link);
        link.host2.links.push(link);
        return link;
    };

    function draw () {
        /* jshint validthis:true */
        if (!this.host1 || !this.host2) {
            window.console.error('Empty link');
            return;
        }
        CTX.save();
        CTX.beginPath();
        CTX.moveTo(this.host1.x, this.host1.y);
        CTX.lineTo(this.host2.x, this.host2.y);
        if (this.isSelected) {
            CTX.lineWidth = 15;
            CTX.strokeStyle = Color.blue;
            CTX.stroke();
        }
        CTX.lineWidth = this.bandwidth/MaxBandwidth*10;
        CTX.strokeStyle = this.load ?
                          this.load/this.bandwidth > 0.9 ? '#DF3A01' :
                          this.load/this.bandwidth > 0.7 ? '#DF7401' :
                          this.load/this.bandwidth > 0.5 ? '#DBA901' :
                          this.load/this.bandwidth > 0.3 ? '#D7DF01' : '#A5DF00'
                          : 'black';
        // CTX.lineWidth = this.load ? this.load/this.bandwidth*10 : 5;
        CTX.stroke();
        CTX.restore();
    }

    function other (host) {
        /* jshint validthis:true */
        return host === this.host1 ? this.host2 : this.host1;
    }

    function isOver (x, y) {
        /* jshint validthis:true */
        var x1 = this.host1.x,
            x2 = this.host2.x,
            y1 = this.host1.y,
            y2 = this.host2.y,
            a = y1 - y2,
            b = x2 - x1,
            c = x1*(y2-y1) - y1*(x2-x1),
            d = Math.abs(a*x + b*y + c) / Math.sqrt(a*a + b*b);

        if (d < MaxD) {
            if ((x < MaxD+Math.max(x1, x2)) && (x > Math.min(x1, x2) - MaxD) && (y < MaxD+Math.max(y1, y2)) && (y > Math.min(y1, y2) - MaxD)) {
                if (HCI.key.ctrl && !this.belongsToRouter()) {
                    return false;
                }
                this.isSelected = true;
                return true;
            }
        }
        return false;
    }
}();
