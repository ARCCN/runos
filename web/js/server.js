/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';
/* exported Server */ /* globals Server:true */

var Server = function () {
    var URL = '';

    return {
        ajax      : ajax,
        getCookie : getCookie,
        setCookie : setCookie
    };

    function getCookie(name) {
        name = name.replace(/%comma%/g, ',') ;
        var matches = document.cookie.match(new RegExp(
        "(?:^|; )" + name.replace(/([\.$?*|{}\(\)\[\]\\\/\+^])/g, '\\$1') + "=([^;]*)"
        ));
        return matches ? decodeURIComponent(matches[1]) : undefined;
    }

    function setCookie(name, value, options) {
        options = options || {};
        name = name.replace(/,/g, '%comma%') ;

        var expires = options.expires;

        if (typeof expires == "number" && expires) {
            var d = new Date();
            d.setTime(d.getTime() + expires * 1000);
            expires = options.expires = d;
        }
        if (expires && expires.toUTCString) {
            options.expires = expires.toUTCString();
        }

        value = encodeURIComponent(value);

        var updatedCookie = name + "=" + value;

        for (var propName in options) {
            updatedCookie += "; " + propName;
            var propValue = options[propName];
            if (propValue !== true) {
              updatedCookie += "=" + propValue;
            }
        }

        document.cookie = updatedCookie;
    }

    function ajax (type, api, data, callback, err_callback) {
        var past = Date.now(),
            request = new XMLHttpRequest();
        request.onreadystatechange = process;
        if (typeof data === 'function') { err_callback = callback; callback = data; data = null; }

        /* dummy */ if (api === 'dummy') { if (callback) callback(); return; }

        request.open(type, URL+api);
        if (type === 'POST' || type === 'PUT') { request.setRequestHeader('Content-Type', 'application/json'); }
        request.send(JSON.stringify(data));
        //window.console.info('sent', type, URL, api, data);

        function process () {
            var response;
            if (request.readyState === 4) {

                try {
                    response = JSON.parse(request.responseText);
                } catch (err) {
                    if (err_callback && typeof err_callback === 'function')
                        err_callback();
                    return;
                }

                if (request.status === 200) {
                    //window.console.info('recieved', (Date.now() - past)/1000, api, response);
                    if (callback) { callback(response); }
                } else {
                    //window.console.error('HTTP-ответ не 200. :-(');
                    //window.console.error(request.responseText);
                    //if (err_callback && request.status !== 502) { err_callback(request.responseText); }
                    if (request.status == 500 || request.status == 403 || request.status == 400 || request.status == 404) {
                        if (typeof err_callback === 'function')
                            err_callback(response);
                    }
                }
            }
        }
    }
}();
