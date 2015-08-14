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
		menu,
		clicked = false,
        switches;

    return {
        init : init,
    };

    function init () {
        main = document.getElementsByTagName('main')[0];
        menu = document.getElementsByClassName('menu')[0];
        Server.ajax('GET', '/api/switch-manager/switches/all', displaySwitches);
    }

    function displaySwitches(response) {
        var html = main.innerHTML;

		if (response.length > 0) {
			html += '<h2>Switch Info</h2>';
			html += '<table>';
			html += '<tr>';
				html += '<th>DPID</th>';
				html += '<th>Manufacturer description</th>';
				html += '<th>Hardware description</th>';
				html += '<th>Software description</th>';
				html += '<th>Ports</th>';
			html += '</tr>';

			var i, len;
            for (i = 0, len = response.length; i < len; ++i) {
				html += '<tr>';
					html += '<td>' + response[i].DPID + '</td>';
					html += '<td>' + response[i].mfr_desc + '</td>';
					html += '<td>' + response[i].hw_desc + '</td>';
					html += '<td>' + response[i].sw_desc + '</td>';
					
					var ports = '';
					var j, llen;
					for (j = 0, llen = response[i].ports.length; j < llen; ++j) {
						if (response[i].ports[j].portNumber > 0) {
							ports += response[i].ports[j].name + ' (' + response[i].ports[j].portNumber + ')';
							ports += '<br>';
						} /*else {
							// OFPP_LOCAL ports
							ports += response[i].ports[j].name;
							ports += '<br>';
						}*/
					}
								
					html += '<td>' + ports + '</td>';
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

    function showMenu(x, y) {
		var html = '';

		html += '<li class="action sw_info">Switch info</li>';
		html += '<li class="action port_info">Ports info</li>';
		html += '<li class="action flow_info">Flow table</li>';
		
		menu.innerHTML = html;
		menu.style.left = x + 'px';
        menu.style.top = y + 'px';
		menu.style.display = 'block';
						
		if (menu.querySelector('.sw_info'))      { menu.querySelector('.sw_info').onmousedown = showSwitchInfo; }
		if (menu.querySelector('.port_info'))      { menu.querySelector('.sw_info').onmousedown = showPortInfo; }
		if (menu.querySelector('.flow_info'))      { menu.querySelector('.sw_info').onmousedown = showFlowInfo; }
		
		main.onmousedown = onMouseDown;		
		clicked = true;		
	}
	
	function onMouseDown(event) {
		if (clicked) {
			menu.style.display = 'none';
			clicked = false;
			main.onmousedown = '';
		}
	}
	
	function showSwitchInfo() {
		
	}
	
	function showPortInfo() {
		
	}
	
	function showFlowInfo() {
		
	}

}();
