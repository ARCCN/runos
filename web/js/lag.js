/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';

/* globals newLAG:true */
newLAG = function(ex) {

	var self = this;

	ex = ex || {};
    this.type = 'this';
    this.id = ex.id;
    this.name = ex.name || 'none';
    this.router = ex.router || false;
    this.ports = ex.ports || [];
    this.attachedPorts = [];
    this.isSelected = false;
    this.hint_up = "lag interface on ports [" + ex.ports + "] was created";
    this.hint_down = "lag interface on ports [" + ex.ports + "] was removed";

    /* Checks */
	if (typeof this.router === 'string') {
		this.router = Net.getHostByID(this.router);
	}
	if (!this.router) {
		window.console.error("Not selected router for this");
		return null;
	}
	if (this.router.lag[this.id]) {
		window.console.error("this " + this.id + " already exists", this.router.lag[this.id]);
		return null;
	}
    /* Checks end */

    Logger.info('lag', 'LAG interface [' + ex.ports + '] was created on switch ' + this.router.id);

    var lagPort = false;
	for (var i = 0, len = this.ports.length; i < len; i++) {
		var port = Net.getPort(this.router, this.ports[i]);
		if (port && port.of_port == this.id) {
			port.isLAG = true;
			lagPort = port;
		}
		else if (port) {
			port.isVisible = false;
		}
	}
	for (var i = 0, len = this.ports.length; i < len; i++) {
		var port = Net.getPort(this.router, this.ports[i]);
		port.lagPort = lagPort;
		port.lagPortStatus = false;
	}

	var port = Net.getPort(this.router, this.id);

	port.isLAG = true;
	this.router.lag[this.id] = this;

	this.timerId = setInterval(function() {
		if (!self.router.lag[self.id]) {
			clearInterval(this.timerId);
			return;
		}
		Server.ajax('GET', '/switches/' + self.router.id + '/lag-ready/' + self.id + '/', function(response) {
            self.attachedPorts = response.array;
        });
	}, 1000);
    
};

newLAG.prototype.addPort = function(port_no) {
    var self = this;
    Server.ajax("GET", "/switches/" + this.router.id + "/lag-ports/" +
        this.id + "/add-port/" + port_no + "/", callbackAddPort);

    function callbackAddPort(response) {
        if (1) { // if OK
            self.ports.push(port_no);
        }

        self.router.requestPorts();
    }
}
newLAG.prototype.updatePorts = function(ports) {
    if (ports.length < 2) {
        this.delLag();
        return;
    }
    var portsForDel = diff(this.ports, ports),
        portsForAdd = diff(ports, this.ports);

    this.delPorts(portsForDel);
    this.addPorts(portsForAdd);

    function diff(A, B) {
        var M = A.length, N = B.length, c = 0, C = [];
        for (var i = 0; i < M; i++)
        {
            var j = 0, k = 0;
            while (B[j] !== A[ i ] && j < N) j++;
            while (C[k] !== A[ i ] && k < c) k++;
            if (j == N && k == c) C[c++] = A[ i ];
        }
        return C;
    }
}
newLAG.prototype.delPorts = function(ports) {
    var self = this;
    Array.isArray(ports) && ports.forEach(function(port) {
        self.delPort(port);
    });
}

newLAG.prototype.delPort = function(port_no) {
    var self = this;
    Server.ajax("GET", "/switches/" + this.router.id + "/lag-ports/" +
        this.id + "/del-port/" + port_no + "/", callbackDelPort);

    function callbackDelPort(response) {
        if (1) { // if OK
            self.ports = self.ports.filter(function(port_in_arr) {
                return port_in_arr != port_no;
            });
        }
        self.removeLocalLag();
    }
}

newLAG.prototype.addPorts = function(ports) {
    var self = this;
    Array.isArray(ports) && ports.forEach(function(port) {
        self.addPort(port);
    });
}

newLAG.prototype.addPort = function(port_no) {
    var self = this;
    Server.ajax("GET", "/switches/" + this.router.id + "/lag-ports/" +
        this.id + "/add-port/" + port_no + "/", callback);

    function callback(response) {
        if (1) { // if OK
            self.ports = self.ports.filter(function(port_in_arr) {
                return port_in_arr != port_no;
            });
        }
        self.removeLocalLag();
    }
}


newLAG.prototype.isPortPortAttached = function(portId) {
    // return true if port is attached
    return !!~this.attachedPorts.indexOf(portId);
}

newLAG.prototype.delLag = function() {
    var self = this;
    Server.ajax("DELETE", '/switches/' + self.router.id + '/lag-ports/' + self.id + '/', function(response) {
        UI.createHint(self.hint_down);
        self.removeLocalLag();
    });
    UI.menu.style.display = 'none';
}

newLAG.prototype.removeLocalLag = function() {
    var router = this.router;
    delete this.router.lag[this.id];
    router.requestPorts();
    clearInterval(this.timerId);
}

newLAG.prototype.draw = function() {
    var p = Net.getPort(this.router, this.id);
    if (!p) {
        window.console.error("No port " + this.router.id + ":" + this.id);
        return;
    }
    p.draw();
}
