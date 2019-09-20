# RUNOS

## What is RUNOS?

RUNOS is a SDN/OpenFlow controller for enterprise software-defined networks, datacenter and career grade SDN networks. 

## RUNOS Key Features

* High performance and scalability is ensured by implementation in C++17 and and the using of multithreading 
* High Availability (Active/Standby)
* Modularity
* Extensibility of functionality
* REST API for external applications
* Monitoring tools (port statistics, control traffic statistics)
* Support OpenFlow version 1.3 [1]
* Support CLI for debugging
* Support Web UI: visualization of the entire network topology and detailed information about individual elements
* Documentation and support

## Getting Started

### Minimal Requirements
* RAM: 2Gb
* HDD: 8Gb
* Operating System: Ubuntu 18.04 and higher
* Browser: Google Chrome

### Dependencies


1. Install Nix package manager [2]:
``` 
curl https://nixos.org/nix/install | sh 
```

2. Install Redis in-memory data store [3]:
```
sudo apt install redis-server
```
3. Install nginx server [4]:
```
sudo apt install nginx
```



### Build RUNOS from Source
1. Getting sources:
``` 
git clone https://github.com/ARCCN/runos.git 
```

2. Run `nix-shell` inside runos directory to build dependencies:
```
cd runos
nix-shell
```

3. Create build directory, run cmake and make:
``` 
mkdir build
cd build
cmake ..
make
cd ..
```

### Start RUNOS
* Start RUNOS with default network settings file (network-settings-with-apps.json):
```
./build/runos
```

* Start RUNOS with your network settings file (your_network_settings.json):
```
./build/runos -c /path_to_file/your_network_settings.json
```

### Interacting with RUNOS via WebUI


1. Configure nginx server (edit nginx.conf):
```
sudo vim /etc/nginx/nginx.conf
```

2. Add the following text about server into http section of nginx.conf file (you need add your absolute path to the runos build directory):
```
http {
    server {
        listen 8080;
        root <...absolute/path/to/runos/build/directory...>/web;
        location ~*\.(html|css|js)$ {}
        location /images {}
        location / {
            proxy_pass http://localhost:8000;
        }
    }
}
```

3. Restart nginx server:
```
sudo service nginx restart
```

4. Start RUNOS Web UI in your browser:
```
http://$CONTROLLER_IP:8080/topology.html
```

## Quick RUNOS Test Using Mininet

1. Install Mininet network emulator [5]:
```
git clone git://github.com/mininet/mininet
cd mininet
sudo ./util/install.sh -nfv
cd ..
```

2. Start RUNOS controller:
```
cd runos
nix-shell
./build/runos
```

3. Start a simple network topology in Mininet (4 switches and 4 hosts):
```
sudo mn --topo linear,4 --switch ovsk,protocols=OpenFlow13 --controller remote,ip=127.0.0.1,port=6653
```

4. Start RUNOS Web UI, open the following link in your browser: [http://localhost:8080/topology.html](http://localhost:8080/topology.html)


## Documentation

* Install RUNOS using virtual machine:

* Install RUNOS using docker:

* English user's and developer's guide:

* Russian user's and developer's guide:


## Support

* Telegram: 

* email: runos@arccn.ru

* Wiki: 

* Official mailing list: 

## License

RUNOS SDN/OpenFlow controller is published under Apache License 2.0

## References

[1]: ONF. OpenFlow Switch Specification. Version 1.3.0 (Wire Protocol 0x04)

[2]: Nix Package Manager Guide. https://nixos.org/nix/manual/

[3]: Redis. https://redis.io/

[4]: Nginx. https://nginx.org/en/

[5]: Mininet. http://mininet.org/
