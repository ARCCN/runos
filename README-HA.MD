README for RUNOS controller failover:

1. Installing software for heartbeat. 
   For example, to install on Ubuntu 12.04:

	sudo apt-get install heartbeat
	sudo apt-get install chkconfig
	
2. Create file /etc/heartbeat/ha.cf on both hosts: 

	autojoin none
	logfacility local0
	udpport 694
	bcast (eth0) eth1 
	keepalive 40ms 
	warntime 1
	deadtime 2
	#initdead 10
	auto_failback on
	node server01
	node server02
	respawn hacluster /usr/lib/heartbeat/ipfail
	use_logd yes
	debugfile /var/log/ha-debug
	logfile /var/log/ha-log

3. Create file /etc/ha.d/haresources on both hosts:
	3.1 For first node:
	
		server02 Ipaddr::10.250.30.205/24 runos

	3.2 For second node:
	
		server01 Ipaddr::10.250.30.201/24 runos

4. Create file /etc/ha.d/authkeys on both hosts:

	auth 2
	2 sha1 password

5. 	Update file etc/hosts on both hosts:
	127.0.0.1	localhost
	10.0.3.201	server01
	10.0.3.205	server02

6. Update server configuration on both hosts:

	sudo chmod 600 /etc/ha.d/authkeys
	sudo chkconfig --level 2345 heartbeat on
	
7. Start heartbeat on both hosts:
	
	/etc/init.d/heartbeart start
	or 
	service heartbeat start
	
8. Start RUNOS on first node.


	

