/*

Copyright 2019 Applied Research Center for Computer Networks

*/

/**
 * Create new mirror
 * @param obj
 * @returns {{id: string, cvlans: Array, svlans: Array, sgroups: Array, cgroups: Array, router: null, in_ports: Array, mirror_ports: Array, del: deleteMirror, hint_up: string}}
 */
newMirror = function(obj) {

    var mirror = {
        id          : obj.id || '',
        from        : obj.from || {},
        to          : obj.to || {},
        to_ctrl     : obj.to_ctrl || false,
        filename    : obj.filename || false,
        cvlans      : obj.cvlans || [],
        svlans      : obj.svlans || [],
        sgroups     : obj.sgroups || [],
        cgroups     : obj.cgroups || [],
        router      : obj.router || null,
        reserved    : obj.reserved || null,
        route_id    : obj.route_id || false,
        del         : deleteMirror,
        hint_up : 'New mirror created'
    };
    mirror.router && (mirror.router.mirrors[mirror.id] = mirror);

    Logger.info('mirror', 'Mirror was created on switch ' + mirror.router.id + ' vlans [' + mirror.svlans + '] -> ports [' + mirror.mirror_ports + ']');

    return mirror;

    function deleteMirror() {
        delete(mirror.router.mirrors[mirror.id]);
    }
};

staticMirror = (function() {

    var columns = [
        'id', 'dpid', 'time'
    ];
    var columnsAlias = {
        id : 'ID',
        dpid : 'dpid',
        time : 'time'
    };
    var logsUrl = '/pcap';

    return {
        showLogList : showLogList
    };

// log list functionality
    function showLogList() {
        var state = {
            data : [],
            filter : '',
            sort   : 'time',
            asc    : true,
            inputTimer : null
        };

        var elements = {
            menu  : null,
            input : null,
            body : null,
            headings : null
        };

        elements.menu = UI.createMenu(getLogListHtml(), {
            title : 'Your App',
            name   : 'mirror-log-list',
            unique: true,
            width: '400px'
        });
        if (!elements.menu) return;

        Server.ajax('GET', '/mirrors/dumps/', onResponse);

        function onResponse(response) {
            var logList = (response && response.array);
            state.data = parseLogListResponse(logList);

            elements.input = elements.menu.element('js-input');
            elements.body = elements.menu.element('js-body');
            elements.headings = elements.menu.querySelectorAll('.js-heading');

            elements.input.addEventListener('input', onInputFilterInput);
            Array.prototype.forEach.call(elements.headings, function(eHeading) {
                eHeading.addEventListener('click', onClickColumnHeading);
            });
            
            elements.menu.element('delete_dumps').onclick = onDeleteAll;

            generateBody();
        }

        function generateBody() {
            var data = state.data.filter(checkItemData).sort(sortFunc);

            updateBody(data);
        }

        function updateBody(data) {
            var html = '', i, n;

            for (i = 0, n = data.length; i < n; ++i) {
                html += rowTemplate(data[i]);
            }

            elements.body.innerHTML = html;
        }

        function rowTemplate(data) {
            var i, n,
                html = '';

            html += '<tr>';
            for (i = 0, n = columns.length; i < n; ++i) {
                switch(columns[i]) {
                    case 'time' :
                        html += '<td>' + formatTime(data[columns[i]]) + '</td>';
                        break;
                    default:
                        html += '<td>' + data[columns[i]] + '</td>';
                }
            }
            html += '<td width="30px"><a href="' + logsUrl + '/' + data.name + '" download><i class="download"></i></a></td>';
            html += '</tr>';

            return html;
        }

        function formatTime(time) {
            var date = new Date(time);

            return date.getDate() + '-' + date.getMonth() + '-' + date.getFullYear() + ' ' +
                    date.getHours() + ':' + date.getMinutes() + ':' + date.getSeconds();
        }

        function checkItemData(item) {
            var i, n,
                regExpColumn = /#([^:]*): *(.+)/gi,
                filter, column,
                temp;

            if (!state.filter) return true;

            temp = regExpColumn.exec(state.filter);
            if (temp) {
                column = temp[1];
                filter = temp[2];
            } else {
                filter = state.filter;
            }

            if (column) {
                return checkValue(item[column], column, filter);
            }
            for (i = 0, n = columns.length; i < n; ++i) {
                if (checkValue(item[columns[i]], columns[i], filter)) {
                    return true;
                }
            }

            return false;
        }
        function checkValue(value, type, template) {
            var regExp = new RegExp(template);
            return (type == 'time' && regExp.test(formatTime(value))) ||
                    (type != 'time' && regExp.test(value));
        }

        function sortFunc(a, b) {
            var valueFirst, valueSecond;
            switch(state.sort) {
                case 'time' :
                    valueFirst = new Date(a[state.sort]);
                    valueSecond = new Date(b[state.sort]);
                    break;

                default:
                    valueFirst = parseInt(a[state.sort], 10);
                    valueSecond = parseInt(b[state.sort], 10);
            }

            return state.asc ? valueFirst < valueSecond : valueSecond < valueFirst;
        }

    //    events
        function onClickColumnHeading() {
            var newSort = this.getAttribute('data-id');
            if (state.sort == newSort) {
                state.asc = !state.asc;
            } else {
                state.asc = true;
                state.sort = newSort;
            }

            generateBody();
        }
        function onInputFilterInput() {
            var me = this;

            clearTimeout(state.inputTimer);
            state.inputTimer = setTimeout(function() {
                state.filter = me.value;
                generateBody();
            }, 250);
        }
        function onDeleteAll() {
            Server.ajax("DELETE", '/mirrors/dumps/');
            elements.menu.remove();
        }
    //    events END
    }

    function parseLogListResponse(list) {
        var data = [],
            i, n;

        for (i = 0, n = list.length; i < n; ++i) {
            data.push(parseItem(list[i]));
        }

        return data;

        function parseItem(itemString) {
            var regExp = /([^_]+)_([^_]+)_([^_]+)/ig;
            var parsedString = regExp.exec(itemString);
            var date = parsedString[3].slice(0, parsedString[3].indexOf('.'));
            
            // Fuck people who create file with NOT ISO format
            var iso_date = date.substr(0, 4) + '-' + date.substr(4, 2) + '-' + date.substr(6, 2)
                        + date.substr(8, 3) + ':' + date.substr(11, 2) + ':' + date.substr(13, 2);

            return {
                name : itemString,
                dpid : parsedString[1],
                id   : parsedString[2],
                time : Date.parse(iso_date)
            }
        }
    }

    function getLogListHtml() {

    }

// end log list functionality

}());
