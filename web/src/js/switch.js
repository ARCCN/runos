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
/* exported Switch */ /* globals Switch:true */

var Switch = function () {
    var main,
        switches;

    return {
        init : init,
    };
    
    function init () {
        main = document.getElementsByTagName('main')[0];
        Server.ajax('GET', '/wm/core/controller/switches/json', setSwitches);
    }
    
    function setSwitches(response) {
        var html = '';
        switches = response;
        
        html += '<h1>Switches info</h1>';
        
        switches.forEach(function(item) {
            //html+= '<h2>Switch ' + item.dpid + '</h2>';
            html += '<br>';
            html += '<hr>';
            
            html += '<ul>';
            html += '<li><u>harole</u><i>'+     item.harole +'</i></li>';
            html += '<li><u>dpid</u><i>'+     item.dpid +'</i></li>';
            html += '<li><u>buffers</u><i>'+  item.buffers +'</i></li>';
            html += '<li><u>capabilities</u><i>'+ item.capabilities +'</i></li>';
            html += '<li><u>connected since</u><i>'+ item.connectedSince +'</i></li>';
            html += '<li><u>inet address</u><i>'+ item.inetAddress +'</i></li>';
            html += '</ul>';
        });
        
        main.innerHTML = html;
    }
    
}();
