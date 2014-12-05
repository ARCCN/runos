# Build prerequirements

This components should be installed in the system:

* Utilities: cmake, autoconf, libtool, pkg-config
* libfluid dependencies: libevent openssl
* libtins dependencies: openssl libpcap
* Libraries: QtCore 5, google-glog, boost::graph

You can use this line on Ubuntu 14.04 to install all required packages:

```
$ sudo apt-get install build-essential cmake autoconf libtool \
    pkg-config libgoogle-glog-dev libpcap-dev libevent-dev \ 
    libssl-dev qtbase5-dev libboost-graph-dev libgoogle-perftools-dev
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
$ make prefix
# Build RuNOS
$ make
```

# Running

Setup environment variables (run once per bash session):
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
$ sudo mn --topo $YOUR_TOPO --switch ovsk,protocols=OpenFlow13 --controller remote,ip=$CONTROLLER_IP,port=6653
```

Be sure your MiniNet installation supports OpenFlow 1.3. See https://wiki.opendaylight.org/view/OpenDaylight_OpenFlow_Plugin::Test_Environment#Setup_CPqD_Openflow_1.3_Soft_Switch for more instructions.

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
