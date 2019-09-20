/*

Copyright 2019 Applied Research Center for Computer Networks

*/

(function() {
    var logs = [],
        lastUpdate = 0,
        loopIntervalId,
        loopTime = 1000;
    
    var fields = {
        services : ['topology', 'runos'],
        types : ['info', 'warning', 'error']
    };

    function formatDate(date) {
        var dd = date.getDate();
        if (dd < 10) dd = '0' + dd;

        var mm = date.getMonth() + 1;
        if (mm < 10) mm = '0' + mm;

        var yy = date.getFullYear() % 100;
        if (yy < 10) yy = '0' + yy;

        var h = date.getHours();
        if (h < 10) h = '0' + h;

        var m = date.getMinutes();
        if (m < 10) m = '0' + m;

        var s = date.getSeconds();
        if (s < 10) s = '0' + s;

        return dd + '.' + mm + '.' + yy + ' ' + h + ':' + m + ':' + s;
    }

    var Log = function(data) {
        this.index = data.index || 0;
        this.date = data.date || undefined;
        this.type = data.type || 'info';
        this.message = data.message || '';
        this.service = data.service || 'runos';
    };
    
    function init() {

        return {
            run     : initLoop,
            stop    : breakLoop,
            get     : getLogs,
            fields  : fields,
            error   : error,
            info    : info,
            show    : showLogger
        };
    }

    function showLogger() {
        var html = '', eMenu,
            config = {
                fields : ['type', 'date', 'source', 'message'],
                services : {
                    'in-band': true,
                    'domain' : true,
                    'lag' : true,
                    'topology' : true,
                    'mirror' : true
                },
                types : {
                    'info': true,
                    'error' : true,
                    'warning':  true
                }
            },
            eLogger, eLoggerList,
            printLoopId, printIntervalId;

        html += '<div class="logger_wrapper">';
        html += generateFilterHtml();
        html += '<div class="logger">';
        html += '<ul class="logger--list">';
        html += '</ul>';
        html += '</div>';
        html += '</div>';

        eMenu = UI.createMenu(html, {
            unique: true,
            name: 'menu--logger',
            width: '600px',
            title: 'Logger',
            cl_callback : stopLogger
        });
        eLoggerList = eMenu.querySelector('.logger--list');
        eLogger = eMenu.querySelector('.logger');

        startLogger();
        initHandlers(eMenu, config);

        function startLogger() {
            var offset = 0,
                printLoopTime = 1000;

            var logData = Logger.get(offset, config);
            printMessages(logData.logs);
            offset = logData.offset;

            printIntervalId = setInterval(function() {
                var logData = Logger.get(offset, config),
                    logs = logData.logs,
                    curIndex = 0;

                offset = logData.offset;

                printLoopId && clearInterval(printLoopId);
                curIndex = 0;
                printLoopId = setInterval(function() {
                    if (curIndex == logs.length) {
                        clearInterval(printLoopId);
                        return;
                    }
                    printMessage(logs[curIndex]);
                    curIndex++;
                    if (eLogger.scrollHeight - eLogger.offsetHeight <  eLogger.scrollTop + 40) {
                        eLogger.scrollTop = Number.MAX_VALUE;
                    }
                }, printLoopTime/(logs.length + 1));
            }, printLoopTime);
        }
        function stopLogger() {
            clearInterval(printIntervalId);
            clearInterval(printLoopId);
        }
        function restartLogger() {
            stopLogger();
            clearLogger();
            startLogger();
        }
        function clearLogger() {
            eLoggerList.innerHTML = '';
        }
        function printMessages(logs) {
            for (var i = 0, len = logs.length; i < len; ++i) {
                printMessage(logs[i]);
            }
            eLogger.scrollTop = Number.MAX_VALUE;
        }
        function printMessage(log) {
            var typeColor = {
                warning : 'orange',
                error   : 'red',
                info    : 'white'
            };

            var li = document.createElement('li');

            li.innerHTML = generateMessage(log);
            li.style.color = typeColor[log.type];

            eLoggerList.appendChild(li);
        }
        function generateMessage(log) {
            var logText = '[ ' +
                '<span class="log-service">' + log.service  + ' | </span> ' +
                // '<span class="log-type" style="text-transform: uppercase">' + log.type[0]  + '</span> ' +
                '<span class="log-date">' + log.date  + '<span> ' +
                '] ' + log.message;
            return logText;
        }

        function initHandlers(eMenu, config) {
            var eCheckboxes = eMenu.querySelectorAll('.logger--checkbox'),
                eLoggerBody = eMenu.querySelector('.logger');

            eLoggerBody.addEventListener('mousedown', function(e) {
                e.stopPropagation();
            });
            for (var i = 0, len = eCheckboxes.length; i < len; ++i) {
                var eCheckbox = eCheckboxes[i];
                eCheckbox.addEventListener('change', function(e) {
                    var type = this.getAttribute('data-type'),
                        field = this.getAttribute('data-field');

                    config[type][field] = this.checked;
                    restartLogger();
                })
            }
        }

        function generateFilterHtml() {
            var services = Logger.fields.services,
                types = Logger.fields.types;
            var i, len;
            var html = '';
            html += '<div class="logger--filter">';
            html += '<div class="logger--column">';
            html += '<h3>Services</h3>';
            html += '<div class="logger--column">';
            for (i = 0, len = services.length; i < len; ++i) {
                var service = services[i];
                html += '<div class="logger--row">';
                html += '<label for="logger-' + service + '">' + service +  '</label>';
                html += '<input id="logger-' + service +
                    '" type="checkbox" checked data-type="services" data-field="' + service +
                    '" class="logger--checkbox">';
                html += '</div>';
            }
            html += '</div>';

            html += '</div>';
            html += '<div class="logger--column">';
            html += '<h3>Type</h3>';
            html += '<div class="logger--column">';
            for (i = 0, len = types.length; i < len; ++i) {
                var type = types[i];
                html += '<div class="logger--row">';
                html += '<label for="logger-' + type + '">' + type +  '</label>';
                html += '<input id="logger-' + type + '" type="checkbox" checked data-type="types" data-field="' + type + '" class="logger--checkbox">';
                html += '</div>';
            }
            html += '</div>';
            html += '</div>';

            return html;
        }
    }

    function getLogs(fromIndex, config) {
        fromIndex = fromIndex || 0;
        var result = [];
        for (var i = fromIndex, len = logs.length; i < len; ++i ) {
            var log = logs[i];
            if (isValid(log.service, config.services) &&
                isValid(log.type, config.types)) {
                result.push(log);
            }
        }
        return {
            logs : result,
            offset : logs.length
        };
    }
    function isValid(value, valids) {
        return valids[value];
    }
    
    var tempDate = 0;
    function getServerLogs() {
//        Server.ajax('GET', 'dummy', { lastUpdate: lastUpdate }, getServerLogsCallback);
        var data = {};
        data.logs = [];
        
        for (var i = 0, len = Math.floor(Math.random() * 4 + 2); i < len; ++i) {
            var log = {
                index : tempDate,
                date : Date(),
                type : ['info', 'error', 'warning'][Math.floor(Math.random() * 3)], 
                service : ['in-band', 'bridge-domain', 'lacp'][Math.floor(Math.random() * 3)], 
                message : 'Some message ' + tempDate
            };
            data.logs.push(log);
            tempDate += 1;
        }
        data.date = tempDate;
        getServerLogsCallback(data);
        
    }
    function getServerLogsCallback(data) {
        var newLogs = data.logs;
        lastUpdate = data.date;
        for (var i = 0, len = newLogs.length; i < len; ++i) {
            logs.push(new Log(newLogs[i]));
        }
    }

    function log(type, service, message) {
        var log = {
            index : 0,
            date : formatDate(new Date()),
            type : type,
            service : service,
            message : message
        };
        logs.push(log);
    }
    function info(service, message) {
        log('info', service, message);
    }
    function error(service, message) {
        log('error', service, message);
    }
    function initLoop() {
        if (loopIntervalId) {
            clearInterval(loopIntervalId);
        }
        loopIntervalId = setInterval(getServerLogs, loopTime);
    }
    function breakLoop() {
        clearInterval(loopIntervalId);
        loopIntervalId = null;
    }
    
    /* globals Logger:true */
    Logger = init();
})();
