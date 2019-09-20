/*

Copyright 2019 Applied Research Center for Computer Networks

*/

(function() {
'use strict';

var Path = function(initObj) {
    this.path = initObj.m_path;
    this.metrics = initObj.metrics;
    this.flapping = initObj.flapping;
    this.brokenFlag = (initObj.broken_flag == "true" ? true : false);
    this.dropThreshold = initObj.drop_threshold;
    this.utilThreshold = initObj.util_threshold;

    this.isShown = initObj.isShow || true;
    this.color = initObj.color || Routes.getRandomColor();
};

var PathSelection = (function() {
    var selector = {
            from : false,
            to   : false,
            mode : false, /// 'exact', 'include', 'exclude'
            exact   : [],
            include : [],
            exclude : []
    };

    return {
            clear      : clear,
            getMode    : getMode,
            setMode    : setMode,
            addDpid    : addDpid,
            delDpid    : delDpid,
            checkExact : checkExact,
            getExact   : getExact,
            getInclude : getInclude,
            getExclude : getExclude
    }

    function clear () {
        selector.mode    = false;
        selector.exact.length   = 0;
        selector.include.length = 0;
        selector.exclude.length = 0;

        Net.hosts.forEach(function(sw) {
            if (sw.type != "router") return;

            sw.selectionType = false;
            sw.links.forEach(function (link) {
                link.isRoute = false;
            });
        });
        Net.need_draw = true;
    }
    function clearDynamic () {
        selector.include.length = 0;
        selector.exclude.length = 0;

        Net.hosts.forEach(function(sw) {
            if (sw.type != "router") return;
            if (sw.selectionType != "exact")
                sw.selectionType = false;
        });
    }
    function clearStatic () {
        selector.exact.length   = 0;

        Net.hosts.forEach(function(sw) {
            if (sw.type != "router") return;
            if (sw.selectionType == "exact")
                sw.selectionType = false;
            sw.links.forEach(function (link) {
                link.isRoute = false;
            });
        });
    }
    // true - OK, false - bad dpid
    function verified (dpid) {
        if (!selector.mode) return false;

        if (selector.mode == "exact") {
            if (selector.exact.length == 0) { // first switch in route
                if (dpid != selector.from && dpid != selector.to)
                    return false;
            } else {
                var index = selector.exact.indexOf(dpid);
                if (index >= 0) return false; // cycle routing - DENY

                var f = selector.exact.indexOf(selector.from),
                    t = selector.exact.indexOf(selector.to);
                if (f >= 0 && t >= 0)
                    return false; // already from and to in route

                var prev_dpid = selector.exact[selector.exact.length-1];
                var prev = Net.getHostByID(prev_dpid);

                for (var i = 0, n = prev.links.length; i < n; i++) {
                    var link = prev.links[i];
                    if (link.host1.id == dpid || link.host2.id == dpid) { // found link
                        link.isRoute = true;
                        return true;
                    }
                }
                // if we are here, so we did not found link
                return false;
            }

        } else { // include or exclude
            if (dpid == selector.from || dpid == selector.to)
                return false;
            if (selector.mode == "include" && selector.include.length > 0) {
                UI.createHint("Please select only one 'include' switch!", {fail: true});
                return false;
            }
        }

        return true;
    }
    function getMode () {
        return selector.mode;
    }
    function setMode (m, from, to) {
        selector.from = from;
        selector.to = to;
        if (selector.mode == m) return;

        if (m == "exact") {
            clearDynamic();
        } else {
            clearStatic();
        }

        selector.mode = m;
        Net.need_draw = true;
    }
    function addDpid (dpid) {
        if (!selector.mode || !verified(dpid)) return;

        switch (selector.mode) {
            case "exact"   : addExact(dpid); break;
            case "include" : delExclude(dpid); addInclude(dpid); break;
            case "exclude" : delInclude(dpid); addExclude(dpid); break;
        }
    }
    function delDpid (dpid) {
        switch (selector.mode) {
            case "exact"   : delExact(dpid);   break;
            case "include" : delInclude(dpid); break;
            case "exclude" : delExclude(dpid); break;
        }
    }
    function addExact (dpid) {
        selector.exact.push(dpid);
        Net.getHostByID(dpid).selectionType = "exact";
    }
    function delExact (dpid) {
        var index = selector.exact.indexOf(dpid);
        if (index == selector.exact.length - 1) { // can delete only last switch
            var sw = Net.getHostByID(dpid);

            selector.exact.splice(index, 1);
            sw.links.forEach(function (link) {
                link.isRoute = false;
            });
            sw.selectionType = false;
        }
    }
    function addInclude (dpid) {
        selector.include.push(dpid);
        Net.getHostByID(dpid).selectionType = "include";
    }
    function delInclude (dpid) {
        var index = selector.include.indexOf(dpid);
        if (index >= 0) {
            selector.include.splice(index, 1);
            Net.getHostByID(dpid).selectionType = false;
        }
    }
    function addExclude (dpid) {
        selector.exclude.push(dpid);
        Net.getHostByID(dpid).selectionType = "exclude";
    }
    function delExclude (dpid) {
        var index = selector.exclude.indexOf(dpid);
        if (index >= 0) {
            selector.exclude.splice(index, 1);
            Net.getHostByID(dpid).selectionType = false;
        }
    }
    function checkExact () {
        if (selector.mode != "exact") return false;
        if (selector.exact.length > 1) {
            if (selector.exact[0] == selector.from && selector.exact[selector.exact.length-1] == selector.to ||
                selector.exact[0] == selector.to && selector.exact[selector.exact.length-1] == selector.from )
                return true;
        }
        return false;
    }
    function getExact () {
        return selector.exact;
    }
    function getInclude () {
        return selector.include;
    }
    function getExclude () {
        return selector.exclude;
    }
})();

var Route = function(initObj) {
    var route = this;
    this.service = initObj.service || 'unknown';
    this.id = initObj.id || null;
    this.sourceId = initObj.from || null;
    this.targetId = initObj.to || null;

    this.usedPath = parseInt(initObj.used_path) || 0;

    this.paths = [];
    if (initObj.paths) {
        var colors = Routes.getSavedColor(this.id);
        if (!colors) colors = [];
        for (var i = 0, l = initObj.paths.length; i < l; i++) {
            var path = initObj.paths[i];
            if (colors[i])
                path.color = colors[i];
            route.paths.push(new Path(path));
        }
        Routes.saveColors(this);
    }
    if (initObj.dynamic != "false") {
        this.dynamic = initObj.dynamic_settings;
    } else {
        this.dynamic = false;
    }
    this.predict = undefined;
};

Route.prototype.predictPath = function(ttl) {
    var route = this;
    Server.ajax("GET", '/routes/id/' + this.id + '/predict-path/', function (resp) {
        if (resp["error"]) {
            UI.createHint(resp["error"], {fail: true});
        } else {
            route.predict = { path: resp.m_path,
                              isShown : true,
                              color : Routes.getRandomColor() };
            setTimeout(function() { delete route.predict; Net.need_draw = true; }, ttl);
            Net.need_draw = true;
        }
    });
}

Route.prototype.addPath = function(initObj) {
    var path = new Path(initObj);
    this.paths.push(path);
    return path;
}

Route.prototype.deletePath = function(pathId) {
    return this.paths.splice(pathId, 1);
}

Route.prototype.drawPath = function(path) {
    if (!path.isShown) {
        return;
    }
    var dataLinkRoute = path.path,
        dataLinkRouteLength = dataLinkRoute.length,
        i = 0;

    if (dataLinkRouteLength < 2) {
        console.error('Path is not correct. length = ' + dataLinkRouteLength);
        return;
    }

    // check availability
    for (i = 0; i < dataLinkRouteLength ; ++i) {
        hostId = dataLinkRoute[i][0];
        portId = dataLinkRoute[i][1];
        host = Net.getHostByID(hostId);
        port = host.getPort(portId);
        if (!host || !port) {
            path.isShown = false;
            return;
        }
    }

    CTX.save();
    CTX.beginPath();

    var hostId = dataLinkRoute[0][0];
    var portId = dataLinkRoute[0][1];
    var host = Net.getHostByID(hostId);
    var port = host.getPort(portId);

    CTX.moveTo(port.x, port.y);

    for (i = 1; i < dataLinkRouteLength ; ++i) {

        hostId = dataLinkRoute[i][0];
        portId = dataLinkRoute[i][1];
        host = Net.getHostByID(hostId);
        port = host.getPort(portId);
        CTX.lineTo(port.x, port.y);

    }

    CTX.lineWidth = 10 * Net.settings.scale / 64;
    CTX.globalAlpha = 0.6;
    CTX.strokeStyle = path.color;

    CTX.stroke();
    CTX.restore();
}

Route.prototype.draw = function() {
    this.paths.forEach(this.drawPath);
    if (this.predict) {
        this.drawPath(this.predict);
    }
}
Route.prototype.show = function() {
    Array.prototype.forEach.call(this.paths, function(path) {
        path.isShown = true;
    });
}
Route.prototype.toggle = function(pathId) {
    if (this.paths.length > pathId) {
        this.paths[pathId].isShown = !this.paths[pathId].isShown;
    }
}
Route.prototype.hide = function() {
    Array.prototype.forEach.call(this.paths, function(path) {
        path.isShown = false;
    });
}

// TODO Надо с прототипами переделать
var InBandRoutes = (function() {
    var service = 'in-band';
    return {
        getRoutesByDpid : getRoutesByDpid,
        draw : draw,
        show : show,
        hide : hide,
        del  : del
    }

    function getRoutesByDpid(dpid, callback) {
        Server.ajax('GET', '/in_band/routes/' + dpid + '/', parseInBandRoutesResponse);

        function parseInBandRoutesResponse(response) {
            var routesByHost = response['routes'];
            if (!Array.isArray(routesByHost)) {
                callback({});
                return;
            }

            if (routesByHost[0] == routesByHost[1]) {
                Server.ajax('GET',
                            '/routes/id/' + routesByHost[0] + '/',
                            parseResponseForIndenticalRouteId);
            } else {
                getRoutesAjax(routesByHost);
            }
        }

        function parseResponseForIndenticalRouteId(response) {
            response.service = service;
            var route = new Route(response);
            route.paths[0].color = Color.green;
            if (route.paths.length > 1) // if backup exists
                route.paths[1].color = Color.red;
            if (Routes.routes[service][dpid]) {
                delete(Routes.routes[service][dpid]);
            }
            Routes.routes[service][dpid] = route;
            callback(route);
        }

        function getRoutesAjax(routesByHost) {
            var i = 0;
            Server.ajax('GET',
                    '/routes/id/' + routesByHost[0] + '/',
                    parseResponeRoutes('main', routesByHost[0]));
            Server.ajax('GET',
                    '/routes/id/' + routesByHost[1] + '/',
                    parseResponeRoutes('alt', routesByHost[1]));

            var mainPath, altPath;

            function parseResponeRoutes(type, id) {

                return function(response) {
                    i++;
                    if (type == 'main') {
                        mainPath = new Path(response.paths[0]);
                        mainPath.color = Color.green;
                    } else if (type == 'alt') {
                        altPath = new Path(response.paths[0]);
                        altPath.color = Color.red;
                    }
                    if (i == 2) {
                        var route = new Route({
                            service : service
                        });
                        route.paths.push(mainPath);
                        route.paths.push(altPath);

                        if (Routes.routes[service][dpid]) {
                            delete(Routes.routes[service][dpid]);
                        }
                        Routes.routes[service][dpid] = route;
                        callback(route);
                    }
                }
            }

        }
    }

    function getRoutes(callback) {

    }

    function updateRoutes() {

    }
    function draw() {
        for (var route in Routes.routes['in-band']) {
            Routes.routes['in-band'][route].draw();
        }
    }
    function show() {
        for (var route in Routes.routes['in-band']) {
            Routes.routes['in-band'][route].show();
        }
    }
    function hide() {
        for (var route in Routes.routes['in-band']) {
            Routes.routes['in-band'][route].hide();
        }
    }
    function del() {

    }
})();

var DomainRoutes = (function() {
    var service = 'bridge-domain';
    return {
        getRoutesByName : getRoutesByName,
        show   : show,
        draw   : draw,
        hide   : hide,
        update : update,
        del    : del
    }

    function getRoutesByName(name, callback) {
        var routes = Routes.routes[service][name];
        /*if (routes) { // FIXME: not save now, need update method!
            for (var i = 0, n = routes.length; i < n; ++i) {
                routes[i].show();
            }
            callback(routes);
            return;
        }*/
        routes = Routes.routes[service][name] = [];

        var domain = Net.getDomainByName(name),
            routesId,
            nCompleteRequest = 0,
            nRoutes = 0;

        if (!domain) {
            console.error('Domain with name ' + name + ' not found');
        }

        routesId = domain.routesId;
        nRoutes = routesId.length;

        for (var i = 0; i < nRoutes; ++i) {
            Server.ajax('GET', '/routes/id/' + routesId[i] + '/', updateRoute);
        }

        function updateRoute(response) {
            response.service = service;
            var route = new Route(response);
            routes.push(route);
            nCompleteRequest++;
            if (nCompleteRequest == nRoutes && callback) {
                callback(routes);
            }
        }
    }

    function draw() {
        var domains = Routes.routes[service],
            routes;
        for (var domain in domains) {
            routes = domains[domain];
            for (var i = 0, n = routes.length; i < n; ++i) {
                routes[i].draw();
            }
        }
    }
    function show() {

    }
    function hide() {
        var domains = Routes.routes[service],
            routes;
        for (var domain in domains) {
            routes = domains[domain];
            for (var i = 0, n = routes.length; i < n; ++i) {
                routes[i].hide();
            }
        }
    }
    function update(name, callback) {
        var routes = Routes.routes[service][name];
        if (routes) {
            for (var i = 0, n = routes.length; i < n; ++i) {
                routes[i].hide();
            }
            delete (Routes.routes[service][name]);
        }

        getRoutesByName(name, callback);
    }
    function del(name) {
        var routes = Routes.routes[service][name];
        if (routes) {
            for (var i = 0, n = routes.length; i < n; ++i) {
                routes[i].hide();
            }
            delete(Routes.routes[service][name]);
        }
    }

})();

var MirrorRoutes = (function() {
    var service = 'mirror';
    return {
        getRoutesById : getRoutesById,
        show   : show,
        draw   : draw,
        hide   : hide,
    }

    function getRoutesById(mirror_id, route_id, callback) {
        var routes = Routes.routes[service][mirror_id];
        routes = Routes.routes[service][mirror_id] = [];
        Server.ajax('GET', '/routes/id/' + route_id + '/', updateRoute);

        function updateRoute(response) {
            response.service = service;
            var route = new Route(response);
            routes.push(route);
            if (callback)
                callback(route);
        }
    }

    function draw() {
        var mirrors = Routes.routes[service],
            routes;
        for (var mirror in mirrors) {
            routes = mirrors[mirror];
            for (var i = 0, n = routes.length; i < n; ++i) {
                routes[i].draw();
            }
        }
    }
    function show() {
    }
    function hide() {
        var mirrors = Routes.routes[service],
            routes;
        for (var mirror in mirrors) {
            routes = mirrors[mirror];
            for (var i = 0, n = routes.length; i < n; ++i) {
                routes[i].hide();
            }
        }
    }
})();

/* globals Routes:true */
Routes = (function() {
    var MetricsTypes = [
        'Manual',
        'Hop',
        'PortSpeed',
        'PortLoading'
    ];

    var services = ['in-band', 'multicast', 'bridge-domain', 'mirror'];

    var routes = {
        'in-band'       : {},
        'multicast'     : {},
        'bridge-domain' : {},
        'mirror'        : {},
        'all'           : {}
    };

    var savedColors = {};

    var offsetColor = 30,
        colorShift = 0;


    return {
        api        : {
            getRoutes  : {
                inBand : {
                    byDpid  : InBandRoutes.getRoutesByDpid,
                    all     : InBandRoutes.getRoutes
                },
                domain : {
                    byDomainName : DomainRoutes.getRoutesByName
                },
                mirror : {
                    byRouteId : MirrorRoutes.getRoutesById
                },
                all     : getAllRoutes
            }
        },
        routes : routes,
        services : services,
        MetricsTypes : MetricsTypes,
        savedColors : savedColors,

        getRouteById : getRouteById,
        getDomainRouteById : getDomainRouteById,
        getAllRoutesByFilter : getAllRoutesByFilter,
        getRandomColor : getRandomColor,
        getSavedColor : getSavedColor,

        delAllDomainRoutes : delAllDomainRoutes,
        updateAllDomainRoutes : updateAllDomainRoutes,
        saveColors : saveColors,

        draw   : draw,
        hide   : hide,
        show   : show,
        del    : del,

        pathSelector : {
            clear      : PathSelection.clear,
            getMode    : PathSelection.getMode,
            setMode    : PathSelection.setMode,
            addDpid    : PathSelection.addDpid,
            delDpid    : PathSelection.delDpid,
            checkExact : PathSelection.checkExact,
            getExact   : PathSelection.getExact,
            getInclude : PathSelection.getInclude,
            getExclude : PathSelection.getExclude
        }
    }
    /* TODO Dump routes. Не возращает paths
    function getAllRoutes() {
        Server.ajax('GET', '/dump/topology/', parseRoutesResponse);
        function parseRoutesResponse(response) {
            if (!Array.isArray(response)) {
                console.error('getAllroutes response is not array');
                return;
            }
            var routes = response.array;
            for (var i = 0, len = response._size; i < len; ++i) {
                makeRoute(routes[i]);
            }
        }

        function makeRoute(route) {
            var routeObj = {
                id :
            }
        }
    }
    */
    function getAllRoutes(callback) {
        Routes.del('all');
        Server.ajax('GET', '/routes/service/all/', parseRoutesResponse);
        function parseRoutesResponse(response) {
            var routes = response.array;
            for (var i = 0, len = response._size; i < len; ++i) {
                makeRoute(routes[i]);
            }
            callback();
        }

        function makeRoute(route) {
            var id = route.id;
            var routeObj = {
                id : id,
                service : route.service,
                main : {
                    path : route.main_route
                },
                alt : {
                    path : route.alt_route
                }
            }

            routes.all[id] = new Route(routeObj);
        }
    }

    function getRouteById(id) {
        return routes.all[id] || null;
    }

    function getDomainRouteById(name, id) {
        var domainRoutes = routes['bridge-domain'][name];
        for (var i = 0, n = domainRoutes.length; i < n; ++i) {
            if (domainRoutes[i].id === id) {
                return domainRoutes[i];
            }
        }
        return null;
    }

    function getAllRoutesByFilter(filters) {
        var buf = routes.all;
        if (!filters) {
            return routes.all;
        }

        if (filters.services && filters.services.length) {
            buf = filter(buf, function(elem) {
                return filters.services.indexOf(elem.service) > -1
            }) || {};
        }
        if (filters.from) {
            buf = filter(buf, function(elem) {
                return elem.sourceId == filters.from;
            }) || {};
        }
        if (filters.to) {
            buf = filter(buf, function(elem) {
                return elem.targetId == filters.to;
            }) || {};
        }

        return buf;

        function filter(obj, predicate) {
            var result = {}, key;

            for (key in obj) {
                if (obj.hasOwnProperty(key) && predicate(obj[key])) {
                    result[key] = obj[key];
                }
            }

            return result;
        };
    }

    function draw(filterService) {
        if (!filterService) {
            for (var i = 0, len = services.length; i < len; ++i) {
                var service = services[i];
                switch(service) {
                    case 'in-band' : InBandRoutes.draw();
                    case 'bridge-domain' : DomainRoutes.draw();
                    case 'mirror' : MirrorRoutes.draw();
                }
            }
            for (var route in routes.all) {
                routes.all[route].draw();
            }

        } else {
            if (!routes[filterService]) {
                console.error('Incorrect route service!');
            }
            switch(filterService) {
                case 'in-band' : InBandRoutes.draw();
                case 'bridge-domain' : DomainRoutes.draw();
                case 'mirror' : MirrorRoutes.draw();
            }
        }
    }

    function hide(filterService) {
        // Hide all
        if (!filterService) {
            for (var i = 0, len = services.length; i < len; ++i) {
                var service = services[i];
                 switch(service) {
                    case 'in-band' : InBandRoutes.hide();
                    case 'bridge-domain' : DomainRoutes.hide();
                    case 'mirror' : MirrorRoutes.hide();
                }
            }
            for (var route in routes.all) {
                routes.all[route].hide();
            }
        } else {
            if (!routes[filterService]) {
                console.error('Incorrect route service!');
            }
            switch(filterService) {
                case 'in-band' : InBandRoutes.hide();
                case 'bridge-domain' : DomainRoutes.hide();
                case 'mirror' : MirrorRoutes.hide();
            }
        }
    }

    function show(filterService) {
        if (!filterService) {
            for (var i = 0, len = services.length; i < len; ++i) {
                var service = services[i];
                switch(service) {
                    case 'in-band' : InBandRoutes.show();
                    case 'bridge-domain' : DomainRoutes.show();
                    case 'mirror' : MirrorRoutes.show();
                }
            }
            for (var route in routes.all) {
                routes.all[route].show();
            }
        } else {
            if (!routes[filterService]) {
                console.error('Incorrect route service!');
            }
            switch(filterService) {
                case 'in-band' : InBandRoutes.show();
                case 'bridge-domain' : DomainRoutes.show();
                case 'mirror' : MirrorRoutes.show();
            }
        }
    }

    function delAllDomainRoutes(name) {
        DomainRoutes.del(name);
    }

    function updateAllDomainRoutes(name, callback) {
        DomainRoutes.update(name, callback);
    }

    function del(filterService) {
        if (!filterService) {
            for (var i = 0, len = services.length; i < len; ++i) {
                var service = services[i];
                switch(service) {
                    case 'in-band' : InBandRoutes.hide();
                    case 'bridge-domain' : DomainRoutes.hide();
                }
            }
            for (var route in routes.all) {
                delete(routes.all[route]);
            }
        } else {
            if (!routes[filterService] && filterService != 'all') {
                console.error('Incorrect route service!');
            }
            switch(filterService) {
                case 'in-band' : InBandRoutes.del();
                case 'bridge-domain' : DomainRoutes.del();
            }
        }
    }

    function getRandomColor() {
        var color = 'hsl(';
        var h, s, l;
        s = '100%';
        l = '40%';

        h = offsetColor;
        offsetColor = (offsetColor + 30)%360;
        if (offsetColor == colorShift) {
            offsetColor = 10 + colorShift;
            colorShift += 5;
        }

        color += h + ',' + s + ',' + l + ')';
        return color;
    }

    function getSavedColor(routeId) {
        if (savedColors[routeId]) {
            return savedColors[routeId];
        }

        return undefined;
    }

    function saveColors(route) {
        if (!route || !route.paths) return;

        var colors = [];
        route.paths.forEach(function(path) {
            colors.push(path.color);
        });
        savedColors[route.id] = colors;
    }

})();

})(); // Scope isolation END;



