# minflow
Prototype for a real-time summarized netflow for low-end firewalls.

Using the packet and DNS data, home/SMB routers could provide meaningful insight on security (do I have malware on my network?) and performance (why is my network slow?).
However, If you've ever tried [dd-wrt with rflow] (http://www.dd-wrt.com/wiki/index.php/Using_RFlow_Collector_and_MySQL_To_Gather_Traffic_Information) on your commercial router, you've seen that it's too heavy for most devices.  At $100-$200, these ARM based devices rely on software routing.  Since the system's stateful firewall is already tracking connections for stateful firewall reasons, why not share this data via [proc] (http://linux.die.net/man/5/proc) to save the overhead.  Privileged processes will then have read-only access to this information for mining the data, such as to periodically calculating the Top-N streams, addresses, and ports as well as looking for anomalies such as fan-out or port scanning.  

## Status
This project is still in the early stages, with no kernel work yet.  The prototype uses libpcap to extract network information and populate what the proc data structures would look like.  A separe process reads the shared data every second and generates summary reports (e.g. top-N) every minute.

## Assumptions
The design requires a configurable pre-allocation system.  At initialization time, the system allocates memory based on a maximum number of addresses, streams, ports, and lans to track.

