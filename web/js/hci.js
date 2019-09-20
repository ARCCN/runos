/*

Copyright 2019 Applied Research Center for Computer Networks

*/


'use strict';

/* globals HCI:true */
HCI = function () {
    var x0, y0;
    var x, y;
    var hovered;
    var mouse_move_objects = {
        object   : [],
        callback : [],
    };
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
        draw       : draw,
        addMMObject: addMMObject,
        clMMObjects: clMMObjects
    };

    function init() {
        x0 = UI.canvas.getBoundingClientRect().left;
        y0 = UI.canvas.getBoundingClientRect().top;
        window.onkeydown = onKeyDown;
        window.onkeyup   = onKeyUp;
        UI.canvas.onmouseup   = onMouseUp;
        UI.canvas.onmousedown = onMouseDown;
        UI.canvas.onmousemove = onMouseMove;
        window.onmousewheel = onWheel;
    }

    function onWheel (event) {
        event = event || window.event;
        if (!event.ctrlKey) return;
        var delta = event.deltaY || event.detail || event.wheelDelta;
        if (Net) {
            if (delta < 0 && Net.settings.scale < 128)
                Net.settings.scale += 8;
            else if (delta > 0 && Net.settings.scale > 32)
                Net.settings.scale -= 8;
            Net.need_draw = true;
        }
        event.preventDefault();
    }

    function onMouseDown (event) {
        var realX = event.pageX - x0,
            realY = event.pageY - y0;
        x = getCanvasX(realX);
        y = getCanvasY(realY);

        UI.menu.style.display = 'none';
        if (event.button === 0) { key.mouse1 = true; }
        if (event.button === 2) { key.mouse2 = true; }

        hovered = hover(x, y);
        if (event.button === 0) {
            if (hovered) {
                Net.need_draw = true;
                if (hovered.type === 'host' || hovered.type === 'router') {
                    hovered.static_pos = true;
                    state.drag = { x: x, y: y };
                    UI.canvas.onmousemove = drag;
                    UI.setStyle(UI.canvas, 'cursor', 'grabbing');
                }
                else if (hovered.type === 'port' && Net.new_domain) {
                    var domain = Net.getZygoteDomain();
                    if (domain && domain.mode == 'none') {
                        UI.createHint("Please first select type of the domain", {fail: true});
                    } else {
                        UI.showNewVlan(hovered, x, y);
                    }
                }
                else if (hovered.type === 'port' && Net.new_lag) {
                    UI.addPortToLag(hovered);
                }
                else if (hovered.type === 'port' && Net.new_mirror) {
                    UI.addPortToMirror(hovered);
                }
                else if (hovered.type === 'port' && Net.new_multicast_listeners) {
                    UI.addPortToMulticastListeners(hovered);
                }
                else if (hovered.type === 'port' && Net.new_query_ports) {
                    UI.addPortToMulticastGeneralQuery(hovered);
                }
                else if (hovered.type === 'port' && Net.import_queue && Net.import_queue.type == 'port') {
                    var ret = {};
                    ret["type"] = Net.import_queue.q_disc;
                    ret["queues"] = Net.import_queue.queues2;
                    Server.ajax('POST', '/switches/' + hovered.router.id + '/port/' + hovered.of_port + '/queues', ret, function (response) {
                        hovered.new_queues.length = 0;
                        hovered.export_queues.length = 0;
                        hovered.new_q_disc = false;
                        var html = 'Queues on port ' + hovered.name + '(' + hovered.of_port + ')' + ' exported!';
                        UI.createHint(html);
                        //Net.import_queue = {};
                    });

                }
                else if (hovered.type === "port" && document.body.querySelector(".queue_hint")) {
                    /*if (hovered.queues2.length > 0) {
                        Net.import_queue.export_queues.length = 0;
                        hovered.queues2.forEach(function (elem) {
                            Net.import_queue.export_queues.push(elem);
                        });
                    }
                    Net.import_queue.new_q_disc = hovered.q_disc;
                    clMMObjects();
                    UI.showQueuesDetails(true, Net.import_queue);*/
                }
            } else if (key.shift) {
                state.select = { x: x, y: y, dx: 0, dy: 0, any: false};
                UI.canvas.onmousemove = moveCanvas;
                UI.setStyle(UI.canvas, 'cursor', 'grabbing');
            } else {
                state.select = { x: x, y: y, dx: 0, dy: 0, any: false};
                UI.canvas.onmousemove = select;
            }
        } else if (event.button === 2) {
            event.preventDefault();
            clear();
            if (hovered) {
                UI.showContextMenu(hovered, realX, realY - (document.body.scrollTop || document.querySelector('html').scrollTop));
            }
        }
    }

    function onMouseUp (event) {
        var realX = event.pageX - x0,
            realY = event.pageY - y0;
        x = getCanvasX(realX);
        y = getCanvasY(realY);

        Net.need_draw = true;
        if (state.drag) {
            state.drag = false;
            UI.canvas.onmousemove = onMouseMove;
            UI.canvas.style.cursor = 'auto';
            hovered = hover(x, y);
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
            x = event.pageX;
            y = event.pageY;

            var mode = Routes.pathSelector.getMode();
            if (mode && hovered.type == 'router') {
                if (mode == hovered.selectionType)
                    Routes.pathSelector.delDpid(hovered.id);
                else
                    Routes.pathSelector.addDpid(hovered.id);
            }
        }
        if (event.button === 2) {
            key.mouse2 = false;
        }
        if (!event.ctrlKey) {
            clear();
        }
    }

    function onMouseMove (event) {
        var i, len, t,
            w, h,
            x = getCanvasX(event.pageX - x0),
            y = getCanvasY(event.pageY - y0);
        state.mousemove = true;
        UI.canvas.style.cursor = 'auto';
        state.mousemove = false;

        mouse_move_objects.object.forEach(function(obj, i) {
            if (obj.isOver(x, y))
                mouse_move_objects.callback[i]();
        });

        return;
    }

    function onKeyDown (event) {
        key.ctrl = event.ctrlKey;
        key.alt  = event.altKey;
        key.shift = event.shiftKey;
        if (event.keyCode === key.code.N) {
            key.n = true;
            Net.settings.show_names = !Net.settings.show_names;
        } else if (event.keyCode === key.code.G) {
            key.g = true;
        } else if (event.keyCode === key.code.S) {
            key.s = true;
        } else if (event.keyCode === key.code.L) {
            key.l = true;
        } else if (event.keyCode === key.code.Z) {
            key.z = true;
        } else if (event.keyCode === key.code.P) {
            key.p = true;
            if (key.shift && key.ctrl && key.alt)
                Net.settings.show_ports = !Net.settings.show_ports;
        } else if (event.keyCode === key.code.T) {
            key.t = true;
        } else if (event.keyCode === key.code.Esc) {
            clear();
            UI.menu.style.display = 'none';
            key.esc = true;
        } else if (event.keyCode === key.code.Enter) {
            key.enter = true;
        }
        Net.need_draw = true;
    }

    function onKeyUp (event) {
        key.ctrl = event.ctrlKey;
        key.alt  = event.altKey;
        key.shift = event.shiftKey;
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
        } else if (event.keyCode === key.code.P) {
            key.p = false;
        } else if (event.keyCode === key.code.T) {
            key.t = false;
        } else if (event.keyCode === key.code.Esc) {
            closeMenu();
            event.stopPropagation();
            key.esc = false;
        } else if (event.keyCode === key.code.Enter) {
            applyActionMenu();
            key.enter = false;
        }
        Net.need_draw = true;
    }

    function closeMenu () {
        if (!UI.activeMenuList.length) return;
        var active = UI.activeMenuList[0];
        if (active && active.querySelector('.close_menu'))
                active.querySelector('.close_menu').click();
    }

    function applyActionMenu () {
        if (!UI.activeMenuList.length) return;
        var active = UI.activeMenuList[0];
        if (active && active.querySelector('.blue'))
                active.querySelector('.blue').click();
    }

    function hover (x, y, dx, dy) {
        var i, len, j, t;

        if (arguments.length === 2) {
            for (i = Net.hosts.length-1; i >= 0; --i) {
                for (var k = Net.hosts[i].ports.length-1; k >= 0; --k) {
                    if (Net.hosts[i].ports[k].isOver(x, y)) {
                        return Net.hosts[i].ports[k];
                    }
                }

                if (Net.hosts[i].isOver(x, y)) {
                    return Net.hosts[i];
                }
            }
            for (i = Net.domains.length-1; i>=0; --i) {
                if (Net.domains[i].isOver(x,y))
                    return Net.domains[i];
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
                                UI.showContextMenu(Net.links[i], x, y);
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
        var dx = getCanvasX(event.pageX - x0) - state.drag.x;
        var dy = getCanvasY(event.pageY - y0) - state.drag.y;
        state.drag.x += dx;
        state.drag.y += dy;
        for (i = Net.hosts.length-1; i >=0; --i) {
            host = Net.hosts[i];
            if (host.isSelected) {
                host.x += dx;
                host.y += dy;
            }
        }
        Net.need_draw = true;
    }

    function select (event) {
        var dx = getCanvasX(event.pageX - x0) - state.select.x;
        var dy = getCanvasY(event.pageY - y0) - state.select.y;
        state.select.dx = dx;
        state.select.dy = dy;
        hover(state.select.x, state.select.y, state.select.dx, state.select.dy);
        Net.need_draw = true;
    }

    function moveCanvas (event) {
        var dx = getCanvasX(event.pageX - x0) - state.select.x;
        var dy = getCanvasY(event.pageY - y0) - state.select.y;

        Net.setCanvasTranslate(Net.settings.canvasTranslateX + dx, Net.settings.canvasTranslateY + dy);

        Net.need_draw = true;
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

    function addMMObject (obj, callback) {
        if (typeof(callback) != "function") // not function
            return;
        if (!obj.type) // not Net object
            return;

        mouse_move_objects.object.push(obj);
        mouse_move_objects.callback.push(callback);
    }

    function clMMObjects () {
        mouse_move_objects.object.length = 0;
        mouse_move_objects.callback.length = 0;
    }

    function getCanvasX(x) {
        return (x - Net.settings.canvasTranslateX) * (1/Net.settings.canvasScaleX);
    }

    function getCanvasY(y) {
        var scrollOffset = document.body.scrollTop || document.querySelector('html').scrollTop;
        return (y - scrollOffset - Net.settings.canvasTranslateY) * (1/Net.settings.canvasScaleY);
    }

}();
