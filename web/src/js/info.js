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
/* exported Info */ /* globals Info:true */

var Info = function () {
    var main,
        controller;

    return {
        init : init,
    };

    function init () {
        main = document.getElementsByTagName('main')[0];
        Server.ajax('GET', '/api/controller-manager/info', display);
    }

    function display (response) {
        var i, len, us, max = 0, html = '', uptime;
        controller = {
            address   : response && response.address   || '0.0.0.0',
            port      : response && response.port      || 0,
            threads   : response && response.nthreads  || 0,
            secure    : response && response.secure    || false,
            logLevel  : response && response.logLevel  || 'HIGH',
            started    : response && response.started   || 0,
        };
        uptime = Math.round(Date.now()/1000 - controller.started);
        window.console.info('uptime', uptime);

        html += '<h1>Info</h1>';
        html += '<h2>Run Options</h2>';
        html += '<ul>';
            html += '<li><u>address</u><i>'+  controller.address +'</i></li>';
            html += '<li><u>port</u><i>'+     controller.port +'</i></li>';
            html += '<li><u>threads</u><i>'+  controller.threads +'</i></li>';
            /*html += '<li><u>services</u>';
                for (i = 0, len = controller.services.length; i < len; ++i) {
                    html += '<b>'+ controller.services[i] +'</b>';
                }
            html += '</li>';
            html += '<li><u>applications</u>';
                for (i = 0, len = controller.apps.length; i < len; ++i) {
                    html += '<b>'+ controller.apps[i] +'</b>';
                }
            html += '</li>';*/
            html += '<li><u>log level</u><i>'+ controller.logLevel +'</i></li>';
        html += '</ul>';

        html += '<h2>Statistics</h2>';
        html += '<ul>';
            html += '<li><u>uptime</u><i>'+  uptime +' sec</i></li>';
            //html += '<li><u>switches</u><i>'+ controller.switchN +'</i></li>';
        html += '</ul>';

        main.innerHTML = html;
        us = main.querySelectorAll('u');
        for (i = 0, len = us.length; i < len; ++i) {
            if (us[i].offsetWidth > max) { max = us[i].offsetWidth; }
        }
        for (i = 0, len = us.length; i < len; ++i) {
            us[i].style.width = max + 'px';
        }
    }

}();


// @pink      : #DD1772;
// @darkblue  : #281A71;
// @lightblue : #0093DD;

// <h1>Info</h1>
// <h2>Run Options</h2>
// <p>port: 6633</p>
// <p>threads: 4</p>
// <p>core mask: 0xF</p>
// <p>services: device manual, link discovery, topology, forwarding</p>
// <p>apps: monitor, enterprise</p>
// <p>log level: high</p>
// <h2>Stastics</h2>
// <p>uptime: 1d 12h 47m 13s</p>
// <p>switches: 4</p>

// сводная информация о запуске контроллера:
//     параметры запуска контроллера
//         порт, на котором идет прослушивание (число);
//         число нитей (число);
//         привязка к ядрам процессора (маска, число в 16ричной системе счисления);
//         запущенные сервисные службы (список имен),
//         запущенные приложения (число);
//         степень логирования (строка LOW/MEDIUM/HIGH))
// статистика:
//     время работы: дни часы минуты секунды
//     количество подключенных свитчей
//     графики от времени (указаны только несколько в качестве примеров):
//         зависимость общего числа сообщения packet_in от времени
//         зависимость подключения свитчей от времени
//         круговой график активности приложений и сервисов контроллера
//             количество сообщений, которые они получают и отправку которых они инициируют
//     текущая загрузка ядер процессора в виде столбиков. Количество столбиков равно количеству ядер из маски.
