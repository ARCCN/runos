<img src="https://raw.githubusercontent.com/ARCCN/runos/master/logo_runos_small.jpg" alt="RUNOS logo" width="50%" height="50%">


# Build prerequirements

This components should be installed in the system:

* Utilities: cmake, autoconf, libtool, pkg-config
* libfluid dependencies: libevent openssl
* libtins dependencies: openssl libpcap
* Libraries: QtCore 5, google-glog, boost::graph, uglifyjs
* UglifyJS dependencies: npm, nodejs

You can use this line on Ubuntu 14.04 to install all required packages:

```
$ sudo apt-get install build-essential cmake autoconf libtool \
    pkg-config libgoogle-glog-dev libpcap-dev libevent-dev \ 
    libssl-dev qtbase5-dev libboost-graph-dev libgoogle-perftools-dev curl \
```

You need to install the latest JavaScript packages:

```
# If you have old versions of Node.js or UglifyJS,
# you should remove them
$ sudo npm un uglify-js -g
$ sudo apt-get remove node nodejs npm
# Install Node.js, npm and UglifyJS via package manager
# (according to https://github.com/joyent/node/wiki/Installing-Node.js-via-package-manager)
$ curl -sL https://deb.nodesource.com/setup | sudo bash -
$ sudo apt-get install nodejs
$ sudo npm install uglify-js -g
```

# Building

```
# Initialize third-party libraries
$ third_party/bootstrap.sh

# Create out of source build directory
$ mkdir -p build; cd build
# Configure
$ cmake -DCMAKE_BUILD_TYPE=Release ..

# Build third-party libraries
$ make prefix -j2
# Build RuNOS
$ make -j2
```

# Running

Setup environment variables (run once per shell session):
```
# Run it INSIDE build directory
$ source ../debug_run_env.sh
```

Run the controller:
```
$ cd .. # Go out of build dir
$ build/runos
```

You can use this arguments for MiniNet:
```
$ sudo mn --topo $YOUR_TOPO --switch ovsk,protocols=OpenFlow13 \
            --controller remote,ip=$CONTROLLER_IP,port=6653
```

To run web UI, open the following link in your browser:
```
http://$CONTROLLER_IP:8000/topology.html
```

Be sure your MiniNet installation supports OpenFlow 1.3.
See https://wiki.opendaylight.org/view/OpenDaylight_OpenFlow_Plugin::Test_Environment#Setup_CPqD_Openflow_1.3_Soft_Switch for more instructions.

# Using QtCreator
1. Execute Building section, but use
   `cmake -DCMAKE_BUILD_TYPE=Debug ..` instead `Release`
2. In QtCreator `File->Open File or Project`
   and select `CMakeLists.txt` as project file
3. In build section: select build directory as `./build`
4. In run section:
    * select working direcory as `./` (project path)
    * set Run Environment:
        * `LD_LIBRARY_PATH` to `$PDW/prefix/lib` for Linux
        * `DYLD_LIBRARY_PATH` to `$PWD/prefix/lib` for OS X
        * `GLOG_logtostderr` to `1`
        * `GLOG_colorlogtostderr` to `1`
5. Compile and run!

# Writing your first RuNOS app

## Step 1: Override Application class

Create ``MyApp.cc`` and ``MyApp.hh`` files inside `src` directory and
reference them in `CMakeLists.txt`.

Fill them with the following content:

    /* MyApp.cc */
    #pragma once

    #include "Application.hh"
    #include "Loader.hh"

    class MyApp : public Application {
    SIMPLE_APPLICATION(MyApp, "myapp")
    public:
        void init(Loader* loader, const Config& config) override;
    };

    /* MyApp.hh */
    #include "MyApp.hh"

    REGISTER_APPLICATION(MyApp, {""})

    void MyApp::init(Loader *loader, const Config& config)
    {
        LOG(INFO) << "Hello, world!";
    }

What we do here? We declare our application by subclassing `Application`
interface and named it `myapp`. Now we should reference `myapp` in configuration
file (defaults to `network-setting.json`) at `services` section to force run it.

Start RuNOS and see "Hello, world!" in the log.

## Step 2: Subscribing to other application events

Now our application was started but do nothing. To make it
useful you need to communicate with other applications.

Look at this line of `MyApp.cc`:

    REGISTER_APPLICATION(MyApp, {""})


In the second argument here you tell RuNOS what apps you will use as dependencies.
Last element should be empty string indicating end-of-list.

You can interact with other applications with *method calls* and *Qt signals*. Let's
subscribe to switchUp event of `controller` application. Add required dependencies:

    REGISTER_APPLICATION(MyApp, {"controller", ""})


Then include `Controller.hh` in `MyApp.cc` and get controller instance:


    void MyApp::init(Loader *loader, const Config& config)
    {
        Controller* ctrl = Controller::get(loader);

        ...
    }



Now we need a slot to process `switchUp` events. Declare it in `MyApp.hh`:

    public slots:
        void onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr);

And connect signal to it in `MyApp::init`:

        QObject::connect(ctrl, &Controller::switchUp,
                         this, &MyApp::onSwitchUp);

Finally, write `MyApp::onSwitchUp` implementation:

    void MyApp::switchUp(OFConnection* ofconn, of13::FeaturesReply fr)
    {
        LOG(INFO) << "Look! This is a switch " << FORMAT_DPID << fr.id();
    }

Great! Now you will see this greeting when switch connects to RuNOS.

## Step 3: Working with data flows

You learned how subscribe to other application events, but how to manage flows?
Imagine you wan't to do MAC filtering, ie drop all packets from hosts not listed
in the configuration file.

To do so you need to create packet-in handler. Packet-in handlers will be created
for each switch by `controller` application and will live in a number of threads.  
So, you need define not only a "handler" but a handler factory.

So, implement OFMessageHandlerFactory interface:

    ...
    #include "OFMessageHandler.hh"

    class MyApp : public Application, public OFMessageHandlerFactory {
    ...
    public:
        std::string orderingName() const override { return "mac-filtering"; }
        bool isPostreq(const std::string &name) const override { return (name == "forwarding"); }
        OFMessageHandler* makeOFMessageHandler() override { new Handler(); }
    ...
    private:
        class Handler: public OFMessageHandler {
        public:
            Action processMiss(OFConnection* ofconn, Flow* flow) override;
        };

What it means? First, all handlers arranged into pipeline, where every handler
can stop processing, look at some packet fields and add actions. To define a place
where to put our handler we may define two functions: `isPostreq(name)` and 
`isPrereq(name)`. First return `true` if we need to place our handler after handler
`name`. Otherwise second return `true` if we need to place handler before.

So, we name our handler as "mac-filtering" and need to place it before "forwarding".
Now we need to register our factory at the controller. Write this line in `MyApp::init`:

    Controller ctrl* = ...;
    ctrl->registerHandler(this);

Finally, implement the handler:

    OFMessageHandler::Action MyApp::Handler::processMiss(OFConnection* ofconn, Flow* flow)
    {
        if (flow->match(of13::EthSrc("00:11:22:33:44:55"))) {
            return Stop;
        } else return Continue;
    }

Now compile RuNOS and test that all packets from ``00:11:22:33:44:55`` had been dropped.

# REST Applications

## List of available REST services

The format of the RunOS REST requests:
`<HTTP-method> /api/<app_name>/<list_of_params>`
   `<HTTP-method>` is GET, POST, DELETE of PUT
   `<app_name>` is calling name of the application
   `<list_of_params>` is list of the parameters separated by a slash
   In POST and PUT request you can pass parameters in the body of the request using JSON format. 

Current version of RunOS has 4 REST services:
* switch-manager
* topology
* device-manager
* controller-manager

### 'Switch Manager'

* `GET /api/switch-manager/switches/all` 	(RunOS version)
* `GET /wm/core/controller/switches/json` 	(Floodlight version)
Return the list of connected switches

### 'Topology'

* `GET /api/topology/links` 			(RunOS version)
* `GET /wm/topology/links/json` 		(Floodlight version)
Return the list of all the inter-switch links

* `GET /wm/topology/external-links/json` 	(Floodlight)
Return external links

### 'Host Manager'

* `GET /api/host-manager/hosts` 		(RunOS)
* `GET /wm/device/`				(Floodlight)
List of all end hosts tracked by the controller

### 'Controller App'

* `GET /wm/core/controller/summary/json`	(Floodlight)
Controller summary (# of Switches, # of Links, etc)

* `GET /wm/core/memory/json`			(Floodlight)
Current controller memory usage

* `GET /wm/core/health/json`			(Floodlight)
Status/Health of REST API

* `GET /wm/core/system/uptime/json`		(Floodlight)
Controller uptime

* `GET /api/controller-manager/info`		(RunOS)
Return the main controller's info: address, port, count of threads, secure mode

### Other

* `GET /apps`
List of available applications with REST API.

Also you can get events in the applications that subscribed to event model.
In this case you should specify list of required applications and your last registered number of events.
* `GET /timeout/<app_list>/<last_event>`
  `<app_list>` is `app_1&app_2&...&app_n`
  `<last_event>` is unsigned integer value


## Examples

For testing your and other REST application's requests and replies you can use `cURL`.
You can install this component by `sudo apt-get install curl` command in Ubuntu.

In mininet topology `--topo tree,2`, for example:

Request: `$ curl $CONTROLLER_IP:$LISTEN_PORT/wm/topology/links/json`
Reply: `[{ "src-switch": "00:00:00:00:00:00:00:01", "src-port": 2, "dst-switch": "00:00:00:00:00:00:00:03", "dst-port": 3, "type": "internal", "direction": "bidirectional" }, { "src-switch": "00:00:00:00:00:00:00:01", "src-port": 1, "dst-switch": "00:00:00:00:00:00:00:02", "dst-port": 3, "type": "internal", "direction": "bidirectional" }]`

## Adding REST for your RunOS app

To make REST for your application `class MyApp`:

* create you REST class for your application:

        #include "rest.h"
        #include "appobject.h"
        #include <string>
        class MyAppRest : public Rest {
            MyAppRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
            std::string handle(std::vector<std::string> params) override;
        }

* add your REST to your application:

        class MyApp {
        private:
            MyAppRest* rest;
        }

* in constructor or `init()` function in `class MyApp` create instance of `MyAppRest`:

        rest = new MyAppRest("My App Name", "myapp_webui.html");


* in `startUp()` function emit conroller's method `newListener(std::string, Rest*)` to notify a new listener:

        # 'myapp' is a calling name for your application
        # rest is a instance of MyAppRest
        emit ctrl->newListener('myapp', rest);

* method `MyApp::handle` proccesses input REST requests.
  This method gets the list of parameters and returns reply in JSON format.

* each REST application may have own webpage, i.e. WebUI for application
  To connect REST application to webpage you must specify this page in Rest constructor:

        MyAppRest* rest = new MyAppRest("Displayed application's name", "myapp_webui.html");

  File `"myapp_webui.html"` must be located in `/web/deploy/` directory.
  If your application has not WebUI, write `"none"` instead webpage.

* if your application supports event model, you can enable it by the command:

        rest->makeEventApp();

In current version events can signal about appearance or disappearance some objects:

    # some_object is object inherited class AppObject
    addEvent(Event::Add, some_object);
or
    addEvent(Event::Delete, some_object);
