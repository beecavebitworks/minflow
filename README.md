# minflow
Prototype for a real-time summarized netflow for low-end firewalls.

Using the packet and DNS data, home/SMB routers could provide meaningful insight on security (do I have malware on my network?) and performance (why is my network slow?).
However, If you've ever tried [dd-wrt with rflow] (http://www.dd-wrt.com/wiki/index.php/Using_RFlow_Collector_and_MySQL_To_Gather_Traffic_Information) on your commercial router, you've seen that it's too heavy for most devices.  At $100-$200, these ARM based devices rely on software routing.  Since the system's stateful firewall is already tracking connections for stateful firewall reasons, why not share this data via [proc] (http://linux.die.net/man/5/proc) to save the overhead.  Privileged processes will then have read-only access to this information for mining the data, such as to periodically calculating the Top-N streams, addresses, and ports as well as looking for anomalies such as fan-out or port scanning.  

## Status
This project is still in the early stages, with no kernel work yet.  The prototype has user processes minflowcap (captures packets via libpcap) and rminflow (reads existing pcaps) to extract network information and populate the shared data structures.  A separate process reads the shared data every second and generates summary reports (e.g. top-N) every minute.  

I have a separate project (not included here yet) that pushes the reports to a web server.  The web server only has a basic data dashboard, but this is really where the most interesting work will be done... mining the data and alerting the user to any issues.

## Assumptions
The design requires a configurable pre-allocation system.  At initialization time, the system allocates memory based on a maximum number of addresses, streams, ports, and lans to track.  This would impact routing.  If not slots are empty for a new address, stream, or port, then the packet has to be dropped.  There's a mechanism for garbage collecting entries, but the routing impact is still a possibility.

## Report Data
  The report format and contents are subject to change.
  
  ### Per-second stats
  
  As with [SiLK] (http://tools.netsa.cert.org/silk/), high level traffic summary stats are kept for the following categories:
 - **in**  Incoming 
 - **out** Outgoing 
 - **inweb** Incoming web
 - **outweb** Outgoing web
 - **internal** Internal LAN

  ### Address and Stream Map
  The Top-N and Anomaly sections only refer to streams and addresses by an ID mapped to the tables listed in this section.
  
  Addresses have fields:
    IID, isInternal, _ipv6 or v4 address_, MAC address
  
  Streams have fields:
    IID, flowFlags, IpProto, LeftAddrID,LeftPort,RightAddrID,RightPort,Label string

  ### Top-N data
  
  Top 20 talkers (address), conversations (streams), and applications (ports) 
  
  ### Anomaly data
  
  Includes reports on which devices (MAC and IP address) are likely doing fan-out or port-scanning.
  
