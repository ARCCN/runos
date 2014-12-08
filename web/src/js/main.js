'use strict';

/* globals CTX:true */
(function () {
	var width;
	var height;
	var now;
	var then;

	window.addEventListener('resize', function() {
    		var width = document.body.clientWidth;
		var height = document.body.clientHeight;
		//alert(width + ' ' + height);
		//window.requestAnimationFrame(main);
	});

	document.addEventListener('DOMContentLoaded', function() {
        Server.ajax('GET', '/apps', Net.appList);
		if (document.body.className === 'enterprise') {
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
				now = Date.now();
				HCI.init();
				Net.init();
				// setTimeout(Net.save, 5000);
				onIconsLoaded(function() {
					window.requestAnimationFrame(main);
				});
			}
		} else if (document.body.className === 'info') {
			Info.init();
		} else if (document.body.className === 'switch') {
			Switch.init();
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
		}
	});

	function main () {
		then = now;
		now = Date.now();
		var	delta = (now - then) / 1000.0;

		update(delta);
		render();

		window.requestAnimationFrame(main);
	}

	function update (delta) {
		return delta;
	}

	function render () {
		CTX.fillStyle = '#FAFAFA';
		CTX.fillRect(0, 0, width, height);
		HCI.draw('before');
		Net.draw();
		HCI.draw('after');

	}

	function onIconsLoaded (callback) {
		var names = ['aboss', 'acloud128', 'acloud256', 'acloud512', 'acloudapp128', 'acloudapp256', 'acloudapp512', 'adatabase', 'adatabase2', 'adatabase3', 'adatabase4', 'adatabase5', 'aimac', 'amacair', 'amacblack', 'amacpro', 'amacwhite', 'amonitor', 'aplanet128', 'aplanet256', 'aplanet512', 'cloud', 'cloud2', 'cloud3', 'database', 'database2', 'firewall', 'firewall2', 'hub', 'hub2', 'lan', 'laptop', 'linux', 'linux2', 'mac', 'mac2', 'ok', 'ok2', 'pc', 'pc2', 'router', 'router2', 'server', 'server2', 'storage', 'storage2', 'sync', 'user', 'win', 'win2', 'workstation', 'workstation2', 'www', 'www2'];
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
}());
