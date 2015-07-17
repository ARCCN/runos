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
