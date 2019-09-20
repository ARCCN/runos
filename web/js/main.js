/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';

/* globals CTX:true */
(function () {
    var width;
    var height;
    var start;
    var hotstart = true;

    window.addEventListener('resize', function() {
        var width = document.body.clientWidth;
        var height = document.body.clientHeight;
    });

    document.addEventListener('DOMContentLoaded', onDocumentContentLoaded);

    function main () {
        if (hotstart) {
            Net.need_draw = true;
            if (Date.now() - start > 5000)
                hotstart = false;
        }

        render();
        window.requestAnimationFrame(main);
    }

    function render () {
        if (Net.need_draw) {
            CTX.save();
            CTX.translate(Net.settings.canvasTranslateX || 0, Net.settings.canvasTranslateY || 0);

            CTX.fillStyle = '#FAFAFA';
            CTX.fillRect(-Net.settings.canvasTranslateX || 0, -Net.settings.canvasTranslateY || 0, width, height);
            CTX.scale(Net.settings.canvasScaleX || 1, Net.settings.canvasScaleY || 1);

            HCI.draw('before');
            Net.draw();
            HCI.draw('after');

            CTX.restore();
        }
    }

    function onIconsLoaded (callback) {
        var names = ['acloud512', 'aimac', 'router3', 'cisco3', 'cisco4', 'dr', 'atm', 'atm2', 'tv', 'camera'];
        names = names.concat(aux_devices_pool);
        var name;
        var i, q = names.length;
        for (i = 0; i < q; ++i) {
            name = names[i];
            UI.icons[name] = {};
            UI.icons[name].ready = false;
        }
        for (i = 0; i < q; ++i) {
            name = names[i];
            UI.icons[name].img = new Image();
            UI.icons[name].img.src = 'images/' + name + '.png';
            UI.icons[name].img.addEventListener('load', onLoad(name));
        }
        function onLoad (name) {
            UI.icons[name].ready = true;
            Net.need_draw = true;
            if (areAllReady()) {
                callback();
            }
        }
        function areAllReady () {
            var allReady = true;
            var i, q = names.length;
            for (i = 0; i < q; ++i) {
                name = names[i];
                if (!UI.icons[name].ready) {
                    allReady = false;
                }
            }
            return allReady;
        }
    }

    function onDocumentContentLoaded() {
        loadInitialUiState(initDocument);
    }

    function loadInitialUiState(cb) {
        Server.ajax('GET', '/api/v1/settings/ui_state', function(res) {
            if (res && res.data) {
                GlobalStore = JSON.parse(res.data);
            }
            isProxyUp = true;
            cb();
        }, function() {
            cb();
        });
    }

    function initDocument() {
        if (document.body.className === 'enterprise') {
            Recovery.init();
            UI.init();
            document.getElementsByTagName('html')[0].style.height = window.innerHeight - 10 + 'px';
            width = UI.canvas.parentNode.offsetWidth;
            height = UI.canvas.parentNode.offsetHeight;
            UI.centerX = width / 2;
            UI.centerY = height / 2;
            UI.canvas.width = width;
            UI.canvas.height = height;
            if (UI.canvas.getContext) {
                CTX = UI.canvas.getContext('2d');
                start = Date.now();
                HCI.init();
                Net.init();
                onIconsLoaded(function() {
                    window.requestAnimationFrame(main);
                });
            }
        } else if (document.body.className === 'start') {
            width = window.innerWidth;
            height = window.innerHeight;
            document.getElementsByTagName('html')[0].style.height = height + 'px';
            UI.canvas = document.getElementsByClassName('sign-in')[0];
            UI.canvas.style.left = width/2 - UI.canvas.getBoundingClientRect().width/2 + 'px';
            UI.canvas.style.top = height/2 - UI.canvas.getBoundingClientRect().height/2 + 'px';
            document.getElementsByTagName('button')[0].onclick = function() {
                window.location = 'topology.html';
            };
        } else if (document.body.className === 'info') {
            Info.init();
        }
    }
}());
