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
