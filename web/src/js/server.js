'use strict';
/* exported Server */ /* globals Server:true */

var Server = function () {
    var URL = '';

    return {
        ajax : ajax,
    };

    function ajax (type, api, data, callback) {
        var past = Date.now(),
            request = new XMLHttpRequest();
        request.onreadystatechange = process;
        if (typeof data === 'function') { callback = data; data = null; }

        /* dummy */ if (api === 'dummy') { if (callback) callback(); return; }

        request.open(type, URL+api);
        if (type === 'POST') { request.setRequestHeader('Content-Type', 'application/json'); }
        request.send(JSON.stringify(data));
        window.console.info('sent', type, URL, api, data);

        function process () {
            var response;
            if (request.readyState === 4) {
                if (request.status === 200) {
                    response = JSON.parse(request.responseText);
                    window.console.info('recieved', (Date.now() - past)/1000, api, response);
                    if (callback) { callback(response); }
                } else {
                    window.console.error('HTTP-ответ не 200. :-(');
                    Net.clear();
                }
            }
        }
    }
}();
