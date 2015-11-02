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

/* globals HCI:true */
HCI = function () {
    var x0, y0;
    var x, y;
    var hovered;
    var state = {
        drag        : false,
        select      : false,
        balanceLoad : false,
        mousemove   : false
    };
    var key = {
        code: {
            'Enter':13,'Shift':16,'Ctrl':17,'Alt':18,'Esc':27,'Space':32,'Meta':91,
            'Left':37,'Up':38,'Right':39,'Down':40,
            '0':48,'1':49,'2':50,'3':51,'4':52,'5':53,'6':54,'7':55,'8':56,'9':57,
            'A':65,'B':66,'C':67,'D':68,'E':69,'F':70,'G':71,'H':72,'I':73,'J':74,'K':75,'L':76,'M':77,'N':78,'O':79,'P':80,'Q':81,'R':82,'S':83,'T':84,'U':85,'V':86,'W':87,'X':88,'Y':89,'Z':90,
            ';':186,'=':187,',':188,'-':189,'.':190,'/':191,'`':192,'[':219,'\\':220,']':221,'\'':222,
        }
    };

    return {
        state   : state,
        key     : key,
        getHovered : getHovered,
        init       : init,
        draw       : draw
    };

    function init() {
        x0 = UI.canvas.getBoundingClientRect().left;
        y0 = UI.canvas.getBoundingClientRect().top;
        window.onkeydown = onKeyDown;
        window.onkeyup   = onKeyUp;
        UI.canvas.onmouseup   = onMouseUp;
        UI.canvas.onmousedown = onMouseDown;
        UI.canvas.onmousemove = onMouseMove;
    }

    function onMouseDown (event) {
        x = event.clientX - x0;
        y = event.clientY - y0;
        UI.menu.style.display = 'none';
        if (event.button === 0) { key.mouse1 = true; }
        if (event.button === 2) { key.mouse2 = true; }
        hovered = hover(x, y);
        if (event.button === 0) {
            if (hovered) {
                if (hovered.type === 'host' || hovered.type === 'router') {
                    state.drag = { x: x, y: y };
                    UI.canvas.onmousemove = drag;
                    UI.setStyle(UI.canvas, 'cursor', 'grabbing');
                }
            } else {
                state.select = { x: x, y: y, dx: 0, dy: 0, any: false};
                UI.canvas.onmousemove = select;
            }
        } else if (event.button === 2) {
            event.preventDefault();
            clear();
            if (hovered) {
                UI.showMenu(hovered, x, y);
            }
        }
    }

    function onMouseUp (event) {
        if (state.drag) {
            state.drag = false;
            UI.canvas.onmousemove = onMouseMove;
            UI.canvas.style.cursor = 'auto';
            hovered = hover (event.clientX - x0, event.clientY - y0);
			var req = new Object();
			req.x_coord = hovered.x;
			req.y_coord = hovered.y;
	    	Server.ajax('PUT', 'api/webui/coord/' + hovered.id, req);
        } else if (state.select) {
            UI.canvas.onmousemove = onMouseMove;
            if (state.select.any) {
                state.select = false;
                return;
            }
            state.select = false;
        } 
        if (event.button === 0) {
            key.mouse1 = false;
            x = event.clientX;
            y = event.clientY;
        }
        if (event.button === 2) {
            key.mouse2 = false;
            UI.aux.style.display = 'none';
        }
        if (!key.ctrl) {
            clear();
        }
    }

    function onMouseMove (event) {
        var i, len, t,
            w, h,
            x = event.clientX - x0,
            y = event.clientY - y0;
        state.mousemove = true;
        UI.canvas.style.cursor = 'auto';
        state.mousemove = false;
        return;
    }

    function onKeyDown (event) {
        if (event.keyCode === key.code.N) {
            key.n = true;
        } else if (event.keyCode === key.code.G) {
            key.g = true;
        } else if (event.keyCode === key.code.Alt) {
            key.alt = true;
        } else if (event.keyCode === key.code.S) {
            key.s = true;
        } else if (event.keyCode === key.code.L) {
            key.l = true;
        } else if (event.keyCode === key.code.Ctrl) {
            key.ctrl = true;
        } else if (event.keyCode === key.code.Z) {
            key.z = true;
        } else if (event.keyCode === key.code.Esc) {
            clear();
            UI.menu.style.display = 'none';
            UI.aux.style.display = 'none';
        } else if (event.keyCode === key.code.Enter) {
            key.enter = true;
            if (UI.aux.style.display != 'none') {
                var button = UI.aux.querySelector('button.new');
                if (button.style.top && button.style.display != 'none') {
                    UI.addRule();
                }
            }
        }

    }

    function onKeyUp (event) {
        if (event.keyCode === key.code.N) {
            key.n = false;
        } else if (event.keyCode === key.code.Alt) {
            key.alt = false;
        } else if (event.keyCode === key.code.S) {
            key.s = false;
            clear();
        } else if (event.keyCode === key.code.G) {
            key.g = false;
        } else if (event.keyCode === key.code.Ctrl) {
            key.ctrl = false;
        } else if (event.keyCode === key.code.L) {
            key.l = false;
        } else if (event.keyCode === key.code.Z) {
            key.z = false;
        } else if (event.keyCode === key.code.Esc) {
            key.enter = false;
        }
    }

    function hover (x, y, dx, dy) {
        var i, len, j, t;
        if (arguments.length === 2) {
            for (i = Net.hosts.length-1; i >= 0; --i) {
                if (Net.hosts[i].isOver(x, y)) {
                    return Net.hosts[i];
                }
            }
            if (key.mouse2 || key.ctrl) {
                for (i = Net.links.length-1; i >= 0; --i) {
                    if (Net.links[i].isOver(x, y)) {
                        if (state.balanceLoad && state.balanceLoad.type === 'router') {
                            t = 0;
                            for (j = 0, len = state.balanceLoad.links.length; j < len; ++j) {
                                if (state.balanceLoad.links[j].isSelected) {
                                    t += 1;
                                }
                            }
                            if (t > 1) {
                                UI.showMenu(Net.links[i], x, y);
                            }
                        }
                        return Net.links[i];
                    }
                }
            }
            return false;
        } else {
            state.select.any = false;
            for (i = Net.hosts.length; i--; ) {
                Net.hosts[i].isSelected = false;
                if (Net.hosts[i].isOver(x, y, dx, dy)) {
                    state.select.any = true;
                }
            }
        }
    }

    function drag (event) {
        var i, host;
        var dx = event.clientX - x0 - state.drag.x;
        var dy = event.clientY - y0 - state.drag.y;
        state.drag.x += dx;
        state.drag.y += dy;
        for (i = Net.hosts.length-1; i >=0; --i) {
            host = Net.hosts[i];
            if (host.isSelected) {
                host.x += dx;
                host.y += dy;
                Net.save();
            }
        }
    }

    function select (event) {
        var dx = event.clientX - x0 - state.select.x;
        var dy = event.clientY - y0 - state.select.y;
        state.select.dx = dx;
        state.select.dy = dy;
        hover(state.select.x, state.select.y, state.select.dx, state.select.dy);
    }

    function clear () {
        var i;
        for (i = Net.hosts.length-1; i>=0; --i) {
            Net.hosts[i].isSelected = false;
        }
        for (i = Net.links.length-1; i>=0; --i) {
            Net.links[i].isSelected = false;
        }
        state.balanceLoad = false;
    }

    function draw (when) {
        if (when === 'before') {

        } else if (when === 'after') {
            if (state.select) {
                CTX.lineWidth = 1;
                CTX.strokeStyle = key.g ? Color.lightblue : Color.darkblue;
                CTX.strokeRect(state.select.x, state.select.y, state.select.dx, state.select.dy);
            }
        }
    }

    function getHovered () {
        return hovered;
    }
}();
