/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';

/* globals Recovery:true */
Recovery = function () {
    var aliases = {
        ID              : 'Controller ID',
        state           : 'State',
        is_Iam          : 'Is I am',
        mode            : 'Mode',
        role            : 'Role',
        is_Connection   : 'Connection',
        of_IP           : 'IP Address',
        of_port         : 'TCP Port',
        hb_status       : 'Status',
        hb_mode         : 'Mode',
        hb_IP           : 'IP Address',
        hb_port         : 'UDP Port',
        hb_interval     : 'Heartbeat Interval',
        hb_primaryDeadInterval  : 'Deadline Interval',
        hb_backupDeadInterval   : 'Backup Deadline Interval',
        db_IP           : 'IP Address',
        db_port         : 'TCP Port',
        db_type         : 'Type',
        is_dbConnection : 'Connection State'
    };
    var columnNames = [
        {
            title: 'Controller ID',
            names: 'ID'
        },
        {
            title: 'IP:Port',
            names: ['of_IP', 'of_port'],
            delimiter: ':'
        },
        {
            title: 'Mode',
            names: 'mode'
        },
        {
            title: 'Internal IP:Port',
            names: ['hb_IP', 'hb_port'],
            delimiter: ':'
        },
        {
            title: 'Data Store',
            names: ['db_IP', 'db_port'],
            delimiter: ':'
        },
        {
            title: 'Status',
            names: 'hb_status'
        },
        {
            title: 'State',
            names: 'state'
        }
    ];


    var dataBlocks = [
        {
            title: 'Controller Basic Settings',
            fields: ['ID', 'state', 'hb_status']
        },
        {
            title: 'OpenFlow Settings',
            fields: ['of_IP', 'of_port']
        },
        {
            title: 'Heartbeat Settings',
            fields: ['hb_IP', 'hb_port', 'hb_mode', 'hb_interval', 'hb_primaryDeadInterval', 'hb_backupDeadInterval']
        },
        {
            title: 'Controller Data Store(CDS)',
            fields: ['db_IP', 'db_port', 'db_type', 'is_dbConnection']
        }
    ];
    var changeBlocks = [
        {
            title: 'Modify Heartbeat settings',
            name: 'Heartbeat',
            command: 'changeHeartbeat',
            fields: [
                'hb_mode', 'hb_IP', 'hb_port', 'hb_interval',
                'hb_primaryDeadInterval', 'hb_backupDeadInterval'
            ]
        },
        {
            title: 'Modify Controller Role',
            name: 'Role',
            command: 'changeRole',
            fields: [
                'role'
            ]
        },
        {
            title: 'Modify CDS Settings',
            name: 'Database',
            command: 'changeDatabase',
            fields: [
                'db_type','db_IP','db_port'
            ]
        },
        {
            title: 'Modify Status',
            name: 'Status',
            command: 'changeStatus',
            fields: [
                'hb_status'
            ]
        },
        {
            title: 'Stop Heartbeat',
            name: 'Stop Heartbeat',
            command: 'stopHeartbeat',
            fields: [ ]
        }

    ];
    var fieldsType = {
        ID              : {
            type : 'text'
        },
        state           : {
            type : 'select',
            options : ['Active', 'Not active']
        },
        isAm            : {
            type : 'text'
        },
        hb_status       : {
            type : 'select',
            options : ['Backup', 'Primary']
        },
        role            : {
            type : 'select',
            options : ['MASTER', 'EQUAL', 'SLAVE']
        },
        is_Connection   : {
            type : 'checkbox'
        },
        of_IP           : {
            type : 'text'
        },
        of_port         : {
            type : 'text'
        },
        hb_mode         : {
            type : 'select',
            options : ['Broadcast', 'Unicast', 'Multicast']
        },
        hb_IP           : {
            type : 'text'
        },
        hb_port         : {
            type : 'text'
        },
        hb_interval     : {
            type : 'text'
        },
        hb_primaryDeadInterval  : {
            type : 'text'
        },
        hb_backupDeadInterval  : {
            type : 'text'
        },
        db_IP           : {
            type : 'text'
        },
        db_port         : {
            type : 'text'
        },
        db_type         : {
            type : 'select',
            options: ['InternalCDS', 'Redis']
        },
        is_dbConnection : {
            type : 'text'
        }
    };

    var data = {
        controllers: {},
        controllersData: '',
        columnNames : columnNames,
        dataBlocks : dataBlocks,
        changeBlocks: changeBlocks,
        fieldsType: fieldsType
    };

    // Recovery API
    return {
        init : init,
        post : post,
        onSubmit : onSubmit,

        getAlias : getAlias,
        getFieldType : getFieldType,
        getLastUpdate : getLastUpdate,

        remove : remove,
        removeSubMenus : removeSubMenus,
        getData : getData,
        getMastershipViewData: getMastershipViewData,
        eMenu : null,
        eSubMenus : {}
    };

    function init() {
        getServerData();

        setInterval(function() {
            getServerData();

        }, 1000);
    }
    function remove() {
        removeSubMenus();
        removeMenu();
    }
    function removeMenu() {
        Recovery.eMenu && Recovery.eMenu.remove();
        Recovery.eMenu = null;
    }
    function removeSubMenus() {
        for(var key in Recovery.eSubMenus) {
            if (!Recovery.eSubMenus.hasOwnProperty(key)) continue;
            Recovery.eSubMenus[key] && Recovery.eSubMenus[key].remove();
            delete(Recovery.eSubMenus[key]);
        }
    }
    function getServerData() {
        Server.ajax('GET', '/recovery/', response);
        function response(responseData) {
            var dataArray = responseData.array;
            normalizeData(dataArray);
            checkChangedData(dataArray);
            data.lastUpdate = getCurrentTime();
        }
    }

    function normalizeData(dataArray) {
        dataArray.forEach(function(data) {
            data['is_dbConnection'] = data['is_dbConnection'] == 'true';
            data['is_Iam'] = data['is_Iam'] == 'true';
            data['is_Connection'] = data['is_Connection'] == 'true';
        });
    }

    function checkChangedData(newData) {
        var newIds = [];
        Array.prototype.forEach.call(newData, function(controllerData) {
           newIds.push(controllerData['ID']);
        });

        for (var id in data.controllers) {
            if (newIds.indexOf(id) == -1) {
                UI.createHint('Controller ' + id + ' is down', {fail: true});
            }
        }
        Array.prototype.forEach.call(newData, function(controllerData) {
            if (!data.controllers[controllerData['ID']]) {

                if (controllerData['is_Iam']) {
                    UI.createHint('Controller ' + controllerData['ID'] + ' is up ');
                } else {
                    UI.createHint('Connection to controller ' + controllerData['ID'] + ' is up ');
                }

                if (controllerData['is_dbConnection'] && controllerData['is_Iam']) {
                    UI.createHint('Database ' + controllerData['db_IP'] + ':' + controllerData['db_port'] + ' connection is up');
                }

            } else {
                if (data.controllers[controllerData['ID']]['is_dbConnection'] != controllerData['is_dbConnection'] &&
                    controllerData['is_Iam']) {

                    if (controllerData['is_dbConnection']) {
                        UI.createHint('Database2 ' + controllerData['db_IP'] + ':' + controllerData['db_port'] + ' connection is up');
                    } else {
                        UI.createHint('Database ' + controllerData['db_IP'] + ':' + controllerData['db_port'] +  ' connection is down', {fail: true});
                    }

                }
            }
        });

        var newControllers = {};
        Array.prototype.forEach.call(newData, function(controllerData) {
            newControllers[controllerData['ID']] = controllerData;
        });
        data.controllers = newControllers;
    }

    function getCurrentTime() {
        var date = new Date();
        var options = {
            hour: 'numeric',
            minute: 'numeric',
            second: 'numeric'
        };
        return date.toLocaleString("ru", options);
    }

    function post(data, callback) {
        Server.ajax('PUT', '/recovery/', data, callback);
    }

    function getAlias(field) {
        return aliases[field] || field;
    }

    function getFieldType(field) {
        return fieldsType[field];
    }

    function onSubmit(e) {
        e.preventDefault();
        e.stopPropagation();
        var fields = {},
            eFields;

        eFields = this.querySelectorAll('.js-field');
        Array.prototype.forEach.call(eFields, function(eField) {
            if (eField.value == '') return;
            fields[eField.name] = eField.value;
        });
        post(fields, function() {
            removeSubMenus();
        });

    }

    function getData() {
        return data;
    }
    function getLastUpdate() {
        return data.lastUpdate;
    }

    function getMastershipViewData(cb) {
        Server.ajax('GET', '/masterview/', responseHandler);

        function responseHandler(response) {
            var switches = {},
                controllers = [];

            var data = response.array;
            for (var controller in data) {
                if (!data.hasOwnProperty(controller)) {
                    continue;
                }
                controllers.push(parseInt(controller));
                for (var controllerSwitch in data[controller]) {
                    if (!data[controller].hasOwnProperty(controllerSwitch)) {
                        continue;
                    }
                    if (switches[controllerSwitch]) {
                        switches[controllerSwitch][controller] = data[controller][controllerSwitch];
                    } else {
                        switches[controllerSwitch] = {};
                        switches[controllerSwitch][controller] = data[controller][controllerSwitch];
                    }
                }
            }
            controllers.sort(function(a, b) {
                return parseInt(a) - parseInt(b);
            });
            cb(controllers, switches);
        }
    }
}();
