/*

Copyright 2019 Applied Research Center for Computer Networks

*/

ProxySettings = function() {

    return {
        sendUiState : sendUiStateToServer
    };

    function sendUiStateToServer(cbSuccess, cbError) {
        Server.ajax('POST', '/api/v1/settings/ui_state', {data : getUiStateData()}, function(res) {
            if (cbSuccess) cbSuccess(res);
        }, function(err) {
            if (cbError) cbError(err);
        });
    }

    function getUiStateData() {
        var data = {};

        for (var i = 0, l = Net.hosts.length; i < l; i++) {
            var dev = Net.hosts[i];
            data["pos_" + dev.id + "_x"] = dev.x;
            data["pos_" + dev.id + "_y"] = dev.y;
            data[dev.id + "_name"] = dev.name;
        }

        return JSON.stringify(data);
    }
}();

