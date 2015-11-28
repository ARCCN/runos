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
/* exported Acl */ /* globals Acl:true */

var Acl = function () {
    var main, menu, clicked = false, rules;

    return {
        init : init,
    };

    function init () {
        main = document.getElementsByTagName('main')[0];
        menu = document.getElementsByClassName('menu')[0];
        Server.ajax('GET', '/api/acl/rules/all', displayRules);
    }

    function displayRules(response) {
        var html = main.innerHTML;
		if (response.length > 0) {
			html += '<h2>Rules Info</h2>';
			html += '<table>';
			html += '<tr>';
				html += '<th>id</th>';
				html += '<th>proto</th>';
				html += '<th>src_ip</th>';
				html += '<th>src_mac</th>';
				html += '<th>src_port</th>';
				html += '<th>dst_ip</th>';
                                html += '<th>dst_mac</th>';
                                html += '<th>dst_port</th>';
				html += '<th>max_flows</th>';
			html += '</tr>';

			var i, len;
            for (i = 0, len = response.length; i < len; ++i) {
				html += '<tr>';
					html += '<td>' + i + '</td>';
					html += '<td>' + response[i].proto + '</td>';
					html += '<td>' + response[i].src.ip + '</td>';
					html += '<td>' + response[i].src.mac + '</td>';
					html += '<td>' + response[i].src.port + '</td>';
					html += '<td>' + response[i].dst.ip + '</td>';
                                        html += '<td>' + response[i].dst.mac + '</td>';
                                        html += '<td>' + response[i].dst.port + '</td>';
					html += '<td>' + response[i].max_flows + '</td>';
				html += '</tr>';
			}
			main.innerHTML = html;
			menu = document.getElementsByClassName('menu')[0];
		}

		var table = document.getElementsByTagName('table')[0];
		table.addEventListener('contextmenu', function(e) {
			e.preventDefault();
			if (e.srcElement.localName == 'td')
				showMenu(e.clientX - table.getBoundingClientRect().left + 30, e.clientY);
		});
    }
}();
