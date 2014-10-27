## Repository initialization

```
$ git submodule init
$ git submodule update --recursive
```

## Prerequirements

This components should be installed in the system:

* Utilities: cmake, autoconf, libtool, pkg-config
* libfluid dependencies: libevent openssl
* libtins dependencies: openssl libpcap
* QtCore 5
* google-glog

You can use this line on Ubuntu 14.04 to install all required packages:

```
$ sudo apt-get install build-essential cmake autoconf libtool \
    pkg-config libgoogle-glog-dev libpcap-dev libevent-dev libssl-dev qtbase5-dev
```

## Building

First, install bundled third-party libraries:

```
$ pushd third_party; ./install.sh; popd
```

Then compile project:

```
$ mkdir -p build; cd build
$ cmake ..
$ make
```

## Running

Setup environment variables (run once per bash session):
```
$ source debug_run_env.sh
```

Run the controller:
```
$ build/runos
```

You can use this arguments for MiniNet:
```
$ sudo mn --topo $YOUR_TOPO --switch ovsk,protocols=OpenFlow13 --controller remote,ip=$CONTROLLER_IP,port=6653
```

Be sure your MiniNet installation supports OpenFlow 1.3. See https://wiki.opendaylight.org/view/OpenDaylight_OpenFlow_Plugin::Test_Environment#Setup_CPqD_Openflow_1.3_Soft_Switch for more instructions.
