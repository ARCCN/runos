<img src="https://raw.githubusercontent.com/ARCCN/runos/master/logo_runos_small.jpg" alt="RUNOS logo" width="50%" height="50%">

# What is RuNOS

Runos is an OpenFlow Controller.

It is fully userspace controller with high functionality, easy to develop your apps, relatively high performance comparing with existing controllers.
It supports OpenFlow 1.3.

More info: http://arccn.github.io/runos/

Slides: http://www.slideshare.net/AlexanderShalimov/runos-openflow-controller-eng

RuNOS documentation [ru]: http://arccn.github.io/runos/doc/ru/index.html

# Build prerequirements

This components should be installed in the system:

* Utilities: cmake, autoconf, libtool, pkg-config
* libfluid dependencies: libevent openssl. NOTE : libfluid requierd libevent v2.1.5, that still in beta. We recommend you install it from source.
* Libraries: QtCore 5, google-glog, boost::graph, boost::system, boost::thread, boost::coroutine, boost::context, uglifyjs
* UglifyJS dependencies: npm, nodejs

You can use this line on Ubuntu 15.10+ to install all required packages:

```
$ sudo apt-get install build-essential cmake autoconf libtool \
    pkg-config libgoogle-glog-dev libevent-dev \
    libssl-dev qtbase5-dev libboost-graph-dev libboost-system-dev \
    libboost-thread-dev libboost-coroutine-dev libboost-context-dev \
    libgoogle-perftools-dev curl nodejs npm \
```

You need to install the JavaScript packages:

```
    $ sudo  npm install uglify-js -g

    # You maybe needed to  creating symbolic link from nodejs to node
    $ sudo ln -s /usr/bin/nodejs /usr/bin/node
```

To install libevent 2.1.5 :
```
# Get source code
$ wget https://github.com/libevent/libevent/releases/download/release-2.1.5-beta/libevent-2.1.5-beta.tar.gz
$ tar -xvf libevent-2.1.5-beta.tar.gz
$ cd libevent-2.1.5-beta
# And build
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```

To build project you must use g++-5.2 compiler (or above) or similar another compiler with support std::regex.
We recommend to install g++-5.2 by this way:

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install g++-5.2
```

# Building

```
# Initialize third-party libraries
$ third_party/bootstrap.sh

# Create out of source build directory
$ mkdir -p build; cd build
# Configure (if you use g++)
$ CXX=g++-5.2 cmake -DCMAKE_BUILD_TYPE=Release ..
# OR configure (otherwise)
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

# Using JetBrains CLion
1. Add project to CLion by `File->Open...` and select the project root's location. Further we will call it as `./`
2. Mark `./third_party` directory as Library root (right click to the directory name in `Project view` panel) and restart the IDE.
You may also need to install `libfluid` system-wide: it will be used by CLion for making smart suggestions (not for compilation).
3. Configure project.
Bad news are that Clion is still not be able to build project natively (via `Build` action), but you can build it by running a bash comand (write your own or use listed bellow one).
Good news are that graphical debugger and other aspects of using IDE works perfect!
So, `Edit configurations... -> runos`:
    * Executable: select `./build/runos`
    * Working directory: `./`
    * Environment variables:
        * `LD_LIBRARY_PATH` to `./prefix/lib` for Linux
        * `GLOG_logtostderr` to `1`
        * `GLOG_colorlogtostderr` to `1`
    * Before launch:
        * Remove `Build`
        * Add `Run external tool -> Add...`:
            * Program: `/bin/bash`
            * Parameters: `-c "cd ./build/ && make -j2 && source ../debug_run_env.sh"`
    * Mark the configuration as `Single instance only`
4. Compile and run via `Run` action or launch debugging session via `Debug` action!

# Writing your first RuNOS app

Note: look at full documentation in Russian: http://arccn.github.io/runos/doc/ru/index.html 

## Step 1: Override Application class

Create ``MyApp.cc`` and ``MyApp.hh`` files inside `src` directory and
reference them in `CMakeLists.txt`.

Fill them with the following content:

    /* MyApp.hh */
    #pragma once

    #include "Application.hh"
    #include "Loader.hh"

    class MyApp : public Application {
    SIMPLE_APPLICATION(MyApp, "myapp")
    public:
        void init(Loader* loader, const Config& config) override;
    };

    /* MyApp.cc */
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
    #include "SwitchConnection.hh"
    ...

    public slots:
        void onSwitchUp(SwitchConnectionPtr ofconn, of13::FeaturesReply fr);

And connect signal to it in `MyApp::init`:

        QObject::connect(ctrl, &Controller::switchUp,
                         this, &MyApp::onSwitchUp);

Finally, write `MyApp::onSwitchUp` implementation:

    void MyApp::onsSitchUp(SwitchConnectionPtr ofconn, of13::FeaturesReply fr)
    {
        LOG(INFO) << "Look! This is a switch " << fr.datapath_id();
    }

Great! Now you will see this greeting when switch connects to RuNOS.

## Step 3: Working with data flows

You learned how subscribe to other application events, but how to manage flows?
Imagine you wan't to do MAC filtering, ie drop all packets from hosts not listed
in the configuration file.

To do so you need to create packet-in handler. Packet-in handlers will be invoked
for each switch by `controller` application and will live in a number of threads.
So, you need define not only a "handler" but a handler factory.

So, register PacketMissHandlerFactory in `init`:

    #include "api/PacketMissHandler.hh"
    #include "api/Packet.hh"
    #include "types/ethaddr.hh"
    #include "api/TraceablePacket.hh"
    #include "oxm/openflow_basic.hh"

    void MyApp::init(Loader *loader, const Config& config)
    {
        ...

        Controller* ctrl = Controller::get(loader);
        ctrl->registerHandler("mac-filtering",
        [=](SwitchConnectionPtr connection){
            return [=](Packet& pkt, FlowPtr, Decision decision){
		if (pkt.test(oxm::eth_src() == "00:11:22:33:44:55"))
		     return decision.drop().return_();
		else
                     return decision;
            };
        });

        ...
    }

What it means? First, all handlers arranged into pipeline, where every handler
can stop processing, look at some packet fields and add actions. We need also
insert our handler in pipeline. In `network-settings.json` `controller` has his setting,
and pariculary `pipeline`, which contains name of handlers in pipeline.


So, we name our handler as "mac-filtering" and need to place it before "forwarding".

Now compile RuNOS and test that all packets from ``00:11:22:33:44:55`` had been dropped.

# REST Applications

## List of available REST services

The format of the RunOS REST requests:

    <HTTP-method> /api/<app_name>/<list_of_params>

* `<HTTP-method>` is GET, POST, DELETE of PUT
* `<app_name>` is calling name of the application
* `<list_of_params>` is list of the parameters separated by a slash

In POST and PUT request you can pass parameters in the body of the request using JSON format.

Current version of RunOS has 6 REST services:
* switch-manager
* topology
* host-manager
* flow
* static-flow-pusher
* stats

### 'Switch Manager'

    GET /api/switch-manager/switches/all 	(RunOS version)
    GET /wm/core/controller/switches/json 	(Floodlight version)
Return the list of connected switches

### 'Topology'

    GET /api/topology/links			    (RunOS version)
    GET /wm/topology/links/json 		(Floodlight version)
Return the list of all the inter-switch links

    GET /wm/topology/external-links/json 	(Floodlight)
Return external links

### 'Host Manager'

    GET /api/host-manager/hosts		(RunOS)
    GET /wm/device/			        (Floodlight)
List of all end hosts tracked by the controller

### 'Flow Manager'

    GET /api/flow/<switch_id>
    DELETE /api/flow/<switch_id>/<flow_id>
List flow entries in the switch and remove some flow entry

### 'Static Flow Pusher'

    POST /api/static-flow-pusher/newflow/<switch_id>
    body of request: JSON description of new flow
Create new flow entry in the some switch

### 'Stats'

    GET /api/stats/port_info/<switch_id>/all
    GET /api/stats/port_info/<switch_id>/<port_id>
Get switch port statistics

### Other

    GET /apps
List of available applications with REST API.

Also you can get events in the applications that subscribed to event model.
In this case you should specify list of required applications and your last registered number of events.

    GET /timeout/<app_list>/<last_event>
* `<app_list>` is `app_1&app_2&...&app_n`
* `<last_event>` is `unsigned integer` value


## Examples

For testing your and other REST application's requests and replies you can use `cURL`.
You can install this component by `sudo apt-get install curl` command in Ubuntu.

In mininet topology `--topo tree,2`, for example:

Request: `$ curl $CONTROLLER_IP:$LISTEN_PORT/api/switch-manager/switches/all`
Reply: `[{"DPID": "0000000000000001", "ID": "0000000000000001"}, {"DPID": "0000000000000002", "ID": "0000000000000002"}, {"DPID": "0000000000000003", "ID": "0000000000000003"}]`

## Adding REST for your RunOS app

To make REST for your application `class MyApp`:

* add REST for your application:

        #include "Rest.hh"
        #include "AppObject.hh"
        #include <string>
        class MyApp : public Application, RestHandler {
            bool eventable() override { return false; }
            std::string displayedName() override { return "My beautiful application"; } // or skip this
            std::string page() override { return "my_page.html"; } // if your application has webpage

            json11::Json handleGET(std::vector<std::string> params, std::string body) override;
            json11::Json handlePOST(std::vector<std::string> params, std::string body) override;
            // also handlePUT and handleDELETE
        }

* in `init()` function in `class MyApp` register Rest handler:

        RestListener::get(loader)->registerRestHandler(this);

* then, you should set the handled pathes and methods in MyApp::init:

    // handle GET request with path /api/my-app/switches/<number>
    acceptPath(Method::GET, "switches/[0-9]+");

    // handle POST request with path /api/my-app/mac/<string>
    acceptPath(Method::POST, "[A-Fa-f0-9:-]");

* method `MyApp::handleGET` proccesses input GET-HTTP requests.
  This method gets the list of parameters and returns reply in JSON format.

* each REST application may have own webpage, i.e. WebUI for application
  To connect REST application to webpage you must specify this page in `page()` method:

        std::string page() override { return "my_page.html"; }

  File `"my_page.html"` must be located in `web/html` directory.
  If your application has not WebUI, write `"none"` instead webpage.

* if your application supports event model, you can enable it by setting `true` in `eventable()` method:

        bool eventable() override { return true; }

In current version events can signal about appearance or disappearance some objects:

    # some_object is object inherited class AppObject
    addEvent(Event::Add, some_object);
    # or
    addEvent(Event::Delete, some_object);

## Static Flow Pusher

You can use static flow pusher to set rules proactively.
At first, you can write required rules in `network-settings.json` file.
For example:

    "static-flow-pusher": {
      "flows": [
        {
        "dpid": "all",
        "flows": [
            {
              "priority": 0,
              "in_port": 15,
              "ip_src": "3.3.3.3",
              "out_port" : 22
            }
         ]
         }
      ]
    }

Also, you can add other match fields: `in_port`, `eth_src`, `eth_dst`, `ip_src`, `ip_dst`, `out_port`. To set idle and hard timeout use `idle` and `hard` strings. To set `OFPP_TO_CONTROLLER` action, write in `out_port` string value `to-controller`.

Secondly, to set flows proactively, you should add to your application StaticFlowPusher application, create FlowDesc object, fill it with your match field and call `&StaticFlowPusher::sendToSwitch` method.

And thirdly, use REST POST requests of StaticFlowPusher to set new flow from REST API.
