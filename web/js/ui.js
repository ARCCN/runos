/*

Copyright 2019 Applied Research Center for Computer Networks

*/

'use strict';
var i = 0;

// Elements in code below. Please keep the order!
// UI DOSControl
// UI Domain
// UI Mirrors
// UI AUX Device
// UI LAG
// UI QOS
// UI Multicast
// UI Endpoint
// UI DpidChecker
// UI Menu abstraction
// UI Details and stats
// UI Cluster
// UI Routes
// UI Tables

/* globals UI:true */
UI = function () {
    return {
        centerX         : false,
        centerY         : false,
        icons           : {},
        main            : false,
        canvas          : false,
        init            : init,
        createMenu      : createMenu,
        getMenu         : getMenu,
        createHint      : createHint,
        showNewVlan     : showNewVlan,
        showContextMenu : showContextMenu,
        addPortToLag    : addPortToLag,
        addPortToMirror : addPortToMirror,
        setStyle        : setStyle,
        reloadDomainList: reloadDomainList,

        getDomainRoutesTableHtml : getRoutesTableHtml,

        // Multicast management
        showMulticastTree : false,
        currentMulticastGroup : false,
        drawMulticastTree : drawMulticastTree,
        createMulticastManagementMenu : createMulticastManagementMenu,
        showMulticastManagementMenu: showMulticastManagementMenu,
        showMulticastAccessListMenu: showMulticastAccessListMenu,
        showMulticastAddSubnetMenu: showMulticastAddSubnetMenu,
        showMulticastAddGroupMenu: showMulticastAddGroupMenu,
        showMulticastAddServerMenu : showMulticastAddServerMenu,
        showMulticastSettingsMenu : showMulticastSettingsMenu,
        showMulticastAddListenersMenu : showMulticastAddListenersMenu,
        addPortToMulticastListeners : addPortToMulticastListeners,
        addPortToMulticastGeneralQuery : addPortToMulticastGeneralQuery,
        setMulticastRefresh: setMulticastRefresh,
        setMulticastAccessListRefresh: setMulticastAccessListRefresh,
        refreshMulticastManagementMenu: refreshMulticastManagementMenu,
        refreshMulticastManagementMenuTables: refreshMulticastManagementMenuTables,
        refreshAccessListTable: refreshAccessListTable,

        multicastRefreshAll: false,
        current_group: false,
        end_point        : {},

        showGroupTable   : showGroupTable,
        showMeterTable   : showMeterTable,

        activeMenuList   : [],
        components       : {},
        getStyleString   : getStyleString
    };
    // UI glob variables
    var hideItemsOffsetTop;

    function initVariables() {
        hideItemsOffsetTop = 20;
    }

    function init () {
        UI.canvas = document.getElementsByClassName('map')[0];
        UI.menu   = document.getElementsByClassName('menu')[0];
        UI.main = document.getElementsByTagName('main')[0];
        UI.mirrors = document.querySelector('.menu-nav-mirrors');
        UI.new_mirrors = document.querySelector('.menu-nav-mirrors-new');
        UI.mirror_pcap = document.querySelector('.menu-nav-mirrors-pcap');
        UI.logger = document.querySelector('.menu-logger');
        UI.recovery = document.querySelector('.menu-recovery');
        UI.menu.addEventListener('contextmenu', function(event) { event.preventDefault(); });
        UI.canvas.addEventListener('contextmenu', function(event) { event.preventDefault(); });
        document.querySelector('.menu-nav-settings').onclick = showGeneralSettings;
        document.querySelector('.canvas--scale-up').onclick = onClickCanvasScaleUp;
        document.querySelector('.canvas--scale-down').onclick = onClickCanvasScaleDown;
        document.querySelector('.menu-nav-switch-roles').onclick = showDpidCheckerMenu;
        UI.mirror_pcap.onclick = staticMirror.showLogList;

        UI.logger.onclick = Logger.show;
        UI.recovery.onclick = showRecoveryTable;
        initVariables();

        var event = new Event('ui:loaded');
        window.dispatchEvent(event);
    }

    function onClickCanvasScaleUp() {
        Net.setCanvasScale(Net.settings.canvasScaleX + 0.05, Net.settings.canvasScaleY + 0.05);
        Net.need_draw = true;
    }
    function onClickCanvasScaleDown() {
        Net.setCanvasScale(Net.settings.canvasScaleX - 0.05, Net.settings.canvasScaleY - 0.05);
        Net.need_draw = true;
    }

/* **** UI DOSControl **** */
    function showDosControl() {
        var hovered = HCI.getHovered();
        var html = '', inputPattern = /^\d+$/i,
            eInput, eBtn, border;

        html += '<input class="is-focused" type="text" name="border" placeholder="max packet-in/sec" id="dos-border-input"/>'
        html += '<button class="fourth blue" id="dos-set-btn"> Set </button>';

        html += '<p id="dos_border">Current dos control border: </p>';
        html += '<p id="mirror_border">Current mirror border: </p>';

        var menu = createMenu(html, {
            width: '300px',
            title : 'DOS Control',
            name  : 'menu--dos_control',
            unique : true
        });

        Server.ajax('GET', '/switches/' + hovered.id + '/dos-border/', function (response) {
            menu.element("dos_border").innerHTML = menu.element("dos_border").innerHTML + response["common_meters"];
            menu.element("mirror_border").innerHTML = menu.element("mirror_border").innerHTML + response["mirror_meters"];
        });

        eInput = menu.element('dos-border-input');
        eBtn = menu.element('dos-set-btn');

        eBtn.addEventListener('click', onClickSetBtn);
        eInput.addEventListener('input', onInputBorderInput);

        function onInputBorderInput(e) {
            var value = this.value;

            if (!inputPattern.test(value)) {
                this.value = value.slice(0, -1);
            }
        }
        function onClickSetBtn(e) {
            border = parseInt(eInput.value);

            if (Number.isNaN(border)) {
                UI.createHint('invalid border value', {fail: true});
                return;
            }

            Server.ajax('POST', '/switches/'+hovered.id+'/dos-border/', {border : border, mirror_border: border}, success, error);
        }

        function success(response) {
            UI.createHint('DOS control level was set');
            Logger.info('runos', 'DOS control level was set on ' + border);
            menu.remove();
        }
        function error(err) {
            UI.createHint(err, {fail: true});
            Logger.error('runos', err);
        }
    }
/* **** UI DOSControl END **** */

/* **** UI Domain **** */
    function showDomainMacPortList(hovered) {
    }

    function deleteDomain (name) {
    }

    function showNewDomain () {
    }
    
    function drawDomainList () {
    }

    function reloadDomainList () {
    }

    function filterDomainList (template, type_tmpl) {
    }

    function showNewVlan (hovered, x, y) {
    }
/* **** UI Domain END **** */

/* **** UI Mirrors **** */
    function showMirrorsList() {
    }

    function addPortToMirror(hovered) {
    }

    function showNewMirror(port, options) {
    }

    function showUpdateMirror(mirror) {
    }

/* **** UI Mirrors END **** */

/* **** UI AUX Device **** */

    function showNewDevice (hovered) {
        if (hovered.type != "port") return;
        if (hovered.link) {
            var other = hovered.link.other(hovered.router);
            if (other.mode != "SOC") return;
        }

        var menu = UI.createMenu(getNewDeviceHtml(), {
                         title: "New Device",
                         name: "new_device",
                         unique: true,
                         width: "300px"
        });

        addEventHandlers();

        function getNewDeviceHtml () {
            var html = '';

            html += '<input type="text" id="name" placeholder="Device name">';
            html += '<input type="text" id="desc" placeholder="Device description">';
            html += '<div class="aux-device-container">';
            for (var i = 0, l = aux_devices_pool.length; i < l; i++) {
                html += '<img src="images/'+aux_devices_pool[i]+'.png" class="aux-device" id="'+aux_devices_pool[i]+'">';
            }
            html += '</div>';
            html += '<button class="blue fourth" id="create_device">Create</button>';
            html += '<button class="rosy fourth" id="close_menu">Close</button>';

            return html;
        }

        function addEventHandlers () {
            menu.element('create_device').onclick = function () {
                var device_obj = {},
                    name = menu.element("name").value;
                device_obj["desc"] = menu.element("desc").value;
                var icon = menu.querySelector('.selected');
                if (!icon || !name) return;

                device_obj["icon"] = icon.id;
                device_obj["sw_dpid"] = hovered.router.id;
                device_obj["sw_port"] = hovered.of_port;

                Server.ajax("PUT", '/aux-devices/' + name + '/', device_obj);
                menu.remove();
            };
            menu.element('close_menu').onclick = function () {
                menu.remove();
            };

            var devices = menu.querySelectorAll('.aux-device');
            for (var i = 0, l = devices.length; i < l; i++) {
                devices[i].onclick = function () {
                    var devices = menu.querySelectorAll('.aux-device');
                    for (var i = 0, l = devices.length; i < l; i++)
                        devices[i].classList.remove("selected");
                    this.classList.add("selected");
                }
            }
        }
    }

    function removeAuxDevice (hovered) {
        var hovered = HCI.getHovered();
        if (hovered.mode != 'AUX') return;

        Server.ajax("DELETE", '/aux-devices/'+hovered.name+'/', function (resp) {
            var error = (resp.error ? true : false);
            createHint(resp.res, {fail: error});
            if (!error) {
                Net.del(hovered.links[0]);
                Net.del(hovered);
                UI.menu.style.display = 'none';
            }
        });
    }

/* **** UI AUX Device END **** */

/* **** UI LAG **** */
    function addPortToLag (hovered) {
    }

    function showNewLag (port) {
    }
    
    function showLagDetails(port) {
    }
/* **** UI LAG END **** */

/* **** UI QOS **** */
    function showQoSMenu () {

    }
/* **** UI QOS END **** */

/* **** UI Multicast **** */
    function createMulticastManagementMenu () {
    //    Server.ajax('GET', '/multicast_group_list/groups/', UI.showMulticastManagementMenu);
    }

    // Multicast management menu
    function showMulticastManagementMenu (response) {
    }
  
    function showMulticastAccessListMenu () {
        // Check if the menu already exists
        var access_list_menu = document.querySelector('.access_list_menu');
        if (access_list_menu) {
            access_list_menu.remove();
            return;
        }

        var  html = '';
        html += '<p class="desc"></p>';
        html += '<button class="blue half multicast--acces_list--add_button">Add</button>';
        html += '<button class="rosy half multicast--acces_list--delete_button">Delete</button>';
        html += '<table class="access_list_table arccn"></table>';
        html += '<button class="full multicast--acces_list--close_button">Close</button>';

        var eMenu = createMenu(html, {
            title: 'Multicast Access List',
            width : '40%',
            name : 'access_list_menu'
        });
        var eDeleteButton = eMenu.querySelector('.multicast--acces_list--delete_button'),
            eAddButton = eMenu.querySelector('.multicast--acces_list--add_button'),
            eCloseButton = eMenu.querySelector('.multicast--acces_list--close_button');

        eDeleteButton.addEventListener('click', onClickDelete);
        eAddButton.addEventListener('click', onClickAdd);
        eCloseButton.addEventListener('click', onClickClose);

        // Refresh data in the menu
        UI.refreshAccessListTable();

        function onClickDelete() {
            var obj = {},
                access_list_table = eMenu.querySelector('.access_list_table'),
                iter = access_list_table.querySelectorAll('.checker');
            for (var i = 0; i < iter.length; i++) {
                if (iter[i].checked) {
                    Server.ajax("DELETE", '/multicast_access_list/' + iter[i].name + '/', obj, UI.setMulticastAccessListRefresh);
                }
            }
            // Refresh data in the menu
            UI.refreshAccessListTable();
            // Send DELETE ajax (delete group)
            //Server.ajax("DELETE", '/multicast_group/' + group_selector.value, obj, UI.setMulticastRefresh);
        }
        function onClickAdd() {
            showMulticastAddSubnetMenu();
        }
        function onClickClose() {
            eMenu.remove();
            Net.need_draw = true;
        }
    }

    function showMulticastAddSubnetMenu () {
        var styles = {
            mask : {
                width: '20%'
            }
        };

        // Get multicast menu
        var management = document.querySelector('.multicast_management_menu');
        if (management == undefined) return;

        // Check if the menu already exists
        var multicast_add_subnet_menu = document.querySelector('.multicast_add_subnet_menu');
        if (multicast_add_subnet_menu) return;

        var html = '';
        html += '<p class="subnet_paragraph desc">Subnet/Mask</p>';
        html += '<input type="text" class="subnet_ip_address_input dname" placeholder="Subnet IP">';
        html += '<input type="text" class="mask_input dname" placeholder="Mask" ' +
                'style="' +  getStyleString(styles.mask)+'">';
        html += '<button class="blue half multicast--add_subnet--add_button">Add subnet</button>';
        html += '<button class="rosy half multicast--add_subnet--cancel_button">Cancel</button>'

        var eMenu = createMenu(html, {
            width : '300px',
            title : 'Add Subnet'
        })

        var eAddButton = eMenu.querySelector('.multicast--add_subnet--add_button'),
            eCancelButton = eMenu.querySelector('.multicast--add_subnet--close_button'),
            eIpInput = eMenu.querySelector('.subnet_ip_address_input'),
            eMaskInput = eMenu.querySelector('.mask_input');

        eAddButton.addEventListener('click', onClickAdd);
        eCancelButton.addEventListener('click', onClickCancel);

        // Refresh data in the menu
        UI.refreshAccessListTable();

        function onClickAdd() {
            var obj = {},
                multicast = {},
                multicast_subnets = [],
                multicast_subnet = {};

            // Get settings for the new subnet
            multicast_subnet["ip-address"] = eIpInput.value;
            multicast_subnet["ip-mask"] = parseInt(eMaskInput.value,10);
            multicast_subnets.push(multicast_subnet);

            multicast["multicast-subnets"] = multicast_subnets;
            obj["multicast"] = multicast;
            // Send PUT ajax (create subnet)
            Server.ajax("PUT", '/multicast_access_list/' + eIpInput.value + '/', obj, UI.setMulticastAccessListRefresh);
            // Close add subnet menu
            eMenu.remove();
            Net.need_draw = true;
        }
        function onClickCancel() {
            eMenu.remove();
        }
    }

    function refreshAccessListTable () {
        var access_list_menu = document.querySelector('.access_list_menu');
        var subnets = Net.multicast_access_list_obj;

        var html = '';
        html += '<table>';
            html += '<thead>';
                html += '<tr>';
                    html += '<th><b>№</b></th>';
                    html += '<th><b>IP-address</b></th>';
                    html += '<th></th>';
                html += '</tr>';
            html += '</thead>';
            html += '<tbody>';
            for (var subnet_i = 0; subnet_i < subnets.length; subnet_i++) {
                var subnet = subnets[subnet_i];

                var subnet_ip = subnet["ip-address"],
                    subnet_mask = subnet["ip-mask"];

                html += '<tr>';
                    html += '<td>' + (subnet_i + 1) + '</td>';
                    html += '<td>' + utils.intToIP(subnet_ip) + "/" + subnet_mask + '</td>';
                    html += '<td><input type="checkbox" class="checker" name="' + utils.intToIP(subnet_ip) + '"></td>';
                html += '</tr>';
            }
            html += '</tbody>';
        html += '</table>';
        // Set new table
        var access_list_table = access_list_menu.querySelector('.access_list_table');
        access_list_table.innerHTML = html;
    }

    function showMulticastAddGroupMenu () {
        var styles = {
            select : {
                width : '160px'
            }
        };

        var groups = Net.multicast["multicast-groups"];

        // Get multicast menu
        var management = document.querySelector('.multicast_management_menu');
        if (management == undefined) return;

        // Check if the menu already exists
        var multicast_add_group_menu = document.querySelector('.multicast_add_group_menu');
        if (multicast_add_group_menu) return;

        var html = '';
        html += '<p class="group_settings_paragraph desc">Group settings</p>';
        html += '<input type="text" class="group_name_input dname" placeholder="Group name">';
        html += '<input type="text" class="group_address_input dname" placeholder="Group address">';
        html += '<p class="server_settings_paragraph desc">Server settings</p>';
        html += '<select class="server_selector" style="' + getStyleString(styles.select) + '">';
            html += '<option value="New server" selected="selected">New server</option>';
        for (var group_i = 0; group_i < groups.length; group_i++) {
            var group_name = groups[group_i]["group-name"];
            html += '<option value="' + group_name + '">' + group_name + '</option>';

        }
        html += '</select>';
        html += '<p class="server_empty_paragraph desc"></p>';
        html += '<input type="text" class="server_switch_dpid_input dname" placeholder="Server switch dpid">';
        html += '<input type="text" class="server_switch_port_input dname" placeholder="Server switch port">';
        html += '<input type="text" class="server_vlan_input dname" placeholder="Server VLAN">';
        html += '<button class="blue half multicast--add_group--add">Add Group</button>';
        html += '<button class="rosy half multicast--add_group--cancel">Cancel</button>';

        var eMenu = createMenu(html, {
            width: '200px',
            title: 'Add Multicast Group'
        });

        var eAddButton = eMenu.querySelector('.multicast--add_group--add'),
            eCancelButton = eMenu.querySelector('.multicast--add_group--cancel'),
            eSelect = eMenu.querySelector('.server_selector'),
            eGroupNameInput = eMenu.querySelector('.group_name_input'),
            eGroupAddressInput = eMenu.querySelector('.group_address_input'),
            eSwitchDpidInput = eMenu.querySelector('.server_switch_dpid_input'),
            eSwitchPortInput = eMenu.querySelector('.server_switch_port_input'),
            eVlanInput = eMenu.querySelector('.server_vlan_input');

        eAddButton.addEventListener('click', onClickAdd);
        eCancelButton.addEventListener('click', onClickCancel);
        eSelect.addEventListener('change', onChangeSelect);

        function onClickAdd() {
            var obj = {},
                multicast = {},
                multicast_groups = [],
                multicast_group = {},
                multicast_servers = [],
                multicast_server = {};

            // Get settings for the new group
            multicast_server["switch"] = parseInt(eSwitchDpidInput.value,10);
            multicast_server["port"] = parseInt(eSwitchPortInput.value,10);
            if(eVlanInput.value != "" && eVlanInput.value != "No")
                multicast_server["vlan-id"] = parseInt(eVlanInput.value,10);
            multicast_servers.push(multicast_server);

            multicast_group["group-name"] = eGroupNameInput.value;
            multicast_group["multicast-address"] = eGroupAddressInput.value;
            multicast_group["multicast-servers"] = multicast_servers;
            multicast_groups.push(multicast_group);

            multicast["multicast-groups"] = multicast_groups;
            obj["multicast"] = multicast;

            // Send PUT ajax (create group)
            Server.ajax("PUT", '/multicast_group/' + eGroupNameInput.value + '/', obj, UI.setMulticastRefresh);

            // Close add group menu
            eMenu.remove();
            Net.need_draw = true;
        }
        function onClickCancel() {
            eMenu.remove();
            Net.need_draw = true;
        }
        function onChangeSelect() {
            var groups = Net.multicast["multicast-groups"];

            if (eSelect.value != "New server") {
                for (var group_i = 0; group_i < groups.length; group_i++) {

                    // Get group name
                    var group = groups[group_i];
                    var group_name = group["group-name"];

                    if (eSelect.value == group_name) {
                        var multicast_servers = group["multicast-servers"],
                            server = multicast_servers[0];

                        eSwitchDpidInput.value = server["switch"];
                        eSwitchPortInput.value = server["port"];
                        eVlanInput.value = server["vlan-id"];

                        eSwitchDpidInput.readOnly = eSwitchPortInput.readOnly = eVlanInput.readOnly = true;
                    }
                }
            } else {
                eSwitchDpidInput.value = eSwitchPortInput.value = eVlanInput.value = '';
                eSwitchDpidInput.readOnly = eSwitchPortInput.readOnly = eVlanInput.value = '';
            }
        }
    }

    function showMulticastAddServerMenu (groups_name, multicast_address) {
        var styles = {
            select : {
                width : '160px'
            }
        };

        var groups = Net.multicast["multicast-groups"];

        // Get multicast menu
        var management = document.querySelector('.multicast_management_menu');
        if (management == undefined) return;

        // Check if the menu already exists
        var multicast_add_group_menu = document.querySelector('.multicast_add_server_menu');
        if (multicast_add_group_menu) return;

        var html = '';
        html += '<input type="text" class="server_switch_dpid_input dname" placeholder="Server switch dpid">';
        html += '<input type="text" class="server_switch_port_input dname" placeholder="Server switch port">';
        html += '<input type="text" class="server_vlan_input dname" placeholder="Server VLAN">';
        html += '<button class="blue half multicast--add_server--add">Add Server</button>';
        html += '<button class="rosy half multicast--add_server--cancel">Cancel</button>';

        var eMenu = createMenu(html, {
            width: '200px',
            title: 'Add Multicast Server'

        });

        var eAddButton = eMenu.querySelector('.multicast--add_server--add'),
            eCancelButton = eMenu.querySelector('.multicast--add_server--cancel'),
            eSwitchDpidInput = eMenu.querySelector('.server_switch_dpid_input'),
            eSwitchPortInput = eMenu.querySelector('.server_switch_port_input'),
            eVlanInput = eMenu.querySelector('.server_vlan_input');

        eAddButton.addEventListener('click', onClickAdd);
        eCancelButton.addEventListener('click', onClickCancel);

        function onClickAdd() {
            var obj = {},
                multicast = {},
                multicast_groups = [],
                multicast_group = {},
                multicast_servers = [],
                multicast_server = {};

            // Get settings for the new group
            multicast_server["switch"] = parseInt(eSwitchDpidInput.value,10);
            multicast_server["port"] = parseInt(eSwitchPortInput.value,10);
            if(eVlanInput.value != "" && eVlanInput.value != "No")
                multicast_server["vlan-id"] = parseInt(eVlanInput.value,10);
            multicast_servers.push(multicast_server);

            multicast_group["group-name"] = groups_name;
            /*DEL*///multicast_group["multicast-address"] = utils.intToIP(multicast_address);
            multicast_group["multicast-address"] = multicast_address;
            multicast_group["multicast-servers"] = multicast_servers;
            multicast_groups.push(multicast_group);

            multicast["multicast-groups"] = multicast_groups;
            obj["multicast"] = multicast;

            // Send PUT ajax (add server)
            Server.ajax("PUT", '/multicast_group/' + groups_name + '/', obj, UI.setMulticastRefresh);

            // Close add server menu
            eMenu.remove();
            Net.need_draw = true;
        }
        function onClickCancel() {
            eMenu.remove();
            Net.need_draw = true;
        }
    }

    function showMulticastSettingsMenu (response) {
        // Get multicast menu
        var management = document.querySelector('.multicast_management_menu');
        if (management == undefined) return;

        // Check if the menu already exists
        var multicast_settings_menu = document.querySelector('.multicast_settings_menu');
        if (multicast_settings_menu) return;
        console.log(response);

        var multicast = response["multicast"];

        if (multicast["general-query-policy"] == "manual") {
            Net.new_query_ports = [];
            if (multicast["general-query-ports"].length > 0) {
                multicast["general-query-ports"].forEach(function(item, i, arr) {
                    var dpid = item["dpid"],
                        port = item["port"];
                    if (dpid != 0 && port != 0)
                        addPortToMulticastGeneralQuery(Net.getPort(Net.getHostByID(dpid), port));
                });
            }
        }

        var html = '';
        html += '<table id="rtk-options_table" class="rtk-options_table">';

            // Log level
            html += '<tr>';
            html += '<td>Log-level</td>';
            html += '<td><select size="1" name="log_level" id="log_level" style="width:100px;" ';
            html += 'class="multicast--log_level">';
            var order = [0, 1, 2, 3],
                log_description = {};
            log_description[0] = "None";
            log_description[1] = "Only errors";
            log_description[2] = "Main log";
            log_description[3] = "Debug log";
            order.forEach(function(key) {
                var log_level = log_description[key];
                if(multicast["log-level"] == key)
                    html += '<option selected value='+key+'> '+log_level+' </option>';
                else
                    html += '<option value='+key+'> '+log_level+' </option>';
            });
            html += '</select></td>';
            html += '</tr>';

            // Flapping interval
            html += '<tr>';
            html += '<td>Flapping interval</td>';
            html += '<td><input type="text" id="flapping_interval" style="width:100px;" ';
            html += 'class="multicast--flapping_interval" ';
            if (multicast["flapping-interval"])
                html += 'value='+multicast["flapping-interval"];
            else
                html += 'placeholder="ms"';
            html += '></td>';
            html += '</tr>';

            // Auto main server reconnection
            html += '<tr style="height:40px">';
            html += '<td>Auto main server</td>';
            html += '<td><input type="checkbox" id="auto_main_server_reconnect" style="width:100px;" ';
            html += 'class="multicast--auto_main_server_reconnect" '
            if (multicast["auto-main-server-reconnect"] == "true")
                html += 'checked';
            html += '></td>';
            html += '</tr>';

            // Multicast queue-id
            html += '<tr>';
            html += '<td>Multicast queue-id</td>';
            html += '<td><input type="text" id="multicast_queue_id" style="width:100px;" ';
            html += 'class="multicast--multicast_queue_id" ';
            if (multicast["multicast-queue-id"])
                html += 'value='+multicast["multicast-queue-id"];
            else
                html += 'placeholder="ms"';
            html += '></td>';
            html += '</tr>';

            // General query policy
            html += '<tr>';
            html += '<td>General query policy</td>';
            html += '<td><select size="1" name="general_query_policy" id="general_query_policy" style="width:100px;" ';
            html += 'class="multicast--general_query_policy">';
            var order = [0, 1, 2],
                policy_description = [];
            policy_description.push("none");
            policy_description.push("manual");
            policy_description.push("all");
            order.forEach(function(index) {
                var general_query_policy = policy_description[index];
                if(multicast["general-query-policy"] == general_query_policy)
                    html += '<option selected value='+policy_description[index]+'> '+policy_description[index]+' </option>';
                else
                    html += '<option value='+policy_description[index]+'> '+policy_description[index]+' </option>';
            });
            html += '</select></td>';
            html += '</tr>';

        html += '</table>';

        html += '<button class="blue half multicast--settings--ok">OK</button>';
        html += '<button class="rosy half multicast--settings--cancel">Cancel</button>';

        var eMenu = createMenu(html, {
            width: '300px',
            title: 'Multicast Settings'
        });

        var eOkButton = eMenu.querySelector('.multicast--settings--ok'),
            eCancelButton = eMenu.querySelector('.multicast--settings--cancel'),
            eLogLevelSelect = eMenu.querySelector('.multicast--log_level'),
            eFlappingIntervalInput = eMenu.querySelector('.multicast--flapping_interval'),
            eAutoMainServerCheckbox = eMenu.querySelector('.multicast--auto_main_server_reconnect'),
            eMulticastQueueIdInput = eMenu.querySelector('.multicast--multicast_queue_id'),
            eGeneralQueryPolicySelect = eMenu.querySelector('.multicast--general_query_policy');

        eOkButton.addEventListener('click', onClickOk);
        eCancelButton.addEventListener('click', onClickCancel);
        eGeneralQueryPolicySelect.addEventListener('change', onChangeQueryPolicy);

        function onClickOk() {
            var obj = {},
                multicast = {};

            multicast["log-level"] = parseInt(eLogLevelSelect.value, 10);
            multicast["flapping-interval"] = parseInt(eFlappingIntervalInput.value, 10);
            multicast["auto-main-server-reconnect"] = eAutoMainServerCheckbox.checked;
            multicast["multicast-queue-id"] = parseInt(eMulticastQueueIdInput.value, 10);
            multicast["general-query-policy"] = eGeneralQueryPolicySelect.value;
            if (Net.new_query_ports)
                multicast["general-query-ports"] = Net.new_query_ports;
            obj["multicast"] = multicast;
            // Send PUT ajax (save settings)
            Server.ajax("PUT", '/multicast/settings/', obj, UI.setMulticastRefresh);
            // Close add subnet menu
            eMenu.remove();
            Net.clearNew();
            Net.need_draw = true;
        }
        function onClickCancel() {
            eMenu.remove();
            Net.clearNew();
            Net.need_draw = true;
        }
        function onChangeQueryPolicy() {
            if (eGeneralQueryPolicySelect.value == "manual") {
                Net.new_query_ports = [];
            } else {
                Net.new_query_ports = false;
                Net.clearNew();
            }
        }
    }

    function addPortToMulticastGeneralQuery (hovered) {
        if (!Net.new_query_ports)
            return;

        var query_port = {};
        query_port["dpid"] = hovered.router.id;
        query_port["port"] = hovered.of_port;

        if (!hovered.isSelected) { // Add to list
            Net.new_query_ports.push(query_port);
            hovered.isSelected = true;
        } else { // Remove from list
            Net.new_query_ports.splice(Net.new_query_ports.indexOf(query_port));
            hovered.isSelected = false;
        }

        Net.need_draw = true;
    }

    function addPortToMulticastListeners (hovered) {
        if (!Net.new_multicast_listeners)
            return;

        var listener = {};
        listener["switch"] = hovered.router.id;
        listener["port"] = hovered.of_port;

        if (!hovered.isSelected) { // Add to list
            Net.new_multicast_listeners.push(listener);
            hovered.isSelected = true;
        } else { // Remove from list
            Net.new_multicast_listeners.splice(
                Net.new_multicast_listeners.indexOf(listener)
            );
            hovered.isSelected = false;
        }

        Net.need_draw = true;
    }

    function setMulticastRefresh () {
        UI.multicastRefreshAll = true;
    }

    function setMulticastAccessListRefresh () {
        UI.multicastRefreshAccessList = true;
    }
    function refreshMulticastManagementMenu (response) {

        // Get multicast menu
        var management = document.querySelector('.multicast_management_menu');

        // If menu is not opened
        if (management == undefined) return;

        // Get multicast data
        var groups = Net.multicast["multicast-groups"],
            current_group = false;
        if (response)
            groups = response["multicast"]["multicast-groups"];

        // Refresh group selector
        {
            // Get group selector entity
            var old_group_selector = management.querySelector('.group_selector'),
                new_group_selector = document.createElement('select'),
                group_num = -1;

            // For every group create option entity
            for (var group_i = 0; group_i < groups.length; group_i++) {

                // Get group name
                var group = groups[group_i],
                    group_name = group["group-name"],
                    group_address = group["multicast-address"];

                // Create option
                var option = document.createElement('option');
                option.value = group_name;
                /*DEL*///option.text = group_name + ' (' + utils.intToIP(group_address) + ')';
                option.text = group_name + ' (' + group_address + ')';

                // Set selected option
                if (old_group_selector.value == group_name) {
                    option.selected = true;
                    current_group = group;
                    group_num = group_i;
                } else {
                    if ((Net.multicast_tree === undefined || Net.multicast_tree == false) && group_i == 0) {
                        option.selected = true;
                        current_group = group;
                        group_num = group_i;
                    } else if(Net.multicast_tree !== undefined &&
                              Net.multicast_tree != false &&
                              Net.multicast_tree["group-name"] == group_name) {
                        option.selected = true;
                        current_group = group;
                        group_num = group_i;
                    }
                }

                // Copy onchange function
                new_group_selector.onchange = old_group_selector.onchange;

                // Add option
                new_group_selector.appendChild(option);
            }
            // Add new selector to the menu
            old_group_selector.innerHTML = new_group_selector.innerHTML;
            old_group_selector.selectedIndex = group_num;
        }
        // Refresh tables
        UI.refreshMulticastManagementMenuTables(response);
    }

    function refreshMulticastManagementMenuTables (response) {
        function onClickDeleteServer(group_name, switch_dpid) {
            var obj = {},
                multicast = {},
                multicast_groups = [],
                multicast_group = {},
                multicast_servers = [],
                multicast_server = {};

            // Get settings for the server
            multicast_server["switch"] = switch_dpid;
            multicast_server["port"] = port;
            multicast_server["mac-address"] = mac_address;
            if(vlan_id >= 0 && vlan_id <= 4095)
                multicast_server["vlan-id"] = vlan_id;
            multicast_server["ip-address"] = ip_address;
            multicast_servers.push(multicast_server);

            // Get settings for the group
            multicast_group["group-name"] = current_group["group-name"];
            multicast_group["multicast-address"] = current_group["multicast-address"];
            multicast_group["multicast-servers"] = multicast_servers;
            multicast_groups.push(multicast_group);

            multicast["multicast-groups"] = multicast_groups;
            obj["multicast"] = multicast;

            // Send DELETE ajax (create group)
            Server.ajax("DELETE", '/multicast_server_delete/' + group_name + '/' + switch_dpid + '/', obj, UI.setMulticastRefresh);
        }
        // Get menu
        var management = document.querySelector('.multicast_management_menu');
        // If menu is not opened
        if (management == undefined) return;
        // Get group selector
        var group_selector = management.querySelector('.group_selector');

        // Get multicast data
        var groups = Net.multicast["multicast-groups"],
            current_group = false,
            settings_table = management.querySelector('.settings_table'),
            server_table = management.querySelector('.server_table'),
            listener_table = management.querySelector('.listener_table');

        if (response)
            groups = response["multicast"]["multicast-groups"];

        // Get current selected group
        for (var group_i = 0; group_i < groups.length; group_i++) {

            // Get group name
            var group = groups[group_i];
            var group_name = group["group-name"];

            // Get selected group
            if (group_selector.value == group_name) {
                current_group = group;
                UI.current_group = group;
            } else {
                if (group_i == 0) {
                    current_group = group;
                    UI.current_group = group;
                }
            }
        }
        Net.multicast["multicast-groups"].forEach (function (group) {
            if(current_group["multicast-address"] == group["multicast-address"]) {
                UI.currentMulticastGroup = group;
                Net.multicast_tree = group;
            }
        });

        var html = '';
        html += '<thead>';
            html += '<th><b>Group info</b></th>';
        html += '</thead>';
        html += '<tr>';
            html += '<td><b>Group name</b></td>';
            html += '<td><b>' + current_group["group-name"] + '</b></td>';
        html += '</tr>';
        html += '<tr>';
            html += '<td><b>Group address</b></td>';
            /*DEL*///html += '<td><b>' + utils.intToIP(current_group["multicast-address"]) + '</b></td>';
            html += '<td><b>' + current_group["multicast-address"] + '</b></td>';
        html += '</tr>';
        settings_table.innerHTML = html;

        var html_head = '';
        html_head += '<thead>';
            html_head += '<tr><th><b>{{title}}</b></th></tr>';
            html_head += '<tr>';
            html_head += '<td><b>№</b></td>';
            html_head += '<td style="width:100px"><b>Switch-DPID</b></td>';
            html_head += '<td style="width:40px" ><b>Port</b></td>';
            html_head += '<td style="width:140px"><b>MAC-address</b></td>';
            html_head += '<td style="width:80px" ><b>Vlan-ID</b></td>';
            html_head += '<td style="width:110px"><b>IP-address</b></td>';
            html_head += '<td style="width:40px" >';
                html_head += '<i id="add_id" class="action add_multicast_server multicast--add_server"></i>';
            html_head += '</td>';
            html_head += '</tr>';
        html_head += '</thead>';

        html = '';
        html += html_head.replace('{{title}}', 'Server list');
        // Set server table data
        var multicast_servers = current_group["multicast-servers"];
        for (var server_i = 0; server_i < multicast_servers.length; server_i++) {
            // Get server data
            var server = multicast_servers[server_i];
            var switch_dpid = server["switch"],
                port = server["port"],
                mac_address = server["mac-address"],
                vlan_id = server["vlan-id"],
                ip_address = server["ip-address"];
            var server_number = server_i + 1;
            var multicast_main_server = Net.multicast["multicast-main-server"];

            // Create server row
            html += '<tr>';
                html += '<td style="width:40px" >';
                    html += '<input id="'
                    html += server_number + '_' + switch_dpid + '_id" ';
                    html += 'class="multicast_main_server" type="radio" name="main_server" data-type="'
                    html += switch_dpid;
                    html += '" ';
                    if(switch_dpid == multicast_main_server) html += 'checked';
                    html += '></input>';
                html += '</td>';
                html += '<td>' + server_number + '</td>';
                html += '<td style="width:100px">' + switch_dpid + '</td>';
                html += '<td style="width:40px" >' + port + '</td>';
                html += '<td style="width:140px">' + mac_address + '</td>';
                html += '<td style="width:80px" >' + ((vlan_id >= 0 && vlan_id <= 4095) ? vlan_id : "No") + '</td>';
                html += '<td style="width:110px">' + ip_address + '</td>';
                html += '<td style="width:40px" >';
                    html += '<i id="'
                    html += server_number + '_id" ';
                    html += 'class="action delete_multicast_server multicast--delete_server_';
                    html += server_number + '"';
                    html += '></i>';
                html += '</td>';
            html += '</tr>';
        }
        server_table.innerHTML = html;

        // Add server button
        var eAddServerButton = document.getElementById('add_id');
        //querySelector('.multicast--add_server');
        eAddServerButton.addEventListener('click', onClickAddServer);
        function onClickAddServer() {
            showMulticastAddServerMenu(current_group["group-name"], current_group["multicast-address"]);
        }

        // Delete server buttons
        for (var server_i = 0; server_i < multicast_servers.length; server_i++) {
            // Get server data
            var server = multicast_servers[server_i];
            var switch_dpid = server["switch"],
                port = server["port"],
                mac_address = server["mac-address"],
                vlan_id = server["vlan-id"],
                ip_address = server["ip-address"];
            var server_number = server_i + 1;

            // Delete specific server button
            var eDeleteServerButton = document.getElementById(server_number + '_id');
            //querySelector('.multicast--delete_server_' + server_number);
            //eDeleteServerButton.addEventListener('click', function () {onClickDeleteServer(current_group["group-name"], switch_dpid)});
            eDeleteServerButton.addEventListener('click', (function(inner_switch_dpid) {
                                                               return function () {
                                                                   onClickDeleteServer(current_group["group-name"], inner_switch_dpid)
                                                               };
                                                           })(switch_dpid)
                                                );
        }

        // Change main server buttons
        var typeBtns = document.querySelectorAll('.multicast_main_server');
        Array.prototype.forEach.call(typeBtns, function(typeBtn) {
            typeBtn.addEventListener('change', function() {
                var obj = {},
                    multicast = {};

                multicast["multicast-main-server"] = this.getAttribute('data-type');
                obj["multicast"] = multicast;
                Server.ajax("PUT", '/multicast_main_server/', obj);
            });
        });

        html = '';
        html += html_head.replace('{{title}}', 'Listener list');
        var multicast_listeners = current_group["multicast-listeners"];
        for (var listener_i = 0; listener_i < multicast_listeners.length; listener_i++) {

            // Get listener data
            var listener = multicast_listeners[listener_i];
            var switch_dpid = listener["switch"],
                port = listener["port"],
                mac_address = listener["mac-address"],
                vlan_id = listener["vlan-id"],
                ip_address = listener["ip-address"];
            var listener_number = listener_i + 1;

            // Create listener row
            html += '<tr>';
                html += '<td>' + listener_number + '</td>';
                html += '<td style="width:100px">' + switch_dpid + '</td>';
                html += '<td style="width:40px" >' + port + '</td>';
                html += '<td style="width:140px">' + mac_address + '</td>';
                html += '<td style="width:80px" >' + ((vlan_id >= 0 && vlan_id <= 4095) ? vlan_id : "No") + '</td>';
                html += '<td style="width:110px">' + ip_address + '</td>';
                html += '<td style="width:40px" ></td>';
            html += '</tr>';
        }
        listener_table.innerHTML = html;
        listener_table.rows[1].cells[6].innerHTML = '<i id="add_listeners" class="action add_multicast_server multicast--add_server"></i>';

        // Add access list button
        var eAddListenersButton = document.getElementById('add_listeners');
        eAddListenersButton.addEventListener('click', onClickAddListeners);

        function onClickAddListeners() {
            Server.ajax("GET", '/multicast_group_list/groups/', UI.showMulticastAddListenersMenu);
        }

    }

    function showMulticastAddListenersMenu(response) {
        // Get multicast menu
        var management = document.querySelector('.multicast_management_menu');
        if (management == undefined) return;

        // Check if the menu already exists
        var multicast_add_listeners_menu = document.querySelector('.multicast_add_listeners_menu');
        if (multicast_add_listeners_menu) return;

        var multicast = response["multicast"],
            group = false,
            vlan = false;

        for (var group_i = 0; group_i < multicast["multicast-groups"].length; group_i++) {
            console.log("TEST");
            console.log("multicast group-name", multicast["multicast-groups"][group_i]["group-name"]);
            console.log("UI.current_group group-name", UI.current_group["group-name"]);
            if (multicast["multicast-groups"][group_i]["group-name"] == UI.current_group["group-name"]) {
                group = multicast["multicast-groups"][group_i];
                vlan = group["multicast-servers"][0]["vlan-id"];
            }
        }

        // Show already presented listeners
        Net.new_multicast_listeners = [];
        var listeners = multicast["multicast-groups"][0]["multicast-listeners"];
        if (listeners.length > 0) {
            listeners.forEach(function(item, i, arr) {
                var dpid = item["switch"],
                    port = item["port"];
                if (dpid != 0 && port != 0)
                    addPortToMulticastListeners(Net.getPort(Net.getHostByID(dpid), port));
            });
        }

        var html = '';
        html += '<p class="desc"> Choose ports where to add static multicast listeners </p>';
        html += '<button class="blue half multicast--settings--ok">OK</button>';
        html += '<button class="rosy half multicast--settings--cancel">Cancel</button>';

        var eMenu = createMenu(html, {
            width: '300px',
            title: 'Add Listeners'
        });

        var eOkButton = eMenu.querySelector('.multicast--settings--ok'),
            eCancelButton = eMenu.querySelector('.multicast--settings--cancel');

        eOkButton.addEventListener('click', onClickOk);
        eCancelButton.addEventListener('click', onClickCancel);

        function onClickOk() {
            var obj = {};

            var listeners = [];
            Net.new_multicast_listeners.forEach(function(item, i, arr) {
                var dpid = item["switch"],
                    port = item["port"];
                if (dpid != 0 && port != 0)
                    listeners.push({"switch": dpid,
                                    "port": port,
                                    "vlan-id": vlan,
                                    "static": true});
            });

            for (var group_i = 0; group_i < multicast["multicast-groups"].length; group_i++) {
                multicast["multicast-groups"][group_i]["multicast-listeners"] = listeners;
            }
            obj["multicast"] = multicast;

            // Send PUT ajax (save settings)
            Server.ajax("PUT", '/multicast_group/' + group["group-name"] + '/', obj, UI.setMulticastRefresh);
            // Close add subnet menu
            eMenu.remove();
            Net.clearNew();
            Net.need_draw = true;
        }
        function onClickCancel() {
            eMenu.remove();
            Net.clearNew();
            Net.need_draw = true;
        }
    }

    function drawMulticastTree() {
        if(!UI.showMulticastTree || !Net.multicast_tree || !Net.multicast_tree["multicast-links"])
            return;

        var line_width = 10 * Net.settings.scale / 64;

        CTX.save();
        if(Net.multicast_tree["multicast-links"].length != 0)
        {
            CTX.beginPath();
            Net.multicast_tree["multicast-links"].forEach (function (link) {
                var host1, host2;
                var port1, port2;
                var net_link;

                // Link to server
                if(link.source_switch == 0) {
                    host1 = Net.getHostByID(link.target_switch); if (!host1) return;
                    port1 = host1.getPort(link.target_port);

                    net_link = Net.getLinkByPort(host1.id, link.target_port); if (!net_link) return;
                    if (net_link.host1.mode == 'CISCO' || net_link.host2.mode == 'CISCO') {
                        server_cisco = net_link.host1.mode == 'CISCO' ? net_link.host1 : net_link.host2;
                        draw_line(port1, server_cisco);

                        var cisco_link = Net.getLinkByPort(server_cisco.id, 'multicast_port');
                        host2 = cisco_link.host1.mode == 'CISCO' ? cisco_link.host2 : cisco_link.host1; if (!host2) return;
                        draw_line(server_cisco, host2);
                    } else {
                        host2 = net_link.host1.id == host1.id ? net_link.host2 : net_link.host1; if (!host2) return;
                        draw_line(port1, host2);
                    }

                }
                // Link to listener
                else if(link.target_switch == 0) {
                    host1 = Net.getHostByID(link.source_switch); if (!host1) return;
                    port1 = host1.getPort(link.source_port);

                    net_link = Net.getLinkByPort(host1.id, link.source_port); if (!net_link) return;
                    if (net_link.host1.mode == 'CISCO' || net_link.host2.mode == 'CISCO') {
                        server_cisco = net_link.host1.mode == 'CISCO' ? net_link.host1 : net_link.host2;
                        draw_line(port1, server_cisco);

                        var cisco_link = Net.getLinkByPort(server_cisco.id, 'multicast_port');

                        host2 = cisco_link.host1.mode == 'CISCO' ? cisco_link.host2 : cisco_link.host1; if (!host2) return;
                        draw_line(server_cisco, host2);
                    } else {
                        host2 = net_link.host1.id == host1.id ? net_link.host2 : net_link.host1; if (!host2) return;
                        draw_line(port1, host2);
                    }
                }
                // Link between switches
                else {
                    host1 = Net.getHostByID(link.source_switch); if (!host1) return;
                    host2 = Net.getHostByID(link.target_switch); if (!host2) return;

                    port1 = host1.getPort(link.source_port);
                    port2 = host2.getPort(link.target_port);

                    draw_line(port1, port2);
                }

                function draw_line(h1, h2) {
                    if (!h1 || !h2) return;

                    var colors = [],
                        group_address = parseInt(Net.multicast_tree["multicast-address"]),
                        tree_color = Color.fullred;

                    for(var key in Color) {
                        colors.push(Color[key]);
                    }

                    tree_color = colors[group_address % colors.length];

                    // Joints between lines
                    CTX.fillStyle = tree_color;
                    CTX.globalAlpha = 0.8;
                    CTX.fillRect(h1.x - line_width/2,
                                 h1.y - line_width/2,
                                 line_width,
                                 line_width);
                    CTX.fillRect(h2.x - line_width/2,
                                 h2.y - line_width/2,
                                 line_width,
                                 line_width);

                    // Lines
                    CTX.moveTo(h1.x, h1.y);
                    CTX.lineTo(h2.x, h2.y);
                }
            });

            var colors = [],
                group_address = parseInt(Net.multicast_tree["multicast-address"]),
                tree_color = Color.fullred;

            for(var key in Color) {
                colors.push(Color[key]);
            }

            tree_color = colors[group_address % colors.length];

            CTX.lineWidth = line_width;
            CTX.fillStyle = tree_color;
            CTX.strokeStyle = tree_color;
            CTX.globalAlpha = 0.8;
            CTX.stroke();
        }
        CTX.restore();
    }
/* **** UI multicast END **** */

/* **** UI Endpoint **** */
    function endpointStats(port) {
        UI.end_point.stats = {};
        var bytes_in = 0, bytes_out = 0,
            pkts_in = 0, pkts_out = 0;
        port.end_point.forEach(function(point) {
            var spl = point.split("-");
            var name = "";
            //get domain name
            for (var i = 0; i < spl.length - 3; i++) {
                if (i > 0) name += '-';
                name += spl[i+1];
            }

            var domain = Net.getDomainByName(name);
            if (!domain) return;
            if (!domain.isSelected) return;
            if (domain.mode == "P2P-Group" || domain.mode == "B2C-Group") {
                if (UI.menu.querySelector('.endpoint_stats')) {
                    UI.menu.querySelector('.endpoint_stats').onclick = function () { showGroupEndpointStatsMenu(domain, port); }
                }
            } else {
                if (UI.menu.querySelector('.endpoint_stats')) {
                    UI.menu.querySelector('.endpoint_stats').onclick = function () {
                        StaticEndpoint.showMenuStatistics(point, port);
                    }
                }
                Server.ajax("GET", "/bucket/"+point+"-in/", callbackEndpointStatsIn);
                Server.ajax("GET", "/bucket/"+point+"-out/", callbackEndpointStatsOut);
            }
        });

        if (UI.menu.style.display == 'block' && UI.end_point.port == port)
            setTimeout(function() {endpointStats(port)}, 1000);

        function callbackEndpointStatsIn(response) {
            bytes_in += Number(response["integral"]["bytes"]);
            pkts_in += Number(response["integral"]["packets"]);
            if (UI.menu.querySelector('.in')) {
                UI.menu.querySelector('.in').innerHTML = 'IN : ' + pkts_in + ' pkts / ' + utils.parseNetworkValue(bytes_in);
            }
        }
        function callbackEndpointStatsOut(response) {
            bytes_out += Number(response["integral"]["bytes"]);
            pkts_out += Number(response["integral"]["packets"]);
            if (UI.menu.querySelector('.out')) {
                var b  = bytes_out,
                    kb = (b / 1000).toFixed(1),
                    mb = (kb / 1000).toFixed(1);
                UI.menu.querySelector('.out').innerHTML = 'OUT : ' + pkts_out + ' pkts / ' +
                    (mb > 0 ? mb + ' MB' : (kb > 0 ? kb + ' KB' : b + ' B'));
            }
        }
    }

    function showEndpointStats() {

    }

    function showGroupEndpointStatsMenu (domain, port) {
        var start, end, point;
        domain.endpoints.forEach(function(endp) {
            if (endp.port != port) return;

            start = Number(endp.stag_start);
            end = Number(endp.stag_end);
            point = endp;
        });

        if (!start || !end || !point) {
            return;
        }

        var html = '';
        html += '<table class="arccn">';
        html += '<thead><tr>';
        html += '<th colspan="3">Info</th>';
        html += '</tr></thead>';
        html += '<tbody>';
        html += '<tr style="font-weight:bold"><td>DPID</td><td>Port</td><td>Domain name</td></tr>';
        html += '<tr>';
        html += '<td>'+ port.router.id +'</td>';
        html += '<td>' + port.name +' (' + port.of_port +')</td>';
        html += '<td>'+ domain.name +'</td>';
        html += '</tr>';
        html += '</tbody>';
        html += '</table>';

        html += '<table class="arccn">';
        html += '<thead><tr>';
        html += '<th colspan="4">Domain Port Statistics</th>';
        html += '</tr></thead>';
        html += '<tbody>';
        html += '<tr style="font-weight:bold">';
            html += '<td>VLAN</td>';
            html += '<td>Current-IN</td>';
            html += '<td>Current-OUT</td>';
            html += '<td>Summary-IN</td>';
            html += '<td>Summary-OUT</td>';
        html += '</tr>';

        for (var i = start; i <= end; i++) {
            html += '<tr class="endpoint endpoint'+i+'" data-name="' + point.name + '-' + i + '">';
            html += '<td>' + i + '</td>';
            html += '<td id="end-'+i+'-cur-in">XXX</td>';
            html += '<td id="end-'+i+'-cur-out">XXX</td>';
            html += '<td id="end-'+i+'-sum-in">XXX</td>';
            html += '<td id="end-'+i+'-sum-out">XXX</td>';
            html += '</tr>';

            getVLANStats(i);
        }

        html += '</tbody>';
        html += '</table>';

        var menu = createMenu(html, {width: "50%", pos: "center"});
        var eEndpoints = menu.querySelectorAll('.endpoint');
        Array.prototype.forEach.call(eEndpoints, function(eEndpoint) {
            eEndpoint.onclick = function() {
                var name = this.getAttribute('data-name');
                StaticEndpoint.showMenuStatistics(name, port);
            }
        });

        function getVLANStats (vlan) {
            //console.log("get stats on", vlan);
            Server.ajax("GET", "/bucket/"+point.name+"-"+vlan+"-in/", callbackVLANStatsIn);
            Server.ajax("GET", "/bucket/"+point.name+"-"+vlan+"-out/", callbackVLANStatsOut);

            setTimeout(function() { getVLANStats(vlan); }, 1000);

            function callbackVLANStatsIn (response) {
                var int = Number(response["integral"]["bytes"]);
                var cur = Number(response["current-speed"]["bytes"]);

                menu.element("end-"+vlan+"-sum-in").innerHTML = parseBytes(int);
                menu.element("end-"+vlan+"-cur-in").innerHTML = parseBytes(cur);
            }

            function callbackVLANStatsOut (response) {
                var int = Number(response["integral"]["bytes"]);
                var cur = Number(response["current-speed"]["bytes"]);

                menu.element("end-"+vlan+"-sum-out").innerHTML = parseBytes(int);
                menu.element("end-"+vlan+"-cur-out").innerHTML = parseBytes(cur);
            }

            function parseBytes(bytes) {
                var kb = (bytes / 1000).toFixed(1),
                    mb = (kb / 1000).toFixed(1),
                    gb = (mb / 1000).toFixed(1);
                if (gb > 0) return gb + ' GB';
                if (mb > 0) return mb + ' MB';
                if (kb > 0) return kb + ' KB';
                return bytes + ' B';
            }
        }
    }
/* **** UI Endpoint END **** */


/* **** UI DpidChecker **** */
    function showDpidCheckerMenu () {
        if (getMenu("roles_menu")) {
            removeMenu("roles_menu");
            return;
        }

        Server.ajax("GET", "/switches/role/", fillTable, onError);

        function onError (response) {
            UI.createHint("Internal error or no connection with controller", {fail: true});
        }

        function fillTable (response) {
            roles = {};

            function form_obj (type) {
                if (!response[type]) return;
                response[type].forEach(function(elem) {
                    roles[elem] = type;
                });
            }

            form_obj('DR');
            form_obj('AR');
            Object.keys(roles).sort();

            var html = '';
            html = '<table class="arccn switch_roles_table">'
                 + '<thead><tr>'
                 + '<th colspan="4">Switch Roles</th>'
                 + '</tr></thead>'
                 + '<tbody>'
                 + '<tr class="bold"><td>DPID</td><td>Name</td><td>Role</td><td></td></tr>';
            for (var dpid in roles) {
                var name = Server.getCookie("host_name_" + dpid) ||
                          (Net.getHostByID(dpid) ? Net.getHostByID(dpid).name : '');
                html += '<tr id="'+dpid+'">'
                      + '<td>'+dpid+'</td>'
                      + '<td class="active">'+name+'</td>'
                      + '<td>'+roles[dpid]+'</td>'
                      + '<td><i class="delete" dpid="'+dpid+'"></i></td>'
                      + '</tr>';
            }
            html += '</tbody>'
                  + '</table>'
                  + '<button class="half blue" id="new_role">New switch role</button>';

            var menu = createMenu(html, {name: "roles_menu", unique: true, width: "600px"});

            make_acts();

            function make_acts () {
                var dels = menu.querySelectorAll('.delete');
                for (var i = 0, len = dels.length; i < len; i++) {
                    dels[i].onclick = on_delete_switch;
                }

                var act = menu.querySelectorAll('.active');
                for (var i = 0, len = act.length; i < len; i++) {
                    act[i].onclick = on_change_name;
                }

                var button = menu.element('new_role');
                button.onclick = add_line;
            }

            function add_line () {
                if (menu.element('new_line')) return;
                var html;
                html  = '<tr id="new_line">'
                      + '<td><input type="text" id="dpid"></td>'
                      + '<td class="active"><input type="text" id="name"></td>'
                      + '<td><select id="role">'
                      +     '<option value="ar">AR</option>'
                      +     '<option value="dr">DR</option>'
                      + '</select></td>'
                      + '<td><i class="apply"></td>'
                      + '</tr>';

                var body = menu.querySelector('.switch_roles_table').querySelector('tbody');
                body.insertAdjacentHTML('beforeEnd', html);
                var tr = body.querySelector('#new_line');
                tr.querySelector('#dpid').onkeypress = check_digit;
                tr.querySelector('.apply').onclick = function () {
                    var obj = {};

                    obj["dpid"] = tr.querySelector('#dpid').value;
                    obj["name"] = tr.querySelector('#name').value;
                    obj["role"] = tr.querySelector('#role').value;

                    if (obj["dpid"]) {
                        Server.ajax("PUT", '/switches/role/', obj, onAdd);
                    }

                    function onAdd (response) {
                        UI.createHint(response.description, response.result != "done");
                        var new_elem = menu.element('new_line');
                        if (response.result == "done") {
                            var dpid = obj["dpid"];
                            var role = obj["role"].toUpperCase()
                            new_elem.id = dpid;
                            new_elem.children[0].innerHTML = dpid;
                            new_elem.children[1].innerHTML = obj["name"];
                            new_elem.children[2].innerHTML = role;
                            new_elem.children[3].innerHTML = '<i class="delete" dpid="'+dpid+'"></i>';
                            new_elem.querySelector('.active').onclick = on_change_name;
                            new_elem.querySelector('.delete').onclick = on_delete_switch;

                            apply_new_name(dpid, obj["name"]);

                            var host = Net.getHostByID(dpid);
                            if (host) {
                                host.changeRole(role);
                            }
                        } else {
                            new_elem.parentElement.removeChild(new_elem); // KMP
                        }
                    }
                }
            }

            function check_digit (e) {
                if (e.ctrlKey) return true;
                if (e.which < 32) return true;
                var ch = String.fromCharCode(e.which);
                var filter = /[0-9]/;
                if (!filter.test(ch)) {
                    createHint("Please use only digits", {fail: true});
                    e.preventDefault();
                    return false;
                }
            }

            function on_change_name () {
                var td = this;
                var dpid = this.parentElement.id;
                this.innerHTML = '<input type="text" id="new_name'+dpid+'" value="'+this.innerHTML+'">';
                this.onclick = '';
                var apply = this.parentElement.querySelector('i');
                apply.className = "apply";
                apply.onclick = function () {
                    apply.className = "delete";
                    apply.onclick = on_delete_switch;

                    var new_name = menu.element('new_name' + dpid).value;
                    td.innerHTML = new_name;
                    td.onclick = on_change_name;
                    apply_new_name(dpid, new_name);
                }
            }

            function apply_new_name (dpid, name) {
                Server.setCookie("host_name_" + dpid, name);
                if (Net.getHostByID(dpid)) {
                    Net.getHostByID(dpid).name = name;
                }
            }

            function on_delete_switch () {
                var tr = this.parentElement.parentElement;
                var dpid = this.getAttribute("dpid");
                menu.confirm(ok_cb, cancel_cb, "Are you sure you want to delete switch #" + dpid + "?");

                function ok_cb () {
                    Server.ajax("DELETE", '/switches/role/' + dpid + '/', onDelete);

                    function onDelete (response) {
                        UI.createHint(response.description, response.result != "done");
                        if (response.result == "done") {
                            tr.parentElement.removeChild(tr); // KMP
                            Net.del(Net.getHostByID(dpid));
                        }
                    }
                }
                function cancel_cb () { }
            }
        }
    }
/* **** UI DpidChecker END **** */

    function showContextMenu (hovered, x, y) {
        var html = '';
        if (hovered.type === 'router') {
            html += '<li>' + hovered.name + '</li>';
            html += '<li>DPID: ' + hovered.id + '</li>';
            html += '<li>IP: ' + hovered.peer.substring(0, hovered.peer.indexOf(':')) + '</li>';
            html += '<li class="separator"></li>';
            html += '<li class="action settings">Router Settings</li>';
            html += '<li class="action flow_tables">Flow Tables</li>';
            html += '<li class="action group_table">Group Table</li>';
            html += '<li class="action meter_table">Meter Table</li>';
            html += '<li class="action dos_control">DOS Control</li>';
            html += '<li class="separator"></li>';
            html += '<li class="action change_name">Change name</li>';
            if (hovered.mode == "AR") { // inband only for ARs
                html += '<li class="separator"></li>';
                html += '<li class="action in_band_routes">In-band routes<i class="' + (hovered.show_inband ? 'on' : 'off') + '"></i></li>';
            }
        } else if (hovered.type === 'link') {
            if (hovered.load) {
                html += '<li>Load: ' + (hovered.load / hovered.bandwidth * 100).toFixed(2) + '%</li>';
            }
            html += '<li>Bandwidth: ' + (hovered.bandwidth / 1e6) + 'Gb</li>';
        } else if (hovered.type === 'host') {
            if (hovered.mode === 'CISCO') {
                html += '<li>' + hovered.name + '</li>';
                html += '<li>' + hovered.platform + '</li>';
                html += '<li class="separator"></li>';
                html += '<li>Capabilities: ' + hovered.cap + '</li>';
            } else if (hovered.mode === 'AUX') {
                html += '<li>' + hovered.name + '</li>';
                if (hovered.desc) {
                    html += '<li class="separator"></li>';
                    html += '<li>' + hovered.desc + '</li>';
                }
                html += '<li class="separator"></li>';
                html += '<li class="action remove_aux_device">Remove</li>';
            } else {
                html += '<li>' + hovered.name + '</li>';
                html += '<li>' + hovered.ip + '</li>';
                html += '<li class="separator"></li>';
                html += '<li class="action mcast_settings">Host Settings</li>';
            }
        } else if (hovered.type === 'port') {
            hovered.getPortInfo();
            if (hovered.isLAG) {
                html += '<li>' + hovered.name + ' [' + hovered.router.lag[hovered.of_port].ports + ']</li>';
                hovered.router.lag[hovered.of_port].ports.forEach(function(port) {
                    html += '<li>Port ' + port + '</li>';
                    html += '<li class="action port_stats" data-id="' + port + '">Statistics</li>';
                    html += '<li class="action port_settings" data-id="' + port + '">Settings</li>';
                    html += '<li class="action queues_settings" data-id="' + port + '">Queues Settings</li>';
                    html += '<li class="action queues" data-id="' + port + '">Queue Statistics</li>';
                    html += '<li class="separator"></li>';
                });
                html += '<li class="action lag_details"> LAG details </li>';
                html += '<li class="action delete_lag">Delete LAG</li>';
            }
            else {
                html += '<li>' + hovered.name + '</li>';

                html += '<li class="action port_settings">Port Settings</li>';

                html += '<li class="action port_stats">Port Statistics</li>';
                html += '<li class="action queues_settings">Queues Settings</li>';
                html += '<li class="action queues">Queue Statistics</li>';
                html += '<li class="separator"></li>';
                html += '<li class="action add_to_lag">Add to LAG</li>';
                html += '<li class="action add_to_mirror"> Add to Mirror </li>';
                html += '<li class="action add_device"> Add Device </li>';
                html += '<li class="separator"></li>';
                html += '<li class="action storm">Storm control' + (hovered.storm ? ' (' + hovered.storm + '%)' : '') + '<i class="' + (hovered.storm ? 'on' : 'off') + '"></i></li>';
            }
            if (hovered.isDomainOn) {
                html += '<li class="separator"></li>';
                html += '<li class="action endpoint_stats">Endpoint Statistics</li>';
                html += '<li class="in"></li>';
                html += '<li class="out"></li>';
                if (hovered.end_point != undefined) {
                    setTimeout(function() {endpointStats(hovered)}, 100);
                    this.end_point.port = hovered;
                    this.end_point.stats = {};
                }
                else {
                    html += '<li>NO ENDPOINTS!</li>';
                }
            }
        } else if (hovered.type === 'domain') {
            html += '<li>' + hovered.name + '</li>';
            html += '<li>Type: ' + hovered.mode + '</li>';
            html += '<li class="separator"></li>';
            html += '<li class="action domain_routes">Routes</li>';
            if (hovered.mode == 'B2C' || hovered.mode == 'B2C-Group' || hovered.mode == 'MP') {
                html += '<li class="action mac_port_list">MAC list</li>';
            }
            html += '<li class="action domain_delete">Delete</li>';
        }

        UI.menu.innerHTML = html;
        var ePortStatsActions = UI.menu.querySelectorAll('.port_stats'),
            ePortSettingsActions = UI.menu.querySelectorAll('.port_settings'),
            ePortQueuesSettingsActions = UI.menu.querySelectorAll('.queues_settings'),
            ePortQueuesActions = UI.menu.querySelectorAll('.queues');

        var eChangeName = UI.menu.querySelector('.change_name');

        if (eChangeName) { eChangeName.addEventListener('click', function() { hovered.showChangeNameMenu()})};

        if (UI.menu.querySelector('.remove_aux_device')) { UI.menu.querySelector('.remove_aux_device').onclick = removeAuxDevice; }
        if (UI.menu.querySelector('.mcast_settings')) { UI.menu.querySelector('.mcast_settings').onclick = showHostDetails; }
        if (UI.menu.querySelector('.mac_port_list')) {
            UI.menu.querySelector('.mac_port_list').onclick = function() { showDomainMacPortList(hovered) };
        }
        if (ePortSettingsActions.length > 0) {
            if (ePortSettingsActions.length == 1) {
                ePortSettingsActions[0].onclick = function() {
                    showPortDetails();
                }
            } else {
                Array.prototype.forEach.call(ePortSettingsActions, function(ePortSettingsAction) {
                    var port = Net.getPort(hovered.router.id, ePortSettingsAction.getAttribute('data-id'));
                    ePortSettingsAction.onclick = function() {
                        showPortDetails(port);
                    };
                })
            }
        }
        if (UI.menu.querySelectorAll('.port_stats').length > 0)    {
            if (ePortStatsActions.length == 1) {
                ePortStatsActions[0].onclick = function() {
                    showPortStats();
                }
            } else {
                Array.prototype.forEach.call(ePortStatsActions, function(ePortStatsAction) {
                    var port = Net.getPort(hovered.router.id, ePortStatsAction.getAttribute('data-id'));
                    ePortStatsAction.onclick = function() {
                        showPortStats(port);
                    };
                })
            }
        }
        if (ePortQueuesSettingsActions.length > 0) {
            if (ePortQueuesSettingsActions.length == 1) {
                ePortQueuesSettingsActions[0].onclick = function() {
                    showQueuesDetails();
                }
            } else {
                Array.prototype.forEach.call(ePortQueuesSettingsActions, function(ePortQueuesSettingsAction) {
                    var port = Net.getPort(hovered.router.id, ePortQueuesSettingsAction.getAttribute('data-id'));
                    ePortQueuesSettingsAction.onclick = function() {
                        showQueuesDetails(false, port);
                    };
                })
            }
        }
        if (ePortQueuesActions.length > 0) {
            if (ePortQueuesActions.length == 1) {
                ePortQueuesActions[0].onclick = function() {
                    showQueues();
                }
            } else {
                Array.prototype.forEach.call(ePortQueuesActions, function(ePortQueuesAction) {
                    var port = Net.getPort(hovered.router.id, ePortQueuesAction.getAttribute('data-id'));
                    ePortQueuesAction.onclick = function() {
                        showQueues(port);
                    };
                })
            }
        }

        if (hovered.type === 'router') {
            if (UI.menu.querySelector('.settings')) { UI.menu.querySelector('.settings').onclick = showDetails; }
            if (UI.menu.querySelector('.flow_tables')) { UI.menu.querySelector('.flow_tables').onclick = showFlowTables; }
            if (UI.menu.querySelector('.group_table')) { UI.menu.querySelector('.group_table').onclick = showGroupTable; }
            if (UI.menu.querySelector('.meter_table')) { UI.menu.querySelector('.meter_table').onclick = showMeterTable; }
            if (UI.menu.querySelector('.dos_control')) { UI.menu.querySelector('.dos_control').onclick = showDosControl; }
            if (UI.menu.querySelector('.in_band_routes')) { UI.menu.querySelector('.in_band_routes').onclick = showInBandRoutesDetails; }
        }

        else if (hovered.type === 'domain') {
            if (UI.menu.querySelector('.domain_delete')) { UI.menu.querySelector('.domain_delete').onclick = function () { deleteDomain(hovered.name) }; }
            if (UI.menu.querySelector('.domain_routes')) { UI.menu.querySelector('.domain_routes').onclick = function () { showDomainRoutes(hovered) }; }
        }

        else if (hovered.type == 'port') {
            if (!hovered.isLAG) {
                if (UI.menu.querySelector('.add_to_lag'))    { UI.menu.querySelector('.add_to_lag').onclick    = function () {showNewLag(hovered) } }
            } else {
                var lag = hovered.router.lag[hovered.of_port];
                if (lag) {
                    if (UI.menu.querySelector('.delete_lag'))    { UI.menu.querySelector('.delete_lag').onclick    = function () { hovered.router.lag[hovered.of_port].delLag(); } }
                    if (UI.menu.querySelector('.lag_details'))  { UI.menu.querySelector('.lag_details').onclick  = function () {showLagDetails(hovered) } }
                }
            }
            if (UI.menu.querySelector('.add_to_mirror')) { UI.menu.querySelector('.add_to_mirror').onclick = function() { showNewMirror(hovered); } }
            if (UI.menu.querySelector('.add_device')) { UI.menu.querySelector('.add_device').onclick = function() { showNewDevice(hovered); } }
            if (UI.menu.querySelector('.storm')) { UI.menu.querySelector('.storm').onclick = function () {selectStorm(hovered); } }
        }

        UI.menu.style.display = 'block';
        UI.menu.style.left = ((x + UI.menu.offsetWidth + UI.canvas.getBoundingClientRect().left < window.innerWidth) ? x : window.innerWidth - UI.menu.offsetWidth - UI.canvas.getBoundingClientRect().left) + 'px';
        UI.menu.style.top = ((y + UI.menu.offsetHeight + UI.canvas.getBoundingClientRect().top < window.innerHeight) ? y : window.innerHeight - UI.menu.offsetHeight - UI.canvas.getBoundingClientRect().top) + 'px';
    }

/* **** UI Storm Control **** */
    function selectStorm(hovered) {
        if (hovered) {
            removeMenu("storm_menu");
            if (!hovered.storm) {
                var html = '';
                html += '<input type="text" id="level" style="width:110px;" placeholder="Control level (%)">';
                html += '<button type="button" style="width:60px;" id="add_storm" class="blue">Create</button>';
                html += '<button type="button" style="width:60px;" id="no_storm" class="rosy">Cancel</button>';
                var menu = createMenu(html, {name: "storm_menu"});
                menu.style.left = ((hovered.x + menu.offsetWidth + UI.canvas.getBoundingClientRect().left < window.innerWidth) ? hovered.x : window.innerWidth - UI.menu.offsetWidth - UI.canvas.getBoundingClientRect().left - 150) + 'px';
                menu.style.top = ((hovered.y + menu.offsetWidth + UI.canvas.getBoundingClientRect().left < window.innerWidth) ? hovered.y : window.innerWidth - UI.menu.offsetWidth - UI.canvas.getBoundingClientRect().left - 150) + 'px';
                menu.element('add_storm').onclick = createStormControl;
                menu.element('no_storm').onclick =  menu.remove;
            }
            else {
                Server.ajax("DELETE", "/storm-control/"+hovered.router.id+"/"+hovered.of_port+"/");
                UI.createHint("Storm control on " +hovered.router.id+":"+hovered.of_port+ " removed!");
                hovered.storm = hovered.storm_is_active = false;
                var t = UI.menu.querySelector('.action.storm');
                t.querySelector('i').className = 'off';
            }
            Net.need_draw = true;
        }

        function createStormControl () {
            var param = {},
                lvl = parseInt(menu.element('level').value);
            if (isNaN(lvl) || lvl < 0 || lvl > 100) {
                menu.element('level').style.color = 'red';
                return;
            }
            param["level"] = lvl;
            Server.ajax("PUT", "/storm-control/"+hovered.router.id+"/"+hovered.of_port+"/", param, function (response) {
                if (response['error']) {
                    UI.createHint("Storm control " + response['error'], {fail: true});
                    return;
                }
                if (response['storm-control'] == 'error') {
                    UI.createHint("Storm control error", {fail: true});
                    return;
                }
                UI.createHint("Storm control on " +hovered.router.id+":"+hovered.of_port+ " with lvl=" + lvl + "% added!");
                hovered.storm = lvl;
                Net.need_draw = true;
            });
            menu.remove();
        }
    }
/* **** UI Storm Control END **** */

/* **** UI Menu abstraction **** */
    /* props -- properties:
     * animation: enable animation for menu
     * pos: "left", "center", "right"
     * vpos: "top", "center", "bottom"
     * width: value
     * name: className of the menu
     * title: title of the menu
     * unique: unique menu?
     * ev_handlers: function, calling for setting events for menu
     * cl_callback: when X pressed
     * hide_callback: when _ pressed
     */
    function createMenu (html, props) {
        var header = '';
        props = props || {};
        props.animation = (typeof(props.animation) == 'boolean' ? props.animation : true);

        if (props.unique && props.name) {
            var menu = document.querySelector('.'+props.name);
            if (menu) return;
        }

        if (props.title)
            header += '<h2 id="title">' + props.title + '</h2>';
        header += '<a class="close_menu"></a>';
        header += '<a class="hide_menu"></a>';

        var menu = document.createElement('div');
        document.body.appendChild(menu);
        menu.innerHTML = header + '<div class="menu_wrapper">' + html + '</div>';

        menu.statics = {};

        // initial events for all menu elements
        function initialEvents () {
            menu.querySelector('.close_menu').onclick = function(event) {
                if (props.cl_callback && typeof(props.cl_callback) == "function")
                    props.cl_callback();
                menu.remove();
                event.stopPropagation();
            }
            menu.querySelector('.hide_menu').onclick = function(e) {
                if (props.hide_callback && typeof(props.hide_callback) == "function")
                    props.hide_callback();
                hideMenu(e);
                event.stopPropagation();
            }
            menu.querySelector('.close_menu').onmousedown = function (event) {
                event.stopPropagation();
            }
            menu.querySelector('.hide_menu').onmousedown = function (event) {
                event.stopPropagation();
            }

            var tbl_size = menu.querySelectorAll('.arccn').length;
            for (var i = 0; i < tbl_size; i++) {
                menu.querySelectorAll('.arccn')[i].onmousemove = function (event) {
                    event.stopPropagation();
                }
                menu.querySelectorAll('.arccn')[i].onmousedown = function (event) {
                    event.stopPropagation();
                }
            }

            var inputs = menu.querySelectorAll('input') || [];
            var select = menu.querySelectorAll('select') || [];
            var button = menu.querySelectorAll('button') || [];
            for (var i = 0; i < inputs.length; i++) {
                inputs[i].onmousemove = function (event) {
                    event.stopPropagation();
                }
                inputs[i].onmousedown = function (event) {
                    event.stopPropagation();
                }
            }
            for (var i = 0; i < select.length; i++) {
                select[i].onmousemove = function (event) {
                    event.stopPropagation();
                }
                select[i].onmousedown = function (event) {
                    event.stopPropagation();
                }
            }
            for (var i = 0; i < button.length; i++) {
                button[i].onmousemove = function (event) {
                    event.stopPropagation();
                }
                button[i].onmousedown = function (event) {
                    event.stopPropagation();
                }
            }
        } // end initialEvents()

        initialEvents();
        if (props.ev_handlers)
            props.ev_handlers(menu);

        setFocus();

        if (Net.settings.animation && props.animation) {
            //setStyle(menu, 'animation', 'open 0.5s');
            menu.style["animation"] = "open 0.5s";
        }

        menu.disable = function () {
            menu.style["pointer-events"] = 'none';
            menu.style.opacity = 0.5;
        }

        menu.enable = function () {
            menu.style["pointer-events"] = 'all';
            menu.style.opacity = 1;
        }

        menu.element = function (id) {
            return menu.querySelector('#'+id);
        }

        menu.remove = function () {
            if (menu) {
                menu.deleteLater = true;
                var pos = UI.activeMenuList.indexOf(menu);
                UI.activeMenuList.splice(pos, 1);
                if (UI.activeMenuList.length > 0)
                    makeActive(UI.activeMenuList[0]);

                if (Net.settings.animation && props.animation) {
                    //setStyle(menu, 'animation', 'close 0.5s');
                    menu.style["animation"] = "close 0.5s";
                    setTimeout(function () {document.body.removeChild(menu);}, 450);
                } else {
                    document.body.removeChild(menu);
                }
            }
        }

        menu.confirm = function (ok_cb, cancel_cb, msg) {
            msg = msg || "Are you sure?";
            menu.disable();

            var html = '';
            html += '<p>'+msg+'</p><br>';
            html += '<button type="button" id="yep" class="blue half">Yes</button>';
            html += '<button type="button" id="nope" class="rosy half">No</button>';

            var confirm_menu = createMenu(html, { name: 'confirm',
                                                  width: '200px',
                                                  pos: 'center',
                                                  vpos: 'center',
                                                  title: 'The confirmation',
                                                  unique: true,
                                                  cl_callback: function () { if (typeof cancel_cb == 'function') cancel_cb(); menu.enable(); }
                                                 });
            confirm_menu.style.left = menu.offsetLeft + menu.offsetWidth/2 - confirm_menu.offsetWidth/2 + 'px';
            confirm_menu.style.top = menu.offsetTop + menu.offsetHeight/2 - confirm_menu.offsetHeight/2 + 'px';

            confirm_menu.element('yep').onclick = function () {
                if (typeof ok_cb == 'function') ok_cb();
                confirm_menu.remove();
                menu.enable();
            }
            confirm_menu.element('nope').onclick = function () {
                if (typeof cancel_cb == 'function') cancel_cb();
                confirm_menu.remove();
                menu.enable();
            }
        }

        menu.update = function (newHtmlMaker, updateInterval) {
            var updateFunc = function () {
                menu.innerHTML = header + newHtmlMaker();
                initialEvents();
                if (props.ev_handlers)
                    props.ev_handlers(menu);
            };

            if (updateInterval && updateInterval > 50) {
                setInterval(updateFunc, updateInterval);
            } else {
                updateFunc();
            }
        }

        function hideMenu(e) {
            var WIDTH = 100, OFFSET = 20;
            var menuWidth = menu.offsetWidth,
                menuHeight = menu.offsetHeight,
                coof = WIDTH/menuWidth;

            var oldTop, oldLeft;
            var smallHeight = menuHeight * coof;

            var overlay = document.createElement('div');
            // TODO: move to css
            overlay.style.cursor = 'pointer';
            overlay.style.position = 'absolute';
            overlay.style.display = 'block';
            overlay.style.zIndex = '100';
            overlay.style.left = 0;
            overlay.style.top = '-20px';
            overlay.style.width = menuWidth + 20 + 'px';
            overlay.style.height = menuHeight + 20 + 'px';

            var pos = UI.activeMenuList.indexOf(menu);
            UI.activeMenuList.splice(pos, 1);
            if (menu.element('title'))
                menu.element('title').classList.remove("active");
            if (UI.activeMenuList.length)
                makeActive(UI.activeMenuList[0]);

            overlay.onclick = function(e) {
                console.log('clock click');
                e.stopPropagation();
                e.preventDefault();
            }

            function onOverlayClick() {
                var offsetTop = menu.offsetTop;

                menu.style.transform = 'scale(1)';
                menu.style.top = oldTop + 'px';
                menu.style.left = oldLeft + 'px';
                menu.style.right = 'auto';

                menu.removeChild(overlay);
                menu.classList.remove('window-hidden');

                hideItemsOffsetTop -= (smallHeight  + OFFSET);

                var eHiddens = document.querySelectorAll('.window-hidden');
                Array.prototype.forEach.call(eHiddens, function(eHidden) {
                   if (eHidden.offsetTop > offsetTop) {
                       eHidden.style.top = eHidden.offsetTop - (smallHeight  + OFFSET) + 'px';
                   }
                });

                overlay.removeEventListener('click', onOverlayClick);

                makeActive(menu);
            }

            function hide(e) {
                oldTop = menu.offsetTop;
                oldLeft = menu.offsetLeft;

                menu.style.transform = 'scale(' + coof + ')';
                menu.style.transformOrigin = '100% 0';
                menu.style.top = hideItemsOffsetTop + 'px';
                menu.style.right = 10 + 'px';
                menu.style.left = 'auto';

                menu.classList.add('window-hidden');

                menu.appendChild(overlay);
                overlay.addEventListener('click', onOverlayClick);

                hideItemsOffsetTop += (smallHeight + OFFSET);

                e.stopPropagation();
            };

            hide(e);
        };

        menu.onmousedown = function (event) {
            makeActive(menu);
            var mouse = {};
            mouse.drag = true;
            var all_menu = document.querySelectorAll('.somemenu');
            Array.prototype.forEach.call(all_menu, function(m) {
                if (m != menu) {
                    m.disable();
                }
            });

            mouse.shiftX = event.clientX - menu.offsetLeft;
            mouse.shiftY = event.clientY - menu.offsetTop;

            window.onmousemove = function (event) {
                if (mouse.drag) {
                    menu.style.left = -mouse.shiftX + event.clientX + 'px';
                    menu.style.top = -mouse.shiftY + event.clientY  + 'px';
                }
            };

            window.onmouseup = function() {
                mouse.drag = false;
                window.onmousemove = '';
                var all_menu = document.querySelectorAll('.somemenu');
                Array.prototype.forEach.call(all_menu, function(m) {
                    m.enable();
                });
            }
        };

        makeActive(menu);

        function makeActive (active) {
            active.style["z-index"] = 1;
            if (active.deleteLater) return;
            var prev_active = UI.activeMenuList[0] || {};
            if (prev_active != active) {
                if (UI.activeMenuList.length > 0) {
                    prev_active.style["z-index"] = 0;
                    if (prev_active.element('title'))
                        prev_active.element('title').classList.remove("active");
                }
                var pos = UI.activeMenuList.indexOf(menu);
                if (pos > 0)
                    UI.activeMenuList.splice(pos, 1);
                UI.activeMenuList.unshift(active);
            }
            if (active.element('title'))
                active.element('title').classList.add("active");
        }

        function setFocus() {
            var eFocused = menu.querySelector('.is-focused');
            setTimeout(function() {
                eFocused && eFocused.focus();
            }, 200);
        }

        menu.className = 'somemenu ' + (props.name ? props.name : '');
        menu.style.display = 'block';
        UI.menu.style.display = 'none';
        if (props.width)
            menu.style.width = props.width;

        var width = menu.offsetWidth,
            height = menu.offsetHeight;

        if (!props.pos) props.pos = "left";
        if (props.pos == "left")
            menu.style.left = "185px";
        else if (props.pos == "right")
            menu.style.left = 160 + 2*UI.centerX - width + "px";
        else if (props.pos == "center")
            menu.style.left = 180 + UI.centerX - (width/2).toFixed(0) + "px";

        if (props.vpos == "top")
            menu.style.top = "20px";
        else if (props.vpos == "center")
            menu.style.top = UI.centerY - height/2+"px";
        else if (props.vpos == "bottom")
            menu.style.top = 2*UI.centerY - height+"px";
        return menu;
    }

    function removeMenu (name) {
        var len = document.body.querySelectorAll('.'+name).length;
        for (var i = 0; i < len; i++) {
            var menu = document.body.querySelectorAll('.'+name)[i];
            menu.remove();
        }
    }

    function getMenu (name) {
        return document.body.querySelector('.'+name);
    }


    /* props -- properties:
     * fail: color of the message (true - red, false - green)
     * unique: unique menu?
     */
    function createHint (html, props) {
        props = props || {};

        // For fail hints with undefined 'unique' we select 'true' for this prop
        if (props.fail && props.unique == undefined) {
            props.unique = true;
        }

        if (props.unique) {
            var hints = document.querySelectorAll('.info-hint');
            for (var i = 0, len = hints.length; i < len; ++i) {
                var text = hints[i].querySelector('.info-hint__info').innerHTML;
                if (text == html) {
                    return;
                }
            }
        }

        html = '<p class="info-hint__info">' + html + '</p>';
        var hint = document.createElement('div');
        document.body.appendChild(hint);
        hint.innerHTML = html;

        if (Net.settings.animation) {
            //setStyle(hint, "animation", "open 0.5s");
            hint.style["animation"] = "open 0.5s";
        }

        hint.className = 'info-hint';
        if (props.fail) {
            hint.classList.toggle("info-hint_fail");
        }
        setStyle(hint, 'display', 'block');

        hint.remove = function () {
            var len = document.body.querySelectorAll('.info-hint').length;
            for (var i = 0; i < len; i++) {
                var h = document.body.querySelectorAll('.info-hint')[i];
                if (h == hint) {
                    var shift = 0;
                    for (var j = i+1; j < len; j++) {
                        var hh = document.body.querySelectorAll('.info-hint')[j];
                        //setStyle(hh, "transition", "top 0.5s ease");
                        hint.style["transition"] = "top 0.5s ease";
                        hh.style.top = h.offsetTop + shift + 'px';
                        shift += (hh.offsetHeight + 10);
                    }
                    break;
                }
            }

            if (Net.settings.animation) {
                //setStyle(hint, "animation", "close_hint 0.5s");
                hint.style["animation"] = "close_hint 0.5s";
                setTimeout(function () {document.body.removeChild(hint);}, 500);
            } else {
                document.body.removeChild(hint);
            }
        }

        var shift = 20;
        var len = document.body.querySelectorAll('.info-hint').length;
        if (len > 1) {
            var last = document.body.querySelectorAll('.info-hint')[len-2];
            shift = last.offsetTop + last.offsetHeight + 10;
        }

        hint.style.top = shift + "px";
        var timer = setTimeout(hint.remove, 5000);
        hint.onmouseenter = function () { clearTimeout(timer); }
        hint.onmouseleave = function () { timer = setTimeout(hint.remove, 1000); }
        hint.oncontextmenu = function (event) {
            hint.remove();
            clearTimeout(timer);
            event.preventDefault();
        }
    }
/* **** UI Menu abstraction END **** */

/* **** UI Details and stats **** */
    function showDetails () {
        var hovered = HCI.getHovered();
        var OFPP_LOCAL = 4294967294,
            GORBUNOK_LOCAL = 999;
        Server.ajax('GET', '/switches/' + hovered.id + '/', displayDetails);

        function displayDetails(response) {
            var html = '';

            html += '<table class="arccn">';
            html += '<thead><tr>';
                html += '<th colspan="2">Network Switch Configuration</th>';
                html += '</tr></thead>';
            html += '<tbody>';
                html += '<tr><td>dpid</td><td>'         + response["dpid"] + '</td></tr>';
                html += '<tr><td>peer address</td><td>' + response["peer"] + '</td></tr>';
                html += '<tr><td>status</td><td>'       + response["connection-status"] + '</td></tr>';
                html += '<tr><td>manufacturer</td><td>' + response["manufacturer"] + '</td></tr>';
                html += '<tr><td>hardware</td><td>'     + response["hardware"] + '</td></tr>';
                html += '<tr><td>description</td><td>'  + response["description"] + '</td></tr>';
                html += '<tr><td>ntables</td><td>'      + response["ntables"] + '</td></tr>';
                html += '<tr><td>nbuffers</td><td>'     + response["nbuffers"] + '</td></tr>';
                html += '<tr><td>miss-send-len</td><td>'+ response["miss-send-len"] + '</td></tr>';
                html += '<tr><td>stats</td><td>'        + response["capabilities"]["stats"] + '</td></tr>';
                html += '<tr><td>ports</td><td>'        + (response["ports"] ? response["ports"].filter(function(p) { return p != OFPP_LOCAL && p != GORBUNOK_LOCAL; }) : 'no ports' ) + '</td></tr>';
                html += '<tr><td>maintenance</td><td><input type="checkbox" '+(hovered.maintenance ? 'checked' : '')+' id="switch_maintenance"></td></tr>';
            html += '</tbody>';
            html += '</table>';

            var menu = createMenu(html, {width: "30%", pos: "center"});

            function ok_cb () {
                hovered.setMaintenance(menu.element('switch_maintenance').checked);
            }

            function cancel_cb () {
                menu.element('switch_maintenance').checked = !menu.element('switch_maintenance').checked;
            }

            menu.element('switch_maintenance').onchange = function () { menu.confirm(ok_cb, cancel_cb); }
            }
    }

    function showHostDetails () {
        var hovered = HCI.getHovered();
        var OFPP_LOCAL = 4294967294,
            GORBUNOK_LOCAL = 999;
        var sw_dpid = hovered.sw_dpid,
            sw_port = hovered.sw_port,
            host_name = hovered.name;
        if(sw_dpid === undefined || sw_port === undefined) return;

        if(host_name == "Host")
            Server.ajax('GET', '/multicast_hosts/' + host_name + '/' + sw_dpid + '/' + sw_port + '/', displayListenerDetails);
        else if(host_name == "Server")
            Server.ajax('GET', '/multicast_hosts/' + host_name + '/' + sw_dpid + '/' + sw_port + '/', displayServerDetails);

        function displayListenerDetails(response) {
            var hosts = response["multicast-hosts"];
            var html = '';

            html += '<table class="arccn">';

            html += '<thead>';
                html += '<tr><th><b>Host Configuration</b></th></tr>';
                html += '<tr>';
                html += '<td><b>№</b></td>';
                html += '<td><b>Switch-DPID</b></td>';
                html += '<td><b>Port</b></td>';
                html += '<td><b>MAC-address</b></td>';
                html += '<td><b>Vlan-ID</b></td>';
                html += '<td><b>IP-address</b></td>';
                html += '<td><b>Group name</b></td>';
                html += '<td><b>Group address</b></td>';
                html += '</td>';
                html += '</tr>';
            html += '</thead>';

            html += '<tbody>';
            for (var host_i = 0; host_i < hosts.length; host_i++) {
                var host = hosts[host_i];

                var switch_dpid = host["switch"],
                    port = host["port"],
                    mac_address = host["mac-address"],
                    vlan_id = host["vlan-id"],
                    ip_address = host["ip-address"];
                var group_name = host["group-name"],
                    multicast_address = host["multicast-address"];
                var host_number = host_i + 1;

                html += '<tr>';
                    html += '<td>' + host_number + '</td>';
                    html += '<td>' + switch_dpid + '</td>';
                    html += '<td>' + port + '</td>';
                    /*DEL*///html += '<td>' + utils.intToMAC(mac_address) + '</td>';
                    html += '<td>' + mac_address + '</td>';
                    html += '<td>' + ((vlan_id >= 0 && vlan_id <= 4095) ? vlan_id : "No") + '</td>';
                    /*DEL*///html += '<td>' + utils.intToIP(ip_address) + '</td>';
                    html += '<td>' + ip_address + '</td>';
                    html += '<td>' + group_name + '</td>';
                    /*DEL*///html += '<td>' + utils.intToIP(multicast_address) + '</td>';
                    html += '<td>' + multicast_address + '</td>';
                    html += '</td>';
                html += '</tr>';
            }
            html += '</tbody>';
            html += '</table>';

            createMenu(html, {width: "80%", pos: "center"});
        }

        function displayServerDetails(response) {
            var hosts = response["multicast-hosts"];
            var html = '';

            html += '<table class="arccn">';

            html += '<thead>';
                html += '<tr><th><b>Server Configuration</b></th></tr>';
                html += '<tr>';
                html += '<td><b>№</b></td>';
                html += '<td><b>Switch-DPID</b></td>';
                html += '<td><b>Port</b></td>';
                html += '<td><b>MAC-address</b></td>';
                html += '<td><b>Vlan-ID</b></td>';
                html += '<td><b>IP-address</b></td>';
                html += '<td><b>Group name</b></td>';
                html += '<td><b>Group address</b></td>';
                html += '</td>';
                html += '</tr>';
            html += '</thead>';

            html += '<tbody>';
            for (var host_i = 0; host_i < hosts.length; host_i++) {
                var host = hosts[host_i];

                var switch_dpid = host["switch"],
                    port = host["port"],
                    mac_address = host["mac-address"],
                    vlan_id = host["vlan-id"],
                    ip_address = host["ip-address"];
                var group_name = host["group-name"],
                    multicast_address = host["multicast-address"];
                var host_number = host_i + 1;

                html += '<tr>';
                    html += '<td>' + host_number + '</td>';
                    html += '<td>' + switch_dpid + '</td>';
                    html += '<td>' + port + '</td>';
                    /*DEL*///html += '<td>' + utils.intToMAC(mac_address) + '</td>';
                    html += '<td>' + mac_address + '</td>';
                    html += '<td>' + ((vlan_id >= 0 && vlan_id <= 4095) ? vlan_id : "No") + '</td>';
                    /*DEL*///html += '<td>' + utils.intToIP(ip_address) + '</td>';
                    html += '<td>' + ip_address + '</td>';
                    html += '<td>' + group_name + '</td>';
                    /*DEL*///html += '<td>' + utils.intToIP(multicast_address) + '</td>';
                    html += '<td>' + multicast_address + '</td>';
                    html += '</td>';
                html += '</tr>';
            }
            html += '</tbody>';
            html += '</table>';

            createMenu(html, {width: "80%", pos: "center"});
        }
    }

    function showPortDetails (port) {
        var hovered = port || HCI.getHovered(),
            html = '';

        html += '<table class="arccn">';
        html += '<thead><tr>';
            html += '<th colspan="2">Network Port Settings</th>';
            html += '</tr></thead>';
        html += '<tbody>';
            html += '<tr><td>switch dpid</td><td>'   + hovered.router.id + '</td></tr>';
            html += '<tr><td>openflow port</td><td>' + hovered.of_port + '</td></tr>';
            html += '<tr><td>name</td><td>'          + hovered.name + '</td></tr>';
            html += '<tr><td>bitrate</td><td>'       + hovered.bitrate + '</td></tr>';
            html += '<tr><td>hw_addr</td><td>'       + hovered.hw_addr + '</td></tr>';
            html += '<tr><td>queues</td><td>'        +(hovered.queues ? hovered.queues : 'none') + '</td></tr>';
            if (hovered.isLAG) {
                html += '<tr><td>LAG ports</td><td>['+ hovered.router.lag[hovered.of_port].ports + ']</td></tr>';
            }
            html += '<tr><td>maintenance</td><td><input type="checkbox" '+(hovered.maintenance ? 'checked' : '')+' id="port_maintenance"></td></tr>';

        html += '</tbody>';
        html += '</table>';

        var menu = createMenu(html, {width: "300px", pos: "center"});

        function ok_cb () {
            hovered.setMaintenance(menu.element('port_maintenance').checked);
        }

        function cancel_cb () {
            menu.element('port_maintenance').checked = !menu.element('port_maintenance').checked;
        }

        menu.element('port_maintenance').onchange = function () { menu.confirm(ok_cb, cancel_cb); }
    }

    function showPortStats (selectedPort) {
        var hovered = HCI.getHovered(),
            port = selectedPort || Net.getPort(Net.getHostByID(hovered.router.id), hovered.of_port),
            html = '';

        if (port.pstats_window) {
            UI.createHint("Window with this port's stat is already opened!", {fail: true});
            return;
        }

        html += '<div id="chart" class="chart"></div>';

        html += '<div class="info" style="display: flex; justify-content: space-around;">';
        html += '<div style="width:44%; display: block; position: relative;">';
            html += '<table class="arccn" style="width:100%">';
            html += '<thead><tr>';
                html += '<th colspan="2">Info</th>';
            html += '</tr></thead>';
            html += '<tbody>';
                html += '<tr><td>Switch DPID</td><td>'+port.router.id+'</td></tr>';
                html += '<tr><td>Port ID</td><td>'+port.of_port+'</td></tr>';
                html += '<tr><td>Port name</td><td>'+port.name+'</td></tr>';
            html += '</tbody>';
            html += '</table>';


            html += '<table class="arccn" style="width:100%">';
            html += '<thead><tr>';
                html += '<th colspan="2">Current load</th>';
            html += '</tr></thead>';
            html += '<tbody>';
                html += '<tr><td>RX</td><td id="rxl_cur">xxx</td></tr>';
                html += '<tr><td>TX</td><td id="txl_cur">xxx</td></tr>';
            html += '</tbody>';
            html += '</table>';
        html += '</div>';

        html += '<div style="width:44%; display: block; position: relative;">';
            html += '<table class="arccn" style="width: 100%">';
            html += '<thead><tr>';
                html += '<th colspan="2">Summary</th>';
            html += '</tr></thead>';
            html += '<tbody>';
                html += '<tr><td>tx_packets</td><td id="txp_sum">xxx</td></tr>';
                html += '<tr><td>rx_packets</td><td id="rxp_sum">xxx</td></tr>';
                html += '<tr><td>tx_bytes</td><td id="txb_sum">xxx</td></tr>';
                html += '<tr><td>rx_bytes</td><td id="rxb_sum">xxx</td></tr>';
                html += '<tr><td>tx_dropped</td><td id="txd_sum">xxx</td></tr>';
                html += '<tr><td>rx_dropped</td><td id="rxd_sum">xxx</td></tr>';
            html += '</tbody>';
            html += '</table>';
            html += '<button class="half blue js-freeze">Clear</button>' +
                    '<button class="half blue js-drop">Restore</button>';
        html += '</div>';
        html += '</div>';

        var menu = createMenu(html, {name: "port_stats_window", pos: "center", width: "80%", cl_callback: close});

        port.pstats_window = menu;
        port.pchart = menu.element('chart');
        port.pchart.style.display = 'block';
        port.isSelected = true;
        Net.need_draw = true;

        var eButtonFreeze = menu.querySelector('.js-freeze');
        var eButtonDrop = menu.querySelector('.js-drop');

        eButtonFreeze.onclick = onClickFreeze;
        eButtonDrop.onclick = onClickDrop;

        function close () {
            port.clearView("port");
        }

        function onClickFreeze() {
            port.setDeltaStats();
        }

        function onClickDrop() {
            port.dropDeltaStats();
        }
    }

    function createQueueMenu (hovered) {
        var html = '';
        var last = (hovered.queues2.length > 0 ? hovered.queues2[hovered.queues2.length-1] : {});
            last = (hovered.new_queues.length > 0 ? hovered.new_queues[hovered.new_queues.length-1] : last);

        html += '<table>';
        html += '<tr><td width="25%">Discipline</td><td><select size="1" name="q_disc" id="q_disc" style="width:160px">';
            html += '<option '+(hovered. new_q_disc == "tb_pq" ? 'selected' : (hovered.q_disc == "tb_pq" ? 'selected' : ''))+' value="tb_pq"> tb_pq </option>';
            html += '<option '+(hovered. new_q_disc == "tb_pq_wrr" ? 'selected' : (hovered.q_disc == "tb_pq_wrr" ? 'selected' : ''))+' value="tb_pq_wrr"> tb_pq_wrr </option>';
            html += '<option '+(hovered. new_q_disc == "tb_pq_dwrr" ? 'selected' : (hovered.q_disc == "tb_pq_dwrr" ? 'selected' : ''))+' value="tb_pq_dwrr"> tb_pq_dwrr </option>';
        html += '</select></td></tr>';
        html += '<tr><td width="25%">Min-rate</td><td><input type="text" id="min_rate" placeholder="min" value="'+(last["min_rate"]||'')+'"></td></tr>';
        html += '<tr><td width="25%">Max-rate</td><td><input type="text" id="max_rate" placeholder="max" value="'+(last["max_rate"]||'')+'"></td></tr>';
        html += '<tr><td width="25%">Burst</td><td><input type="text" id="burst" placeholder="burst-size" value="'+(last["burst"]||'')+'"></td></tr>';
        html += '<tr><td width="25%">Priority</td><td><input type="text" id="priority" placeholder="prio" value="'+(last["priority"]||'')+'"></td></tr>';
        html += '<tr><td width="25%">Weight</td><td><input type="text" id="weight" placeholder="weight" value="'+(last["weight"]||'')+'"></td></tr>';
        html += '</table>';
        html += '<button type="button" id="add_queue" class="blue half">Add queue</button>';
        html += '<button type="button" id="cancel_queue" class="rosy half">Cancel</button>';

        var menu = createMenu(html, {title: "Creating new queue", pos: "center", vpos: "center", width: "20%"});
        menu.element('add_queue').onclick = function () {
            var queue = {};
            queue["min_rate"] = menu.element('min_rate').value;
            queue["max_rate"] = menu.element('max_rate').value;
            queue["burst"]    = menu.element('burst').value;
            queue["priority"] = menu.element('priority').value;
            queue["weight"]   = menu.element('weight').value;
            hovered.new_q_disc = menu.element('q_disc').value;
            hovered.new_queues.push(queue);
            hovered.export_queues.length = 0;
            menu.remove();
            showQueuesDetails(true, hovered);
        }
        menu.element('cancel_queue').onclick = menu.remove;
    }

    function activateExportQueue (hovered) {
        Net.import_queue = hovered;
    }

    function activateCopyQueue (hovered) {
        Net.import_queue = hovered;
        Net.hosts.forEach(function (sw) {
            sw.ports.forEach(function (p) {
                HCI.addMMObject(p, showQueueHint);

                function showQueueHint () {
                    var hint = document.body.querySelector('.queue_hint');
                    if (!hint) {
                        var html = '';
                        html += 'Switch: ' + p.router.id + '<br>';
                        html += 'Port: ' + p.name + ' (' + p.of_port + ')';
                        html += '<table style="font-size:14px">';
                        p.queues2.forEach(function (q) {
                            html += '<tr>';
                            html += '<td>Queue ' + q.id + '</td>';
                            html += '<td>min rate: ' + q.min_rate + '</td>';
                            html += '<td>max rate: ' + q.max_rate + '</td>';
                            html += '<td>burst: ' + q.burst + '</td>';
                            html += '<td>priority: ' + q.priority + '</td>';
                            html += '<td>weight: ' + q.weight + '</td>';
                            html += '</tr>';
                        });
                        if (p.queues2.length == 0) {
                            html += '<tr><td>No queues</td></tr>';
                        }
                        html += '</table>';

                        var name = "queue_hint " + "s"+p.router.id+"p"+p.of_port;
                        var hint = createMenu(html, {width: (p.queues2.length > 0 ? "500px" : "150px"), animation: false, name: name});
                        hint.style.left = 180 + p.x + "px"; // TODO: check overflow
                        hint.style.top = 16 + p.y + "px";

                    }
                    else if (!~hint.className.indexOf("s"+p.router.id+"p"+p.of_port)) {
                        removeMenu('queue_hint');
                    }
                }
            });
        });
    }

    function showQueuesDetails (fast, port) {
        var hovered = port ? port : HCI.getHovered();

        hovered.getPortQueues();
        if (fast)
            displayDetails();
        else
            setTimeout(displayDetails, 500);

        function displayDetails () {
            var html = '',
                array = hovered.queues2,
                size  = hovered.queues2.length;

            removeMenu('queue_settings_menu');
            removeMenu('queue_hint');

            if (hovered.export_queues.length > 0) {
                hovered.new_queues.length = 0;
                hovered.export_queues.forEach(function (elem) {
                    hovered.new_queues.push(elem);
                });
            }

            html += '<table class="arccn">';
            html += '<thead><tr>';
            html += '<th colspan="2">Info</th>';
            html += '</tr></thead>';
            html += '<tbody>';
            html += '<tr style="font-weight:bold"><td>DPID</td><td>Port</td><td>Queues count</td><td>Discipline</td></tr>';
            html += '<tr>';
            html += '<td>'+ hovered.router.id +'</td>';
            html += '<td>' + hovered.name +' (' + hovered.of_port +')</td>';
            html += '<td>'+ size +'</td>';
            if (hovered.new_q_disc != hovered.q_disc)
                html += (hovered.new_q_disc ? '<td  style="color:#B03060">'+ (hovered.q_disc ? hovered.q_disc + '->' : '') + hovered.new_q_disc : '<td>'+hovered.q_disc) + '</td>';
            else
                html += '<td>'+hovered.q_disc+'</td>';
            html += '</tr>';
            html += '</tbody>';
            html += '</table>';

            html += '<table class="arccn">';
            html += '<thead><tr>';
            html += '<th colspan="6">Queues Configuration</th>';
            html += '</tr></thead>';
            html += '<tbody>';
                if (size > 0 || hovered.new_queues.length > 0) {
                    html += '<tr style="font-weight:bold">';
                        html += '<td>id</td>';
                        html += '<td>min_rate</td>';
                        html += '<td>max_rate</td>';
                        html += '<td>burst</td>';
                        html += '<td>priority</td>';
                        html += '<td>weight</td>';
                    html += '</tr>';
                }
                else {
                    html += '<tr><td colspan="6">No queues on the port</td></tr>';
                }
                if (size > 0) {
                    for (var i = 0; i < size; i++) {
                        html += '<tr '+(hovered.export_queues.length > 0 ? 'style="text-decoration:line-through"' : '')+'>';
                        html += '<td>Queue '+ hovered.queues2[i]["id"] +'</td>';
                        html += '<td>'+ hovered.queues2[i]["min_rate"] +'</td>';
                        html += '<td>'+ hovered.queues2[i]["max_rate"] +'</td>';
                        html += '<td>'+ hovered.queues2[i]["burst"] +'</td>';
                        html += '<td>'+ hovered.queues2[i]["priority"] +'</td>';
                        html += '<td>'+ hovered.queues2[i]["weight"] +'</td>';
                        html += '</tr>';
                    }
                }
                if (hovered.new_queues.length > 0) {
                    for (var i = 0; i < hovered.new_queues.length; i++) {
                        html += '<tr style="color:#B03060">';
                        html += '<td>Queue X</td>';
                        html += '<td>'+ hovered.new_queues[i]["min_rate"] +'</td>';
                        html += '<td>'+ hovered.new_queues[i]["max_rate"] +'</td>';
                        html += '<td>'+ hovered.new_queues[i]["burst"] +'</td>';
                        html += '<td>'+ hovered.new_queues[i]["priority"] +'</td>';
                        html += '<td>'+ hovered.new_queues[i]["weight"] +'</td>';
                        html += '</tr>';
                    }
                    html += '<tr><td colspan="6">';
                    html += '<button type="button" id="apply_queue" class="blue fourth">Apply changes</button>';
                    html += '<button type="button" id="discard_queue" class="rosy fourth">Discard changes</button>';
                    html += '</td></tr>';
                }
            html += '</tbody>';
            html += '</table>';
            html += '<button type="button" id="add_queue" class="blue fourth">Add queue</button>';
            html += '<button type="button" id="copy_queue" class="blue fourth">Export queues</button>';
            html += '<button type="button" id="del_queue" class="rosy fourth">Delete all queues</button>';

            var menu = createMenu(html, {name: "queue_settings_menu", width: "50%", pos: "center", cl_callback: close});
            menu.element('add_queue').onclick = function () {
                createQueueMenu(hovered);
            }
            menu.element('copy_queue').onclick = function () {
                //activateCopyQueue(hovered);
                activateExportQueue(hovered);
            }
            menu.element('del_queue').onclick = function () {
                Server.ajax('DELETE', '/switches/' + hovered.router.id + '/port/' + hovered.of_port + '/queues', function (response) {
                    hovered.new_queues.length = 0;
                    hovered.new_q_disc = false;
                    showQueuesDetails();
                });
            }

            // apply changes action
            if (menu.element('apply_queue'))
                menu.element('apply_queue').onclick = function () {
                    var ret = {};
                    ret["type"] = hovered.new_q_disc;
                    ret["queues"] = [];
                    if (hovered.export_queues.length == 0)
                        ret["queues"] = hovered.queues2;
                    hovered.new_queues.forEach(function(q) {
                        ret["queues"].push(q);
                    });

                    console.log("queues", ret);

                    Server.ajax('POST', '/switches/' + hovered.router.id + '/port/' + hovered.of_port + '/queues', ret, function (response) {
                        hovered.new_queues.length = 0;
                        hovered.export_queues.length = 0;
                        hovered.new_q_disc = false;
                        Net.import_queue = {};
                        port ? showQueuesDetails(false, port) : showQueuesDetails();
                    });
                };

            // discard changes action
            if (menu.element('discard_queue'))
                menu.element('discard_queue').onclick = function () {
                    hovered.new_queues.length = 0;
                    hovered.export_queues.length = 0;
                    hovered.new_q_disc = false;
                    //Net.import_queue = {};
                    port ? showQueuesDetails(true, port) : showQueuesDetails(true);
                };

            function close () {
                HCI.clMMObjects();
                Net.import_queue = {};
                removeMenu('queue_hint');
            }
        } // end displayDetails
    }

    function showQueues (selectedPort) {
        var hovered = HCI.getHovered(),
            port = selectedPort || Net.getPort(Net.getHostByID(hovered.router.id), hovered.of_port),
            html = '';

        if (port.queues.length == 0)
            return;

        html += '<div id="chart" class="chart"></div>';

        html += '<div class="info"><table class="arccn" style="width:44%">';
        html += '<thead><tr>';
            html += '<th colspan="2">Info</th>';
        html += '</tr></thead>';
        html += '<tbody>';
            html += '<tr><td>Switch DPID</td><td>'+port.router.id+'</td></tr>';
            html += '<tr><td>Port ID</td><td>'+port.of_port+'</td></tr>';
            html += '<tr><td>Port name</td><td>'+port.name+'</td></tr>';
        html += '</tbody>';
        html += '</table>';

        html += '<table class="arccn" style="width:44%">';
        html += '<thead>';
            html += '<tr>';
                html += '<th colspan="4">Summary</th>';
            html += '</tr>';
            html += '<tr>';
                html += '<th>Name</th>';
                html += '<th>Tx bytes</th>';
                html += '<th>Drops</th>';
            html += '</tr>';
        html += '</thead>';
        html += '<tbody>';
            for (var i = 0, l = port.queues.length; i < l;i++) {
                html += '<tr><td>Queue ' + i + '</td><td id="q'+i+'_bytes">xxx</td><td id="q'+i+'_drops">xxx</td></tr>';
            }
        html += '</tbody>';
        html += '</table></div>';

        var menu = createMenu(html, {name: "port_stats_window", pos: "center", width: "80%", cl_callback: close});

        port.qstats_window = menu;
        port.qchart = menu.element('chart');
        port.qchart.style.display = 'block';
        port.showQ = true;
        port.isSelected = true;
        Net.need_draw = true;
        port.getQueuesStat();

        function close () {
            port.clearView("queue");
        }
    }
/* **** UI Details and stats END **** */

    function setStyle (obj, prop, val) {
        obj.style[prop] = val;
        if (obj.style[prop].indexOf(val) < 0) { obj.style[prop] = '-webkit-' + val; }
        if (obj.style[prop].indexOf(val) < 0) { obj.style[prop] = '-moz-' + val; }
        if (obj.style[prop].indexOf(val) < 0) { obj.style[prop] = '-o-' + val; }
        if (obj.style[prop].indexOf(val) < 0) { window.console.error('Can not style', obj, prop, val); }
    }

/* **** UI Cluster **** */
    function showRecoveryTable() {
        if (Recovery.eMenu) {
            Recovery.remove();
        } else {
            show(Recovery.getData());
        }
        function show(recoveryData) {
            var data = recoveryData.controllers,
                columns = recoveryData.columnNames,
                blockData = recoveryData.dataBlocks,
                changeBlocks = recoveryData.changeBlocks;

            // to fix width of mastership view table
            var menuMinWidth = 100;
            for (var controllerId in data) {
                if (data.hasOwnProperty(controllerId)) {
                    menuMinWidth += 100;
                }
            }
            var menuWidth = menuMinWidth > 800 ? menuMinWidth : 800;

            var html = '';
            html += '<div id="recovery" class="recovery" style="display: block; position: relative;">';
                html += '<table class="arccn controllers--table" style="float: none;">';
                    html += makeTableContent(data, columns);
                html += '</table>';
                html += '<div style="display:block; position: relative;" class="mastership--view">';
                    html += '';
                html += '</div>';
            html += '</div>';

            var eMenu = createMenu(html, {
                title: 'RUNOS Cluster',
                width: menuWidth + 'px',
                cl_callback: onMenuClose
            });
            Recovery.eMenu = eMenu;

            initLineEvent();

            var intervalId = setInterval(function() {
                if (eMenu.offsetHeight == 0) {
                    clearInterval(intervalId);
                    return;
                }
                update();
            }, 1000);

            function update() {
                data = recoveryData.controllers;
                eMenu.querySelector('.controllers--table').innerHTML =
                    makeTableContent(data, columns);

                Recovery.getMastershipViewData(function(controllers, switches) {
                    eMenu.querySelector('.mastership--view').innerHTML = makeMastershipView(controllers, switches);
                });

                initLineEvent();
            }

            function initLineEvent() {
                Array.prototype.forEach.call(eMenu.querySelectorAll('.line>.settings'), function(eLine) {
                    eLine.addEventListener('click', onClickLine);
                });
            }

            function onMenuClose() {
                Recovery.remove();
            }

            function onClickLine() {
                var id = this.getAttribute('data-id');
                if (Recovery.eSubMenus[id]) {
                    Recovery.eSubMenus[id].remove();
                    Recovery.eSubMenus[id] = null;
                    return;
                }

                if (data[id]['is_Iam'] == 'false') {
                    return;
                }
                Recovery.removeSubMenus();
                var eSubMenu = createMenu(makeSubtable(data[id], blockData, changeBlocks),
                    {
                        cl_callback: function() {
                            Recovery.eSubMenus[id] = null;
                        }
                    });
                Recovery.eSubMenus[id] = eSubMenu;

                var eChangeButtons = eSubMenu.querySelectorAll('.change-button');
                var eChangeCloseButtons = eSubMenu.querySelectorAll('.change-close');
                var eChangeFormWrappers = eSubMenu.querySelectorAll('.change-wrapper');
                var eChangeForms = eSubMenu.querySelectorAll('.js-form');

                var curActive;

                Array.prototype.forEach.call(eChangeButtons, function(eChangeButton) {
                    eChangeButton.addEventListener('click', function onClickChangeButton(e) {
                        e.stopPropagation();
                        if (curActive != undefined) {
                            eChangeFormWrappers[curActive].style.display = 'none';
                        }
                        curActive = this.getAttribute('data-index');
                        eChangeFormWrappers[curActive].style.display = 'block';
                    });
                });
                Array.prototype.forEach.call(eChangeCloseButtons, function(eChangeCloseButton) {
                    eChangeCloseButton.addEventListener('click', function(e) {
                        e.stopPropagation();
                        e.preventDefault();
                        eChangeFormWrappers[curActive].style.display = 'none';
                        curActive = undefined;
                    });
                });
                Array.prototype.forEach.call(eChangeFormWrappers, function(eChangeFormWrapper) {
                    eChangeFormWrapper.addEventListener('click', function(e) {
                       e.stopPropagation();
                    });
                });
                Array.prototype.forEach.call(eChangeForms, function(eChangeForm) {
                    eChangeForm.addEventListener('submit', Recovery.onSubmit);
                });

                eSubMenu.addEventListener('click', function() {
                    if (curActive != undefined) {
                        eChangeFormWrappers[curActive].style.display = 'none';
                        curActive = undefined;
                    }
                });
            }
        }

        function makeMastershipView(controllers, switches) {
            // if (!controllers || !switches) return 'Not enough information';
            var html = '';
            html += '<h2>Mastership View</h2>';
            html += '<table class="arccn" style="float: none;">';

            html += '<thead style="color: white;">';
                html += '<tr>';
                html += '<td>Switches</td>';
                    //for each controller
                    for (var i = 0; i < controllers.length; ++i) {
                        html += '<td> RUNOS(ID=' + controllers[i] + ')</td>';
                    }
                html += '</tr>';
            html += '</thead>';

            for (var dpid in switches) {
                html += '<tr>';
                    html += '<td> Switch (DPID=' + dpid + ')</td>';
                    // for each controller
                    for (var i = 0; i < controllers.length; ++i) {
                        html += '<td>' + switches[dpid][controllers[i]] + '</td>';
                    }
                html += '</tr>';
            }

            html += '</table>';

            return html;
        }

        function makeTableContent(data, columns) {
            var html = '';
            if (typeof data == 'string') {
                html += makeMessage();
            } else {
                html += makeHead(columns);
                html += makeBody(data, columns);
            }
            return html;
        }


        function makeHead(columns) {
            var styles = {
                td : {
                    color : 'white'
                }
            };
            var html = '';
            html += '<thead>';
            html += '<tr>';
            for (var i = 0, len = columns.length; i < len; ++i) {
                html += '<td style="' + getStyleString(styles.td) + '">' + columns[i].title + '</td>';
            }
            html += '<td width="25px"></td></tr></thead>';
            return html;
        }
        function makeBody(controllers, columns) {
            var activeColor = 'rgba(80,200,80,0.6)';
            var html = '';
            html += '<tbody style="overflow: auto;">';
            for (var controllerId in controllers) {
                var controller = controllers[controllerId];
                html += '<tr data-id="' + controller.ID + '" data-index="' + i + '" class="line">';
                for (var j = 0, len_j = columns.length; j < len_j; ++j) {
                    html += '<td style="' +
                        (controller['is_Iam'] ? 'background:' + activeColor : '') +
                        '">';
                    if (columns[j].title == 'state' && controller[columns[j].names] == 'Active') {
                        html += controller[columns[j]];
                    } else {
                        if (Array.isArray(columns[j].names)) {
                            for (var k = 0, len_k = columns[j].names.length; k < len_k; ++k) {
                                html += controller[columns[j].names[k]];
                                if (k < len_k - 1) {
                                    html += columns[j].delimiter;
                                }
                            }
                        } else {
                            html += controller[columns[j].names];
                        }
                    }
                    html += '</td>';
                }
                if (controller['is_Iam']) {
                    html += '<td width="25px" ' +
                        'style="background: lightblue !important;' +
                               'cursor: pointer;' +
                               'position: relative"' +
                        'class="settings"' +
                        'data-id="' + controllerId + '"> <span class="icon-settings"></span> </td>';
                } else {
                    html += '<td width="25px"></td>';
                }
                html += '</tr>';
            }
            html += '</tbody>';
            return html;
        }
        function makeMessage() {
            var styles = {
                message: {
                    display: 'block',
                    position: 'relative'
                }
            }

            return '<tr style="' + getStyleString(styles.message) + '"><td>В данный момент нет запущеных контроллеров</td></tr>';
        }
        function makeSubtable(data, blockData, changeBlocks) {
            var styles = {
                subtable : {
                    display : 'block',
                    position: 'relative',
                    width: '100%'
                },
                row : {
                    display: 'flex',
                    position: 'relative',
                    'justify-content' : 'space-between',
                    padding: '5px 10px'
                },
                rowElem : {
                    display: 'block',
                    position: 'relative'
                }
            };

            var html = '<div id="subtable' + data.ID + '" class="recovery-subtable" ' +
                        'style="' + getStyleString(styles.subtable) + '">';
            for (var i = 0, len = blockData.length; i < len; ++i) {
                html += '<h2>' + blockData[i].title + '</h2>';
                for (var j = 0, len2 = blockData[i].fields.length; j < len2; ++j) {
                    var fields = blockData[i].fields;
                    html += '<div class="recvery-subtable-row" style="' + getStyleString(styles.row) +'">';
                        html += '<div style="' + getStyleString(styles.rowElem)+ '">' + Recovery.getAlias(fields[j]) + '</div>';
                        html += '<div style="' + getStyleString(styles.rowElem)+ '">' + data[fields[j]] + '</div>';
                    html += '</div>';
                }
            }
            html += '</div>';
            html += '<hr/>';
            html += makeFormsCaller(changeBlocks);
            html += makeForms(changeBlocks, data);
            return html;
        }

        function makeFormsCaller(changeBlocks) {
            var html = '<h2 class="blue"> Modify </h2>';
            for (var i = 0, len = changeBlocks.length; i < len; ++i) {
                html += '<button class="change-button blue" data-index="' + i + '">' + changeBlocks[i].name + '</button>';
            }
            return html;
        }

        function makeForms(changeBlocks, data) {
            var html = '', i, len;
            for (i = 0, len = changeBlocks.length; i < len; ++i) {
                html += makeForm(changeBlocks[i], i, data);
            }
            return html;

        }
        function makeForm(changeBlock, index, data) {
            var styles = {
                wrapper : {
                    position: 'absolute',
                    bottom: '45px',
                    top: 'auto',
                    'min-width': '200px'
                },
                label : {

                },
                row : {
                    display: 'flex',
                    'justify-content': 'space-between',
                    'align-items': 'center',
                    position: 'relative'
                }
            };
            var html = '<div class="somemenu_manual change-wrapper" data-index="' + index + '"style="' + getStyleString(styles.wrapper) + '">';
            html += '<h2>' + changeBlock.title + '</h2>';

            html += '<form class="js-form">';
                html += '<input type="hidden" name="command" class="js-field" value="' + changeBlock.command +'">';
                changeBlock.fields.forEach(function (field) {
                    html += '<div style="' + getStyleString(styles.row) +'">';
                        html += '<label for="' + field + '" stlye="' + getStyleString(styles.label) + '">' + Recovery.getAlias(field) + '</label>';
                        html += makeFromInput(field, data[field]);
                    html += '</div>';
                });
                html += '<button type="submit" class="submit blue">Submit</button>';
                html += '<button class="change-close rosy" data-index="' + index + '">Close</button>';
            html += '</form></div>';
            return html;
        }

        function makeFromInput(field, value) {
            var fieldData = Recovery.getFieldType(field);
            var html = '', i, len;

            switch (fieldData.type) {
                case 'select':
                    html += '<select class="js-field"' +
                                  'name="' + field +
                                  '" id="' + field +
                                  '" style="width: auto">';
                    for (i = 0, len = fieldData.options.length; i < len; ++i) {
                        html += '<option value="' + fieldData.options[i] + '" ' +
                            (fieldData.options[i] == value ? 'selected' : '') + '>' +
                                 fieldData.options[i] + '</option>';
                    }
                    html += '</select>';
                    return html;
                // Default text case returned
                default:
                    return '<input id="' + field + '"type="text" name="' + field +
                        '" class="js-field" value="' + value + '">';

            }
        }
    }
/* **** UI Cluster END **** */

    function getStyleString(styles) {
        var result = '', style;
        for (style in styles) {
            result += style + ':' + styles[style] + ';';
        }
        return result;
    }


/* **** UI Routes **** */
    function showInBandRoutesDetails() {
        var hovered = HCI.getHovered(),
            id = hovered.id;

        hovered.show_inband = !hovered.show_inband;
        if (UI.menu && UI.menu.querySelector('.in_band_routes')) {
            UI.menu.querySelector('.in_band_routes').querySelector('i').className = (hovered.show_inband ? 'on' : 'off');
        }

        if (hovered.show_inband) {
            Routes.api.getRoutes.inBand.byDpid(id, function () {});
        } else {
            Routes.routes['in-band'][id].hide();
        }

        Net.need_draw = true;
    }

    function showDomainRoutes(hovered) {
        hovered = hovered || HCI.getHovered();
        Routes.hide();

        Routes.api.getRoutes.domain.byDomainName(hovered.name, show);

        function show(routes) {

            routes.sort(function(a, b) {
               return parseInt(a.id) - parseInt(b.id);
            });

            ///////// Register events for Routes menu /////////
            function registerEvents(menu) {
                // Initial method for menu updating
                function updateMenu (new_routes) {
                    new_routes.sort(function(a, b) {
                       return parseInt(a.id) - parseInt(b.id);
                    });
                    menu.update(function () { return getRoutesTableHtml(new_routes, hovered); });
                }

                // Domain settings
                ['auto_rerouting', 'auto_dr_switching'].forEach(function(type) {
                    if (!menu.querySelector('.' + type)) return;

                    menu.querySelector('.' + type).onclick = function () {
                        menu.confirm(ok_cb, cancel_cb);
                        checkbox = this;

                        function ok_cb () {
                            var res = checkbox.checked;
                            var settings = {};
                            settings[type] = res;
                            Server.ajax("POST", '/bridge_domains/' + hovered.name + '/', settings, function (resp) {
                                createHint("Settings were changed!");
                                hovered[type] = res;
                            });
                        }

                        function cancel_cb () {
                            checkbox.checked = !checkbox.checked;
                        }
                    }
                });

                // Path show/hide
                var routesElemCollection = menu.querySelectorAll('.route');
                Array.prototype.forEach.call(routesElemCollection, function(routeElem) {
                    var routeId = routeElem.getAttribute('data-id');
                    var pathId = routeElem.getAttribute('data-sub-id');

                    var route = Routes.getDomainRouteById(hovered.name, routeId);

                    var cells = routeElem.querySelectorAll('.path-active-cell');
                    Array.prototype.forEach.call(cells, function(cell) {
                        cell.onclick = function() {
                            togglePath(route, cell, pathId);
                            Net.need_draw = true;
                        };

                        cell.addEventListener('contextmenu', function(event) {
                            var path = route.paths[pathId];
                            path.color = Routes.getRandomColor();
                            cell.style.background = path.color;
                            Net.need_draw = true;
                            Routes.saveColors(route);
                            event.preventDefault();
                        });
                    });

                    // Modify path parameters
                    var changable = routeElem.querySelectorAll('.changable');
                    Array.prototype.forEach.call(changable, function(changable) {
                        changable.onclick = function () {
                            var re = /(\D)/;
                            var value = changable.innerHTML;
                            var old_value = value;
                            value = value.substring(0, value.search(re));
                            changable.innerHTML = '<input type="text" class="change_path_options" value="'+value+'" old_value="'+old_value+'">';
                            changable.onclick = {};

                            var new_last_child_html = '<i class="apply"></i><i class="delete"></i>';
                            var old_last_child_html = '';
                            if (changable.parentElement.lastChild.innerHTML != new_last_child_html) {
                                old_last_child_html = changable.parentElement.lastChild.innerHTML;
                                changable.parentElement.lastChild.innerHTML = new_last_child_html;

                                changable.parentElement.querySelector('.apply').onclick = function () {
                                    var tr = this.parentElement.parentElement;
                                    var params = {};
                                    for (var child = tr.firstChild; child != null; child = child.nextSibling) {
                                        if (child.querySelector('input.change_path_options')) {
                                            params[child.id] = child.querySelector('input').value;
                                        }
                                    }

                                    var path = '/routes/id/' + routeId + '/modify-path/' + pathId + '/';
                                    Server.ajax("POST", path, params,
                                    function (resp) {
                                        for (var child = tr.firstChild; child != null; child = child.nextSibling) {
                                            if (child.querySelector('input.change_path_options')) {
                                                child.innerHTML = child.querySelector('input').value + child.getAttribute('postfix');
                                            }
                                        }

                                        changable.parentElement.lastChild.innerHTML = old_last_child_html;
                                        registerEvents(menu);
                                        createHint(resp['act']);
                                    },
                                    function (err) {
                                        createHint(err.error.msg, {fail: true});
                                        for (var child = tr.firstChild; child != null; child = child.nextSibling) {
                                        if (child.querySelector('input.change_path_options')) {
                                            child.innerHTML = child.firstChild.getAttribute('old_value');
                                        }
                                    }

                                    changable.parentElement.lastChild.innerHTML = old_last_child_html;
                                    registerEvents(menu);
                                    });
                                }

                                changable.parentElement.querySelector('.delete').onclick = function () {
                                    var tr = this.parentElement.parentElement;
                                    for (var child = tr.firstChild; child != null; child = child.nextSibling) {
                                        if (child.querySelector('input.change_path_options')) {
                                            child.innerHTML = child.firstChild.getAttribute('old_value');
                                        }
                                    }

                                    changable.parentElement.lastChild.innerHTML = old_last_child_html;
                                    registerEvents(menu);
                                }
                            }
                        }
                    });
                });

                // Hide paths
                menu.querySelector('.hide_routes').onclick = function() {
                    Routes.hide();
                    var activeCellElemCollection = menu.querySelectorAll('.path-active-cell.active');
                    Array.prototype.forEach.call(activeCellElemCollection, function(activeCellElem) {
                        activeCellElem.classList.remove('active');
                        activeCellElem.style.background = '#999999';
                    });
                    Net.need_draw = true;
                }

                // Remove path
                var remPath = menu.querySelectorAll('.remove_path');
                Array.prototype.forEach.call(remPath, function(tag) {
                    var routeId = tag.parentElement.getAttribute('data-id');
                    var pathId = tag.parentElement.getAttribute('data-sub-id');

                    function ok_cb () {
                        var path = '/routes/id/' + routeId + '/delete-path/' + pathId + '/';
                        Server.ajax("DELETE", path, function () {
                            UI.createHint("Path " + pathId + " removed for route " + routeId);
                            Routes.savedColors[routeId].splice(pathId, 1);
                            Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                        }, function (response) {
                            UI.createHint(response.error.msg, {fail: true});
                        });
                    }
                    tag.onclick = function () { menu.confirm(ok_cb); }
                });

                // Remove dynamic
                var remDyn = menu.querySelectorAll('.del_dynamic');
                Array.prototype.forEach.call(remDyn, function(tag) {
                    var routeId = tag.parentElement.getAttribute('data-id');

                    function ok_cb () {
                        var path = '/routes/id/' + routeId + '/delete-dynamic/';
                        Server.ajax("DELETE", path, function () {
                            UI.createHint("Dynamic removed for route " + routeId);
                            Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                        });
                    }
                    tag.onclick = function () { menu.confirm(ok_cb); }
                });


                // Up path
                var upPath = menu.querySelectorAll('.up_path');
                Array.prototype.forEach.call(upPath, function(tag) {
                    var routeId = tag.parentElement.getAttribute('data-id');
                    var pathId = parseInt(tag.parentElement.getAttribute('data-sub-id'));

                    function ok_cb () {
                        var path = '/routes/id/' + routeId + '/move-path/' + pathId + '/';
                        Server.ajax("POST", path, {"new_pos": pathId-1}, function () {
                            UI.createHint("Path " + pathId + " moved up for route " + routeId);
                            var colors = Routes.savedColors[routeId];
                            var tmp = colors[pathId];
                            colors[pathId] = colors[pathId-1];
                            colors[pathId-1] = tmp;
                            Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                        });
                    }
                    tag.onclick = function () { menu.confirm(ok_cb); }
                });

                // Down path
                var downPath = menu.querySelectorAll('.down_path');
                Array.prototype.forEach.call(downPath, function(tag) {
                    var routeId = tag.parentElement.getAttribute('data-id');
                    var pathId = parseInt(tag.parentElement.getAttribute('data-sub-id'));

                    function ok_cb () {
                        var path = '/routes/id/' + routeId + '/move-path/' + pathId + '/';
                        Server.ajax("POST", path, {"new_pos": pathId+1}, function () {
                            UI.createHint("Path " + pathId + " moved down for route " + routeId);
                            var colors = Routes.savedColors[routeId];
                            var tmp = colors[pathId];
                            colors[pathId] = colors[pathId+1];
                            colors[pathId+1] = tmp;
                            Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                        });
                    }
                    tag.onclick = function () { menu.confirm(ok_cb); }
                });

                routes.forEach(function(route) {
                route = Routes.getDomainRouteById(hovered.name, route.id); // hack. 'route' in 'routes' may by old

                // Change used path
                var usedElemCollection = menu.querySelectorAll('.use_path_'+route.id);
                Array.prototype.forEach.call(usedElemCollection, function(usedElem) {
                    var routeId = usedElem.getAttribute('data-id');
                    var pathId = usedElem.getAttribute('data-sub-id');

                    function ok_cb () {
                        Server.ajax("POST", "/bridge_domains/"+hovered.name+"/swap_path/",
                            {"route": routeId, "path": pathId},
                            function(resp) {
                                if (resp["res"] == "ok") {
                                    createHint("Switching to selected path<br>Domain: " + hovered.name);

                                    // make this the first path
                                    var path = '/routes/id/' + routeId + '/move-path/' + pathId + '/';
                                    Server.ajax("POST", path, {"new_pos": 0}, function () {
                                        var colors = Routes.savedColors[routeId];
                                        var tmp = colors[pathId];
                                        colors[pathId] = colors[0];
                                        colors[0] = tmp;
                                        Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                                    });
                                } else {
                                    createHint("Cannot switch path in domain " + hovered.name, {fail: true});
                                }
                            });
                    }

                    var prev_path_id;
                    function cancel_cb () {
                        // FIXME: can be bug, if there are more than one route!
                        menu.querySelectorAll('.use_path_'+route.id)[prev_path_id].checked = true;
                    }

                    usedElem.onmouseup = function () { prev_path_id = document.querySelector('input[class="use_path_'+route.id+'"]:checked').getAttribute('data-sub-id'); }
                    usedElem.onchange = function () { menu.confirm(ok_cb, cancel_cb); }
                });

                // Add path
                menu.querySelector('.add_path_'+route.id).onclick = function() {
                    var path_menu = createMenu(getNewPathHtml(route.paths[route.paths.length-1]),
                                                                 {title: "Creating new path",
                                                                  pos: "center",
                                                                  width: "250px",
                                                                  unique: true,
                                                                  cl_callback: Routes.pathSelector.clear});
                    // because Manual path
                    path_menu.element('clude').disabled = true;
                    Routes.pathSelector.setMode("exact", route.sourceId, route.targetId);

                    // Add path action
                    path_menu.element('add_path').onclick = function () {
                        var path = {
                            metrics : path_menu.element('metrics').value,
                            flapping : path_menu.element('flapping').value || "0",
                            broken_flag : "true", // always react on broken links
                            drop_threshold : path_menu.element('drop').value || "0",
                            util_threshold : path_menu.element('util').value || "0"
                        };

                        var type = path_menu.element('type').value,
                            mode = Routes.pathSelector.getMode();
                        if (mode) {
                            if (mode == "exact") {
                                path.exact = Routes.pathSelector.getExact();
                                if (!Routes.pathSelector.checkExact()) {
                                    UI.createHint("Please select the manual path!", {fail: true});
                                    return;
                                }
                            }
                            else {
                                var include = Routes.pathSelector.getInclude();
                                var exclude = Routes.pathSelector.getExclude();
                                if (include.length > 0) path.include = include;
                                if (exclude.length > 0) path.exclude = exclude;
                            }
                        }

                        if (type == "static") {
                            if (path.exact) {
                                if ((path.exact[0] != route.sourceId || path.exact[path.exact.length-1] != route.targetId) &&
                                    (path.exact[0] != route.targetId || path.exact[path.exact.length-1] != route.sourceId))
                                    return;
                            }
                            Server.ajax("POST", '/routes/id/' + route.id + '/add-path/', path,
                            function (resp) {
                                if (resp["act"] == "path created") {
                                    UI.createHint("Path " + resp["path_id"] + " created for route " + resp["route_id"]);
                                    Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                                }
                                else UI.createHint(resp["error"], {fail: true});
                            },
                            function (err_resp) {
                                UI.createHint(err_resp.error.msg, {fail: true});
                            });
                        } else if (type == "dynamic") {
                            Server.ajax("POST", '/routes/id/' + route.id + '/add-dynamic/', path, function (resp) {
                                UI.createHint("Dynamic created for route " + route.id);
                                Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                            });
                        }
                        path_menu.remove();
                        Routes.pathSelector.clear();
                    };

                    // Close sub menu
                    path_menu.element('cancel_path').onclick = function () {
                        path_menu.remove();
                        Routes.pathSelector.clear();
                    }

                    // Select include-exclude
                    path_menu.element('clude').onchange = function() {
                        Routes.pathSelector.setMode(this.value, route.sourceId, route.targetId);
                    };

                    // Select type of path
                    path_menu.element('type').onchange = function () {
                        var type = path_menu.element('type').value;
                        if (type == 'dynamic') {
                            if (path_menu.element('metrics').value == 'Manual') {
                                path_menu.element('metrics').value = 'Hop';
                                Routes.pathSelector.clear();
                            }
                            path_menu.element('metrics').options[0].disabled = true; // TODO: very bad '0'!!
                            path_menu.element('clude').disabled = false;
                        } else if (type == 'static') {
                            path_menu.element('metrics').options[0].disabled = false; // TODO: very bad '0'!!
                        }
                    }

                    // Manual path or not
                    path_menu.element('metrics').onchange = function () {
                        var metrics = path_menu.element('metrics').value;
                        if (metrics == "Manual") {
                            path_menu.element('clude').disabled = true;
                            Routes.pathSelector.setMode("exact", route.sourceId, route.targetId);
                        } else {
                            var clude = path_menu.element('clude');
                            clude.disabled = false;
                            Routes.pathSelector.setMode(clude.value, route.sourceId, route.targetId);
                        }
                    };
                }

                // Predict path
                var predict = menu.querySelector('.predict_path_'+route.id);
                if (predict) {
                    predict.onclick = function () {
                        route.predictPath(2000);
                    }
                }

                }); // END routes.forEach

                // Change used switch (for domains with backup)
                var usedElemCollection = menu.querySelectorAll('.use_switch');
                Array.prototype.forEach.call(usedElemCollection, function(usedElem) {
                    var mainId = usedElem.getAttribute('main-id');
                    var thisId = usedElem.getAttribute('this-id');

                    function ok_cb () {
                        Server.ajax("POST", "/bridge_domains/"+hovered.name+"/swap_switch/",
                            {"dpid": mainId, "deny": mainId != thisId},
                            function(resp) {
                                if (resp["res"] == "ok") {
                                    createHint("Changed upstream to " + thisId + " switch<br>Domain: " + hovered.name);
                                } else {
                                    createHint("Cannot change upstream in domain " + hovered.name, {fail: true});
                                }

                                Routes.updateAllDomainRoutes(hovered.name, updateMenu);
                            });
                    }

                    var prev_switch;
                    function cancel_cb () {
                        menu.querySelectorAll('.use_switch')[prev_switch].checked = true;
                    }

                    usedElem.onmouseup = function () { prev_switch = document.querySelector('input[class="use_switch"]:checked')
                                                                             .getAttribute('num'); }
                    usedElem.onchange = function () { menu.confirm(ok_cb, cancel_cb); }
                });
            }
            ///////// End of register events for Routes menu /////////

            var menu = createMenu(getRoutesTableHtml(routes, hovered), {name: 'domain-routes',
                                         width: '550px',
                                         title: 'Domain routes',
                                         unique: true,
                                         ev_handlers: registerEvents,
                                         cl_callback: Routes.hide});

            menu.statics.update = function() {
                Routes.updateAllDomainRoutes(hovered.name, function (routes) {
                    menu.update(function () {
                        return UI.getDomainRoutesTableHtml(routes, hovered);
                    });
                });
            }
        }
    }

    function getRoutesTableHtml (routes, domain) {
        var html = '';

        html += '<table class="arccn" style="float:inherit">'
             +  '<thead><tr><th>Domain settings</th></tr></thead>'
             +  '<tbody><tr>'
             +  '<td>Auto rerouting</td>'
             +  '<td><input type="checkbox" class="auto_rerouting" '+(domain.auto_rerouting ? 'checked' : '')+'></td></tr>';
        if (domain && domain.mode != "MP" && domain.hosts.length > 2) {
            html += '<tr><td>DR auto switching</td>'
                 +  '<td><input type="checkbox" class="auto_dr_switching" '+(domain.auto_dr_switching ? 'checked' : '')+'></td></tr>';
        }
        html += '</tbody></table>';

        // Domain has backup switch to upstream
        if (domain && domain.mode != "MP" && domain.hosts.length > 2) {
            html += '<table class="arccn" style="float:inherit">'
                 +  '<thead><tr>'
                 +  '<th class="routes--th">Switch Role</th>'
                 +  '<th class="routes--th">Switch DPID</th>'
                 +  '<th class="routes--th">Used switch</th>'
                 +  '</tr></thead>'
                 +  '<tbody>';
            for (var i = 1; i < domain.hosts.length; i++) {
                var sw = domain.hosts[i];
                var active = (i == 1 && domain.deny == "false" || i > 1 && domain.deny == "true");
                html += '<tr>'
                     +  '<td>'+ (i == 1 ? 'Main' : 'Backup') +'</td>'
                     +  '<td>'+ sw.id +'</td>'
                     +  '<td><input type="radio" class="use_switch" main-id="'+domain.hosts[1].id+'"'
                     +  ' this-id="'+sw.id+'" num="'+(i-1)+'" name="used_switch"'+ (active ? ' checked' : '') +'></td>'
                     +  '</tr>';
            }
            html += '</tbody></table>';
        }

        routes.forEach(function(route) {

            html += '<table class="arccn" style="float:inherit">';
            html += '<thead>';
                html += '<tr>' +
                    '<th class="routes--th" colspan="3">From: '+route.sourceId+'</th>' +
                    '<th class="routes--th" colspan="3">To: '+route.targetId+'</th>' +
                    '</tr>';
                html += '<tr>' +
                    '<th class="routes--th">Path id</th>' +
                    '<th class="routes--th">Flap</th>' +
                    '<th class="routes--th">Drop</th>' +
                    '<th class="routes--th">Util</th>' +
                    '<th class="routes--th">Used</th>' +
                    '<th class="routes--th">Show</th>' +
                    '<th class="routes--th"></th>' +
                    '</tr>';
            html += '</thead>';

            html += '<tbody class="routes_self">';

            var postfixes = {
                'flapping' : ' sec',
                'drop'     : '%',
                'util'     : '%'
            };

            for (var i = 0; i < route.paths.length; i++) {
                var path = route.paths[i];
                html += '<tr class="route" data-id="' + route.id + '" data-sub-id="' + i
                     + '" data-service="' + route.service + '">';
                    html += '<td>' + i + '</td>';
                    html += '<td class="changable" id="flapping" postfix="'+postfixes['flapping']+'">' + path.flapping + postfixes['flapping'] + '</td>';
                    html += '<td class="changable" id="drop_threshold" postfix="'+postfixes['drop']+'">' + path.dropThreshold + postfixes['drop'] + '</td>';
                    html += '<td class="changable" id="util_threshold" postfix="'+postfixes['util']+'">' + path.utilThreshold + postfixes['util'] + '</td>';
                    html += '<td><input type="radio" class="use_path_'+route.id+'" data-id="' + route.id + '" data-sub-id="' + i
                            +'" name="used_'+route.id+'" '
                            + (route.usedPath == i ? 'checked' : '') +'></td>';
                    html += '<td class="path-active-cell '
                            + (path.isShown ? 'active' : '')
                            + '" style="background: '
                            + (path.isShown ? path.color : '#999999')
                            + '"></td>';
                    html += '<td data-id="' + route.id + '" data-sub-id="' + i + '">'
                            + (i != 0 ? '<i class="up_path"></i>' : '')
                            + (i != route.paths.length - 1 ? '<i class="down_path"></i>' : '')
                            + '<i class="remove_path"></i>'
                            + '</td>';
                html += '</tr>';
            }

            if (route.dynamic) {
                html += '<tr class="route">'
                     +  '<td style="background-color:#c0c0c0;">D</td>'
                     +  '<td style="background-color:#c0c0c0;">'+route.dynamic.flapping+' sec</td>'
                     +  '<td style="background-color:#c0c0c0;">'+route.dynamic.drop_threshold+'%</td>'
                     +  '<td style="background-color:#c0c0c0;">'+route.dynamic.util_threshold+'%</td>'
                     +  '<td style="background-color:#c0c0c0;">'+route.dynamic.metrics+'</td>'
                     +  '<td style="background-color:#c0c0c0;padding:0px;"><button class="predict_path_'+route.id+' blue" style="padding:2.5px;">Predict</button></td>'
                     +  '<td style="background-color:#c0c0c0;" data-id="' + route.id + '"><i class="remove_path del_dynamic"></i></td>'
                     +  '</tr>';
            }

            html += '<tr><td colspan="7">'
                 //+  '<button class="hide_routes fourth blue" style="padding:2.5px;">Hide all</button>'
                 +  '<button class="add_path_'+route.id+' fourth blue" style="padding:2.5px;">Add new path</button>'
                 +  '</td></tr>';
            html += '</tbody>';
            html += '</table>';
        });

        html += '<button class="hide_routes fourth blue">Hide all routes</button>';
        //html += '<button class="add_path fourth blue">Add new path</button>';

        return html;
    }

    function getNewPathHtml (prevPath) {
        var html = '';

        html += '<table style="padding-bottom:10px">';
        html += '<tr><td>Path type</td><td><select size="1" name="type" id="type" style="width:100px">'
             +  '<option value="static">Static</option>'
             +  '<option value="dynamic">Dynamic</option>'
             +  '</select></td></tr>';

        html += '<tr><td>Metrics</td><td><select size="1" name="metrics" id="metrics" style="width:100px">';
        for (var key in MetricsTypes)
            html += '<option value="'+key+'"> '+MetricsTypes[key]+' </option>';
        html += '</select></td></tr>';

        html += '<tr><td>(In|Ex) clude list</td><td><select size="1" id="clude" style="width:100px">'
             +  '<option value="none"> none </option>'
             +  '<option value="include"> include </option>'
             +  '<option value="exclude"> exclude </option>'
             +  '</select></td></tr>';

        html += '<tr><td>Flapping</td><td><input type="text" id="flapping" style="width:100px" value="'+prevPath.flapping+'"></td></tr>'
             +  '<tr><td>Drop trigger</td><td><input type="text" id="drop" style="width:100px" value="'+prevPath.dropThreshold+'"></td></tr>'
             +  '<tr><td>Util trigger</td><td><input type="text" id="util" style="width:100px" value="'+prevPath.utilThreshold+'"></td></tr>';

        html += '</table>';
        html += '<button type="button" id="add_path" class="blue half">Add path</button>';
        html += '<button type="button" id="cancel_path" class="rosy half">Cancel</button>';

        return html;
    }

    function togglePath (route, elem, pathId) {
        route.toggle(pathId);

        if (elem.classList.contains('active')) {
            elem.style.background = '#999999';
            elem.classList.remove('active');
        } else {
            elem.style.background = route.paths[pathId].color;
            elem.classList.add('active');
        }
    }

    function getRoutesFilters(filters) {
        var html = '';
        var services = Routes.services;

          html += '<div class="routes--filters--row">';
            html += '<div class="routes--filters--column">';
              html += '<h2 class="routes--filters--column--title">' +
                      'Services' + '</h2>';
              html += '<div class="routes--filters--column--data services">'
              services.forEach(function(service) {
                html += '<div class="routes--filters--line"><label class="routes--filters--label" for="' + service + '">' + service + '</label>' +
                    '<input type="checkbox" id="' + service +
                    '" class="routes--filters--checkbox js-service" ' +
                        (filters && filters.services && filters.services.indexOf(service) > -1 ?
                            'checked' : ''
                        ) + '>' +
                    '</div>';
              });
              html += '</div>'
            html += '</div>';
          html += '</div>';
          html += '<h2 class="routes--filters--column--title">' +
                    'Path' + '</h2>';
          html += '<div class="routes--filters--line">';
            html += '<div class="routes--filters--line"><label> From </label><input type="text" class="js-from" value="' +
                    (filters && filters.from ? filters.from : '') + '"></div>';
            html += '<div class="routes--filters--line"><label> To </label><input type="text" class="js-to" value="' +
                       (filters && filters.to ? filters.to : '') + '"></div>';
          html += '</div>';

          html += '<div class="routes--filters--line routes--filters--line_end">';
            html += '<button class="js-reset_input fourth blue">Reset</button>';
            html += '<button class="js-find_routes fourth blue">Find</button>';
          html += '</div>';
        return html;
    }
/* **** UI Routes END **** */

/* **** UI Tables **** */
    function showGroupTable (dpid, visible_id) {
        var variable_cols = { //  alias          width     sub-menu   visibility
            'group_id'      : ["group-id",      "100px",    '',         true],
            'type'          : ["type",          "100px",    '',         true],
            'service'       : ["service",       "100px",    '',         true],
            'duration_sec'  : ["age (sec)",     "100px",    '',         true],
            'ref_count'     : ["references",    "100px",    '',         true],
            'packet_count'  : ["count (pkts)",  "100px",    '',         true],
            'byte_count'    : ["count (bytes)", "100px",    '',         true]
        };

        var bucket_cols = {
            'weight'        : ["weight",        "100px",    '',         true],
            'watch_port'    : ["watch-port",    "100px",    '',         true],
            'watch_group'   : ["watch-group",   "100px",    '',         true],
            'actions'       : ["actions",       "200px",    '',         true],
            'packet_count'  : ["count (pkts)",  "100px",    '',         true],
            'byte_count'    : ["count (bytes)", "100px",    '',         true]
        };

        var group_owners = {
            'InBand'        : [5000,  6000],
            'LAG'           : [35000, 36000],
            'Mirror'        : [40001, 41001], // TODO: should shift in mirror-app!
            'BridgeDomain'  : [50000, 60000],
            'Multicast'     : [60000, 61000],
            'Unknown'       : [0, 0]
        };

        function getOwner (group_id) {
            var id = Number(group_id);

            for (var key in group_owners) {
                if (id >= group_owners[key][0] && id < group_owners[key][1])
                    return key;
            }
            return "Unknown";
        }

        var hovered = typeof(dpid) == "string" ? Net.getHostByID(dpid) : HCI.getHovered();
        var responses = 0;
        var data = [], filters = {}, columns = {}, subcolumns = {};
        filters = group_owners;

        for (var key in variable_cols) {
            columns[key] = {
                alias    : variable_cols[key][0],
                width    : variable_cols[key][1],
                isActive : variable_cols[key][3]
            }
        }

        for (var key in bucket_cols) {
            subcolumns[key] = {
                alias    : bucket_cols[key][0],
                width    : bucket_cols[key][1],
                isActive : bucket_cols[key][3]
            }
        }

        function findGroupInData (group_id) {
            if (!responses) return undefined;

            return data.find(function(elem) {
                if (elem.row.group_id == group_id)
                    return true;
            });
        }

        Server.ajax('GET', '/switches/'+hovered.id+'/group-desc/', responseGroup);
        Server.ajax('GET', '/switches/'+hovered.id+'/group-stats/', responseGroup);

        function responseGroup (response) {
            prepareData();
            if (responses != 0) { // already handled once
                if (data.length == 0)
                    UI.createHint("No groups on switch " + hovered.id, {fail: true});
                else {
                    UI.components.tableWithFilter(hovered.id, 'Group table on switch #' + hovered.id +
                                                  ' ('+utils.cutLength(hovered.name, 40)+')',
                                                   columns, filters, data, onUpdate);
                }
            }
            responses++;

            function onUpdate(cb) {
                var responses = 0;
                data = [];
                Server.ajax('GET', '/switches/'+hovered.id+'/group-desc/', responseUpdate);
                Server.ajax('GET', '/switches/'+hovered.id+'/group-stats/', responseUpdate);

                function responseUpdate(_response) {
                    response = _response;
                    prepareData();
                    if (responses != 0)
                        cb(columns, filters, data);
                    responses++;
                }
            }

            function prepareData () {
                if (response["_size"] == 0) return;
                response.array.forEach(function(group) {
                    var id = group.group_id;
                    var obj = findGroupInData(id);
                    if (!obj) {
                        obj = {}; obj.row = {};
                        obj.subrows = [];
                        obj.subrow_visible = (id == visible_id);
                        obj.subcolumns = subcolumns;
                        obj.name = getOwner(id);
                        data.push(obj);
                    }
                    fillGroup(group, obj);
                    fillBucket(group, obj);
                });

                function fillGroup (group, obj) {
                    for (var elem in variable_cols) {
                        if (group[elem])
                            obj.row[elem] = group[elem];
                        if (elem == "service")
                            obj.row[elem] = obj.name;
                    }
                }

                function fillBucket (group, obj) {
                    if (group["buckets"] || group["bucket_stats"]) {
                        var buckets = group["buckets"] || group["bucket_stats"];
                        for (var i = 0, l = buckets.length; i < l; i++) {
                            if (!obj.subrows[i])
                                obj.subrows[i] = {};
                            for (var elem in bucket_cols) {
                                if (buckets[i][elem]) {
                                    if (elem == "actions")
                                        obj.subrows[i][elem] = parseActions(buckets[i][elem]);
                                    else
                                        obj.subrows[i][elem] = buckets[i][elem];
                                }
                            }
                        }
                    }
                }

                function parseActions (act) {
                    var ret = '';
                    for (var akey in act) {
                        ret += flow_parser.parse_action(akey,  act[akey]);
                    }
                    return ret;
                }
            } // end PrepareData
        } // end responseGroup
    } // end showGroupTable

    function showMeterTable (dpid, visible_id) {
        var variable_cols = { //  alias          width     sub-menu   visibility
            'meter_id'      : ["meter-id",      "100px",    '',         true],
            'flags'         : ["flags",         "100px",    '',         true],
            'service'       : ["service",       "100px",    '',         true],
            'duration_sec'  : ["age (sec)",     "100px",    '',         true],
            'packet_count'  : ["count (pkts)",  "100px",    '',         true],
            'byte_count'    : ["count (bytes)", "100px",    '',         true]
        };

        var band_cols = {
            'type'          : ["type",          "100px",    '',         true],
            'rate'          : ["rate",          "100px",    '',         true],
            'burst_size'    : ["burst-size",    "100px",    '',         true]
        };

        var meter_owners = {
            'DOS-Control'   : [950, 951],
            'BridgeDomain'  : [1, 901],
            'Unknown'       : [0, 0]
        };

        var meter_type = {
            1      : "DROP",
            2      : "DSCP_REMARK",
            0xFFFF : "EXPERIMENTER"
        }

        function getOwner (meter_id) {
            var id = Number(meter_id);

            for (var key in meter_owners) {
                if (id >= meter_owners[key][0] && id < meter_owners[key][1])
                    return key;
            }
            return "Unknown";
        }

        var hovered = typeof(dpid) == "string" ? Net.getHostByID(dpid) : HCI.getHovered();
        var responses = 0;
        var data = [], filters = {}, columns = {}, subcolumns = {};
        filters = meter_owners;

        for (var key in variable_cols) {
            columns[key] = {
                alias    : variable_cols[key][0],
                width    : variable_cols[key][1],
                isActive : variable_cols[key][3]
            }
        }

        for (var key in band_cols) {
            subcolumns[key] = {
                alias    : band_cols[key][0],
                width    : band_cols[key][1],
                isActive : band_cols[key][3]
            }
        }

        function findMeterInData (meter_id) {
            if (!responses) return undefined;

            return data.find(function(elem) {
                if (elem.row.meter_id == meter_id)
                    return true;
            });
        }

        Server.ajax('GET', '/switches/'+hovered.id+'/meter-config/', responseMeter);
        Server.ajax('GET', '/switches/'+hovered.id+'/meter-stats/', responseMeter);

        function responseMeter (response) {
            prepareData();
            if (responses != 0) { // already handled once
                if (data.length == 0)
                    UI.createHint("No meters on switch " + hovered.id, {fail: true});
                else {
                    UI.components.tableWithFilter(hovered.id, 'Meter table on switch #' + hovered.id +
                                                  ' ('+utils.cutLength(hovered.name, 40)+')',
                                                   columns, filters, data, onUpdate);
                }
            }
            responses++;

            function onUpdate(cb) {
                var responses = 0;
                data = [];
                Server.ajax('GET', '/switches/'+hovered.id+'/meter-config/', responseUpdate);
                Server.ajax('GET', '/switches/'+hovered.id+'/meter-stats/', responseUpdate);

                function responseUpdate(_response) {
                    response = _response;
                    prepareData();
                    if (responses != 0)
                        cb(columns, filters, data);
                    responses++;
                }
            }

            function prepareData () {
                if (response["_size"] == 0) return;
                response.array.forEach(function(meter) {
                    var id = meter.meter_id;
                    var obj = findMeterInData(id);
                    if (!obj) {
                        obj = {}; obj.row = {};
                        obj.subrows = [];
                        obj.subrow_visible = (id == visible_id);
                        obj.subcolumns = subcolumns;
                        obj.name = getOwner(id);
                        data.push(obj);
                    }
                    fillMeter(meter, obj);
                    fillBand(meter, obj);
                });

                function fillMeter (meter, obj) {
                    for (var elem in variable_cols) {
                        if (meter[elem])
                            obj.row[elem] = meter[elem];
                        if (elem == "service")
                            obj.row[elem] = obj.name;
                    }
                }

                function fillBand (meter, obj) {
                    if (meter["bands"]) {
                        var bands = meter["bands"];
                        for (var i = 0, l = bands.length; i < l; i++) {
                            if (!obj.subrows[i])
                                obj.subrows[i] = {};
                            for (var elem in band_cols) {
                                if (bands[i][elem]) {
                                    if (elem == "type")
                                        obj.subrows[i][elem] = parseBandType(bands[i][elem]);
                                    else
                                        obj.subrows[i][elem] = bands[i][elem];
                                }
                            }
                        }
                    }
                }

                function parseBandType (type) {
                    if (meter_type.hasOwnProperty(type)) return meter_type[type];
                    return type;
                }

            } // end PrepareData
        } // end responseMeter
    } // end showMeterTable

    function showFlowTables () {
        var app_mask = {
            'Discovery'     : 0x11D0,
            'QoS-Classifier': 0xA5C0,
            'InBand'        : 0xE1F0,
            'InBand-def'    : 0xDEF0,
            'Multicast'     : 0xCA50,
            'BridgeDomain'  : 0xBBD0,
            'Metering-SC'   : 0xB5C0,
            'LAG'           : 0x1A60,
            'Mirror'        : 0x8A80
        };
        var variable_cols = { //  alias          width     sub-menu   visibility
            'number'        : ["#",             "50px",     '',         false],
            'table_id'      : ["table-id",      "100px",    '',         true],
            'in_port'       : ["in-port",       "50px",     'match',    true],
            'vlan_vid'      : ["vlan-vid",      "70px",     'match',    true],
            'vlan_pcp'      : ["vlan-pcp",      "70px",     'match',    true],
            'eth_src'       : ["eth-src",       "150px",    'match',    true],
            'eth_dst'       : ["eth-dst",       "150px",    'match',    true],
            'eth_type'      : ["eth-type",      "60px",     'match',    true],
            'arp_spa'       : ["arp-spa",       "110px",    'match',    false],
            'arp_sha'       : ["arp-sha",       "150px",    'match',    false],
            'arp_tpa'       : ["arp-tpa",       "110px",    'match',    false],
            'arp_tha'       : ["arp-tha",       "150px",    'match',    false],
            'ipv4_src'      : ["ipv4-src",      "110px",    'match',    true],
            'ipv4_dst'      : ["ipv4-dst",      "110px",    'match',    true],
            'ip_proto'      : ["ip-proto",      "60px",     'match',    true],
            'metadata'      : ["meta",          "60px",     'match',    true],
            'cookie'        : ["cookie",        "100px",    '',         true],
            'service'       : ["service",       "100px",    '',         true],
            'priority'      : ["priority",      "100px",    '',         true],
            'instructions'  : ["instructions",  "230px",    '',         true],
            'duration_sec'  : ["age (sec)",     "100px",    '',         true],
            'packet_count'  : ["count (pkts)",  "100px",    '',         true],
            'byte_count'    : ["count (bytes)", "100px",    '',         true]
        };
        var OF_VLAN_PRESENT = 4096;

        var hovered = HCI.getHovered();
        Server.ajax("GET", '/switches/'+hovered.id+'/flow-tables/', drawFlowTables);

        function drawFlowTables (response) {
            // load_flow_table_cols();

            var data = [], filters = {}, columns = {};

            preapareData();
            if (data.length == 0)
                UI.createHint("No flows on switch " + hovered.id, {fail: true});
            else {
                UI.components.tableWithFilter(hovered.id, 'Flow table on switch #' + hovered.id +
                                              ' ('+utils.cutLength(hovered.name, 40)+')',
                                               columns, filters, data, onUpdate);
            }

            function onUpdate(cb) {
                Server.ajax('GET', '/switches/'+hovered.id+'/flow-tables/', function(_response) {
                    response = _response;
                    preapareData();
                    cb(columns, filters, data);
                });
            }

            function preapareData() {
                if (response["_size"] == 0) return;
                data = [];
                filters = {};
                columns = {};
                for (var key in variable_cols) {
                    columns[key] = {
                        alias    : variable_cols[key][0],
                        width    : variable_cols[key][1],
                        isActive : variable_cols[key][3]
                    }
                }

                filters = app_mask;

                var counter = 0;
                response.array.forEach(function(flow) {
                    var flow_app = '', tempData = {},
                        mask = flow.cookie & 0xFFFF;
                    for (var key in app_mask) {
                        if (mask == app_mask[key]) {
                            flow_app = key;
                            break;
                        }
                    }

                    // filters[flow_app] = true;
                    tempData.name = flow_app;
                    tempData.row = {};

                    for (var elem in variable_cols) {
                        var params = variable_cols[elem];
                        if (elem == 'number') {
                            tempData.row[elem] = ++counter;
                            continue;
                        }
                        if (elem == 'service') {
                            tempData.row[elem] = flow_app;
                            continue;
                        }
                        var disp = (params[2] == '' ? flow[elem] : flow[params[2]][elem]);
                        tempData.row[elem] = flow_parser.parsing(elem, disp);
                    }

                    data.push(tempData);
                });
            }

        } //end drawFlowTables()

    } // end showFlowTables()
/* **** UI Tables END **** */

    function showGeneralSettings() {
	/*
        var html = getHtml();

        var menu = createMenu(html, {name: "gen_set", unique: true, title : 'UI settings', pos: "left", width : '250px'});

        var eCheckboxShown = menu.element('background--is-shown');
        var eCheckboxDraggable = menu.element('background--is-draggable');
        var eInputFile = menu.element('background--upload');
        var eButtonReset = menu.element('background--reset');
        var eButtonStateSave = menu.element('ui-state-save');

        eCheckboxDraggable.onchange = onChangeCheckboxDraggable;
        eCheckboxShown.onchange = onChangeCheckboxShown;
        eInputFile.onchange = onChangeInputFile;
        eButtonReset.onclick = onClickReset;
        eButtonStateSave.onclick = onClickStateSave;

        function getHtml() {
            var html = '';

            html += '<p>';
                html += '<label for="background--is-shown">Is shown</label>';
                html += '<input type="checkbox" ' + (Background.isShown() ? 'checked' : '') + ' id="background--is-shown">';
            html += '</p>';

            html += '<p>';
                html += '<label for="background--is-draggable">Is draggable</label>';
                html += '<input type="checkbox" ' + (Background.isDraggable() ? 'checked' : '') + ' id="background--is-draggable">';
            html += '</p>';

            html += '<p>';
                html += '<label for="background--upload">Upload new image</label>';
                html += '<input type="file" id="background--upload">';
            html += '</p>';

            html += '<p>';
                html += '<button id="background--reset" class="half blue">Reset</button>';
            html += '</p>';

            html += '<h2> UI state </h2>';
            html += '<button id="ui-state-save" class="half blue">Save</button>';

            return html;
        }

        function onChangeCheckboxShown() {
            Background.setShown(this.checked);
        }

        function onChangeCheckboxDraggable() {
            Background.setDraggable(this.checked);
        }

        function onChangeInputFile() {
            var file = this.files[0];
            if (file) {
                Background.upload(file);
            }
        }

        function onClickReset() {
            Background.reset();
        }

        function onClickStateSave() {
            ProxySettings.sendUiState(function(res) {
                UI.createHint('Settings was successfully saved');
            });
        }
	*/
    }

}();

/* components */


UI.components.tableWithFilter = function(dpid, title, columns, filters, tableData, update) {
    var html = '';
    var styles = {
        'container' : {
            'overflow-y' : 'auto'
        },
        'header' : {
            'display'       : 'block',
            'width'         : '700px',
            'margin-left'   : '2%',
            'position'      : 'relative'
        },
        'heading' : {
            'background': 'linear-gradient(to right, #BA55D3, #000080)',
            'color': 'white',
            'font-size': '1.25em',
            'padding': '10px 10px',
            'border-top-left-radius': '10px',
            'border-top-right-radius': '10px'
        },
        'header-content' : {
            'display'   : 'flex',
            'flex-wrap' : 'wrap',
            'padding'   : '15px 0',
            'background': '#E6E6FA',
            'position'  : 'relative'
        },
        'main-elem' : {
            'flex-grow'     : '0',
            'flex-shrink'   : '0',
            'margin-bottom' : '10px',
            'display'       : 'flex',
            'align-items'   : 'center'
        },
        'main-elem-text' : {
            'margin-right'  : '10px',
            'margin-left'   : '20px'
        },
        'link' : {
            'color': 'forestgreen',
            'text-decoration': 'underline',
            'cursor': 'pointer'
        }
    };

    html += '<div style="' + UI.getStyleString(styles['header']) + '">';
        html += '<h3 style="' + UI.getStyleString(styles['heading']) + '">Applications</h3>';
        html += '<ul style="' + UI.getStyleString(styles['header-content'])  + '" id="headings">';
            html += getHtmlHeadings();
        html += '</ul>';

        html+= '<div class="custom_block">';
            html += '<input ' +
                        'class="js-input-filter" ' +
                        'type="text" placeholder="filter"' +
                    '/>';
            html += '<button class="table_with_filter--refresh blue" id="refresh">' +
                    '<img class="global__routes--refresh__button--image" src="../images/sync.png">' +
                '</button>';
        html += '</div>';
    html += '</div>';


    html += '<div class="flow_table_div" style="' + UI.getStyleString(styles.container) + '">';
    html += '<table class="arccn" id="flow_table" style="margin:0;">';
    html += '<thead>';
    html += '<tr>';
    html += '<th colspan="2">' + title + '</th>';
    html += '<th colspan="2" style="text-align:right"><i class="flow_settings" id="filter"></i></th>';
    html += '</tr>';
    html += '<tr style="font-weight:bold">';
    for (var key in columns) {
        var column = columns[key];
        html += '<td class="'+key+'" width="' + column.width + '">'+column.alias+'</td>';
    }
    html += '</tr>';
    html += '</thead>';
    html += '<tbody style="max-height: calc(100vh - 400px); overflow-y: auto;" id="body">';
        html += getHtmlBody();
    html += '</tbody>';
    html += '</table>';
    html += '</div>';

    var menu = UI.createMenu(html, {title : title, pos: "left", width : '80%'});
    hide_invisible(menu);

/* Events */
    menu.element('filter').onclick = function () {show_cols(menu)};
    menu.querySelector('.flow_table_div').addEventListener('mousedown', function(event) {  event.stopPropagation(); })
    menu.querySelector('.js-input-filter').oninput = onInputFilter;
    menu.element('refresh').onclick = onClickRefresh;
    onClickActiveRow();
    onClickHyperlinks();

    scroll_to_visible_subrow(menu);

    var t = menu.querySelectorAll('input');
    for (var i = 0, len = t.length; i < len; i++) {
        t[i].onchange = onChangeInputApp;
    }

    function onClickHyperlinks () {
        var active = menu.querySelectorAll('a.active_element');
        for (var i = 0, len = active.length; i < len; i++) {
            active[i].style = UI.getStyleString(styles['link']);
            active[i].onclick = function () {
                if (this.getAttribute('info-group'))
                    UI.showGroupTable(dpid, this.getAttribute('info-group'));
                else if (this.getAttribute('info-meter'))
                    UI.showMeterTable(dpid, this.getAttribute('info-meter'));
            }
        }
    }

    function onClickActiveRow () {
        var active_row = menu.querySelectorAll('.active');
        for (var i = 0, len = active_row.length; i < len; i++) {
            active_row[i].onclick = function () {
                this.nextSibling.classList.toggle('is-hidden');
            }
        }

        var sub = menu.querySelectorAll('.table_with_filter--subrow');
        for (var i = 0, len = sub.length; i < len; i++) {
            sub[i].classList.toggle('is-hidden');
        }
    }

    function onChangeInputApp() {
        var name = this.id;
        if (!name) return;

        if (name == 'all') {
            for (i = 0, len = t.length; i < len; i++) {
                t[i].checked = this.checked;
            }
        }
        toggle_rows(name, this.checked);
    }

    function onInputFilter() {
        var filter = makeFilter(menu.querySelector('.js-input-filter').value),
            eRows = menu.querySelector('tbody').querySelectorAll(filter.service ? ('.' + filter.service) : 'tr');

        var regExp = new RegExp(filter.search, 'i');

        Array.prototype.forEach.call(eRows, function(eRow){
            if (eRow.classList.contains("subrow") || eRow.classList.contains("table_with_filter--subrow")) return;

            var eCells = eRow.querySelectorAll(filter.field ? ('.' + filter.field) : 'td'),
                i, n = eCells.length;

            for(i = 0; i < n; ++i) {
                if (regExp.test(eCells[i].innerHTML)) {
                    eRow.classList.contains('is-hidden') && eRow.classList.remove('is-hidden');
                    return;
                }
            }

            eRow.classList.add('is-hidden');
            if (eRow.classList.contains('active')) {
                eRow.nextSibling.classList.add('is-hidden');
            }
        });
        update_numbering();
    }

    function onClickRefresh() {
        update(function(_columns, _filters, _tableData) {
            columns = _columns;
            filters = _filters;
            tableData = _tableData;

            menu.element('body').innerHTML = getHtmlBody();
            menu.element('body').scrollIntoView(true);
            hide_invisible(menu);

            onInputFilter();
            onClickActiveRow();
            onClickHyperlinks();

            var eCheckboxApps = menu.element('headings').querySelectorAll('input');
            Array.prototype.forEach.call(eCheckboxApps, function(eCheckboxApp) {
                if (eCheckboxApp.id == 'all') return;
                toggle_rows(eCheckboxApp.id, eCheckboxApp.checked);
            });
        });
    }
/* Events End */

    function show_cols(parent) {
        var html = '';

        html += '<table class="arccn">';
        html += '<thead><tr>';
        html += '<th>Displayed fields</th>';
        html += '</tr></thead>';
        html += '<tbody>';
        for (var key in columns) {
            var column = columns[key];
            html += '<tr><td>'+column.alias+'</td>';
            html += '<td style="width:60px"><input type="checkbox" id="'+key+'" '+(column.isActive ? 'checked' : "")+'></td></tr>';
        }
        html += '</tbody>';
        html += '</table>';
        html += '<button class="blue half" id="save_fields">Save</button>';
        html += '<button class="rosy half" id="disc_fields">Discard</button>';

        var menu = UI.createMenu(html, {width: "250px", pos: "center"});

        menu.element('disc_fields').onclick = menu.remove;
        menu.element('save_fields').onclick = function () {
            for (var key in columns)
                Server.setCookie(key, menu.element(key).checked);
            menu.remove();
            load_cols();
            hide_invisible(parent);
        }
    }

    function load_cols() {
        for (var key in columns) {
            if (!columns.hasOwnProperty(key)) continue;

            var vis = Server.getCookie(key);
            if (vis) {
                columns[key].isActive = vis == "true";
            }
        }
    }

    function hide_invisible(table_menu) {
        load_cols();
        var tds = table_menu.element('flow_table').querySelectorAll('td');
        for (var i = 0, l = tds.length; i < l; i++) {
            var col = tds[i];
            if (col.className == "subrow" || columns[col.className].isActive)
                col.style.display = 'table-cell';
            else
                col.style.display = 'none';
        }
    }

    function scroll_to_visible_subrow(table_menu) {
        var elems = table_menu.getElementsByClassName('table_with_filter--subrow');
        var visible_elem;
        for (var i = 0, l = elems.length; i < l; ++i) {
            if (!elems[i].classList.contains('is-hidden')) {
                visible_elem = elems[i];
                break;
            }
        }

        if (visible_elem) {
            table_menu.element('body').scrollTop = visible_elem.offsetTop - 70;
        }
    }

    function toggle_rows(name, isChecked) {
        var rows, len, i;
        if (name == 'all') {
            rows = menu.querySelector('tbody').querySelectorAll('tr.table_with_filter--row');
        } else {
            rows = menu.querySelectorAll('.' + name);
        }
        for (i = 0, len = rows.length; i < len; i++) {
            rows[i].style.display = (isChecked ? '' : 'none');
            if (rows[i].classList.contains('active') && !isChecked)
                rows[i].nextSibling.classList.add('is-hidden');
        }
        update_numbering();
    }

    function update_numbering() {
        var rows = menu.querySelector('tbody').querySelectorAll('tr.table_with_filter--row'),
            counter = 0, len, i;
        for (i = 0, len = rows.length; i < len; i++) {
            if (rows[i].style.display != 'none' && !rows[i].classList.contains('is-hidden')) {
                if (rows[i].querySelector('.number'))
                    rows[i].querySelector('.number').innerHTML = ++counter;
            }
        }
    }

    function makeFilter(value) {
        if (typeof value != 'string') return;

        var service = null,
            field = null,
            search = value,
            tmp = '';

        if (/^#[^:]*: */i.test(value)) {
            tmp = value.match(/^#([^:]*): *(.*)/i);
            if (checkService(tmp[1])) {
                service = tmp[1];
                search = tmp[2] || '.*';
            } else if (checkField(tmp[1])) {
                field = tmp[1];
                search = tmp[2] || '.*';
            }
        }

        function checkService(name) {
            for (var key in filters) {
                if (key == name) {
                    return true;
                }
            }
        }

        function checkField(name) {
            for (var key in columns) {
                // var column = columns[key];
                if (key == name) {
                    return true;
                }
            }
        }

        return {
            service : service,
            field   : field,
            search  : search
        }
    }

    function getHtmlHeadings() {
        var html = '';

        html += '<li style="' + UI.getStyleString(styles['main-elem']) + '">'
        html += '<span style="' + UI.getStyleString(styles['main-elem-text']) + '">all</span>';
        html += '<input type="checkbox" id="all" checked>'
        html += '</li>';
        for (var key in filters) {
            html += '<li style="' + UI.getStyleString(styles['main-elem']) + '">'
            html += '<span style="' + UI.getStyleString(styles['main-elem-text']) + '">' + key + '</span>';
            html += '<input type="checkbox" id="' + key + '"' + (filters[key] ? "checked" : "") + ' >';
            html += '</li>';
        }

        return html;
    }

    function getHtmlBody() {
        var html = '';
        tableData.forEach(function(data) {
            html += '<tr class="'+data.name +' table_with_filter--row'+(data.subrows ? " active" : "")+'">';
            for (var key in columns) {
                var column = columns[key];
                html += '<td class="'+key+'" width="' + column.width + '">'+ data.row[key] +'</td>';
            }
            html += '</tr>';

            if (data.subrows) {
                html += '<tr class="table_with_filter--subrow ' + (data.subrow_visible ? 'is-hidden' : '') + '"><td class="subrow">';
                    html += getHtmlSubrow(data);
                html += '</td></tr>';
            }

        });
        return html;
    }

    function get_subrow_table_width (data) {
        if (!data.subcolumns) return 0;

        var ret = 0;
        for (var key in data.subcolumns) {
            var width = data.subcolumns[key].width;
            var num_width = Number(width.substr(0, width.length-2));
            ret += num_width;
        }
        return ret;
    }

    function getHtmlSubrow (data) {
        var html = '';
        html += '<table width="'+get_subrow_table_width(data)+'px" class="table--subrow"><thead><tr style="font-weight:bold" class="subrow">';
        for (var key in data.subcolumns) {
            var column = data.subcolumns[key];
            html += '<td class="subrow" width="' + column.width + '">'+column.alias+'</td>';
        }
        html += '</tr></thead>';
        html += '<tbody style="max-height: 600px; overflow-x: auto;overflow-y: hidden;" id="body">';
        data.subrows.forEach(function(row) {
            html += '<tr class="subrow">';
            for (var key in data.subcolumns) {
                var column = data.subcolumns[key];
                html += '<td class="subrow" width="' + column.width + '">'+ row[key] +'</td>';
            }
            html += '</tr>';
        });
        html += '</tbody>';
        html += '</table>';
        return html;
    }

    return html;
};
