/*
 * This example shows basic construction of an 802.11p node.  N nodes
 * are constructed with 802.11p devices, and by default, one node sends a single
 * packet to another node (the number of packets and interval between
 * them can be configured by #define macros.  The example shows
 * typical usage of the helper classes for this mode of WiFi (where "OCB" refers
 * to "Outside the Context of a BSS")."
 */

#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/packet.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/position-allocator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/vector.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/wave-module.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"
#include <fstream>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("mymobility-example3");


/*
 * In WAVE module, there is no net device class named like "Wifi80211pNetDevice",
 * instead, we need to use Wifi80211pHelper to create an object of
 * WifiNetDevice class.
 *
 *
 * The reason of not providing a 802.11p class is that most of modeling
 * 802.11p standard has been done in wifi module, so we only need a high
 * MAC class that enables OCB mode.
 */


// Global variables:

NodeContainer nodes;

// Vehicle speed in m/s: (MPH*0.44704) (KMPH/3.6) ; Max without crash is -40.236 m/s at 175.402 m
//double speed_ms = -5.5555; // 20 km/h
//double speed_ms = -11.1111; // 40 km/h
//double speed_ms = -22.2222; // 80 km/h
double speed_ms = -33.3333; // 120 km/h

// Typical tyre to road friction coefficient:
double friction_coeff = 0.7;

// Standard Gravity force (m/s2): 
double gravity =  9.81;

// Typical reaction time to stop in seconds:
double dts = 1.5; 

// Global file stream for logging
std::ofstream csvFile;

// Safety messages are 200 bytes each 10 Hz

#define TX_POWER_DBM 16.0 // Tx Power in dBm, 32 max
#define NUM_NODES 20 // Number of nodes to create
#define DISTANCE_BETWEEN_NODES 50.0 // Distance to add between created nodes
#define PCKT_SIZE 200 // size of the dummy packet
#define NUM_PCKT 1000 // max packets to send by the traffic generator
#define TIME_INTERVAL 0.1 //seconds
#define TIME_TO_RUN 30 // seconds
#define TIME_TO_TX 1 // milliseconds
#define TTL 64 // times to re-send the original broadcast message on hops

void SendBroadcast(Ptr<Socket> socket, uint32_t pktSize, uint8_t ttlvalue)
{
 	// Get the IP of the interface this socket is bound to
    Ptr<Node> node = socket->GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ipv4Address myAddr = ipv4->GetAddress(1,0).GetLocal(); // Assuming interface 1
 	  
  	Ptr<Packet> packet = Create<Packet> (pktSize); // Your data
  	socket->SetAllowBroadcast(true);
  	Ipv4Address broadcastAddr ("255.255.255.255");
  	uint16_t port = 80;
  	
  	SocketIpTtlTag ttlTag;
	ttlTag.SetTtl(ttlvalue); // Set a specific TTL for this packet only
	packet->AddPacketTag(ttlTag);
	socket->SendTo (packet, 0, InetSocketAddress (broadcastAddr, port));
  	NS_LOG_UNCOND("\nTime: "<< Simulator::Now ().GetSeconds () << " seconds. One broadcast packet transmitted by " << myAddr << " with TTL: "<< (uint32_t)ttlvalue);
  
}


/**
 * Receive a packet
 * \param socket Rx socket
 */
 
void ReceivePacket(Ptr<NetDevice> device, Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    uint32_t pktSize = PCKT_SIZE;
    Address from; // This will hold the sender's address
    uint8_t ttlValue=0;
    packet = socket->RecvFrom(from);	// Retrieve packet and sender address	
    
    // Avoid flooding with broadcast messages
    Ipv4Address srcAddr = InetSocketAddress::ConvertFrom(from).GetIpv4();      
    // Get the IP of the interface this socket is bound to
    Ptr<Node> node = socket->GetNode();
    uint32_t id = socket->GetNode()->GetId();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ipv4Address myAddr = ipv4->GetAddress(1,0).GetLocal(); // Assuming interface 1
     // 1. Check if the packet is from OURSELVES
      
    if (InetSocketAddress::IsMatchingType(from))
    	{
         Ipv4Address senderIp = InetSocketAddress::ConvertFrom(from).GetIpv4();
         uint16_t senderPort = InetSocketAddress::ConvertFrom(from).GetPort();
         NS_LOG_UNCOND("\nTime: " << Simulator::Now ().GetSeconds () << " seconds. Packet received at Node "<<id << " (" << myAddr << ") from IP: " << senderIp << " Port: " << senderPort);
         SocketIpTtlTag ttlTag;
         // Attempt to find the TTL tag attached to the packet
         if (packet->RemovePacketTag(ttlTag))
        	{
             ttlValue = ttlTag.GetTtl();
             NS_LOG_UNCOND("Rx Packet TTL tag: " << (uint32_t)ttlValue );
        	} 
         else {
             NS_LOG_UNCOND("No TTL tag found on this packet.");
        	}
         Ptr<Node> node0 = nodes.Get (0);
    	 Ptr<MobilityModel> mobility0 = node0->GetObject<MobilityModel>();
    	 Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    	 Vector pos = mobility->GetPosition();
    	 double distance = mobility->GetDistanceFrom(mobility0);
    	 double crash = 0.0;
    	 NS_LOG_UNCOND("Pos: " << pos.x << ", Distance to Node 0: " << distance );
    	 if (id == ((NUM_NODES)-1))
    	 	{
    	 	 float distance_to_stop = (-dts*speed_ms) + ( (speed_ms*speed_ms) / (friction_coeff * gravity) );
    		 if ( distance_to_stop < distance )
        		{
        	 	 NS_LOG_UNCOND("Time to crash: " << (-distance / speed_ms) );
        		}
    		else
        		{
        	  	 crash = 100.0;
        		 if( pos.x < 0 )
        			{
        	 	 	 NS_LOG_UNCOND("Car at last node crashed to hazzard.\n\n" );
        			}
        		 else
        			{
        			 NS_LOG_UNCOND("************ CAR AT LAST NODE WILL CRASH TO HAZZARD !!!! *********** ");
        			}		
        		}
      		 csvFile <<  Simulator::Now ().GetSeconds () << "," << distance_to_stop << "," << distance << "," << crash << std::endl;
       	 	}
        }	
	else if (Inet6SocketAddress::IsMatchingType(from))
       	{
      	 Ipv6Address senderIp = Inet6SocketAddress::ConvertFrom(from).GetIpv6();
       	 NS_LOG_UNCOND("\nReceived packet from IPv6: " << senderIp);
    	}
    if (srcAddr == myAddr)
        {
            NS_LOG_UNCOND(" Received my own broadcast packet... ignoring.");   
        }
    else if (srcAddr != myAddr)
    	{
    	 ttlValue--;
    	 if(id != ((NUM_NODES)-1)) // Check if it is not the last node, because that one only receives packets
    	 	{
    	 	if (ttlValue) // Check if Time-to-live is still valid
				{
			 	 Simulator::Schedule(MilliSeconds(TIME_TO_TX), &SendBroadcast, socket, pktSize, ttlValue);
    			}
  			else
  				{
  		 	 	 NS_LOG_UNCOND("Packet not broadcasted again because it was the last hop at TTL. ");
    			}
    		}
    	}	
}


/**
 * Generate traffic
 * \param socket Tx socket
 * \param pktSize packet size
 * \param pktCount number of packets
 * \param pktInterval interval between packet generation
 */
static void GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval)
{
    if (pktCount > 0)
    	{
		 Ptr<Packet> packet = Create<Packet>(pktSize);
		 SocketIpTtlTag ttlTag;
		 ttlTag.SetTtl(TTL); // Set a specific TTL for this packet only
		 packet->AddPacketTag(ttlTag);
         socket->Send(packet);
         Simulator::Schedule(pktInterval, &GenerateTraffic, socket, pktSize, pktCount - 1, pktInterval);
         Ptr<Node> node0 = nodes.Get (0);
     	 Ptr<MobilityModel> mobility0 = node0->GetObject<MobilityModel>();
     	 Vector pos0 = mobility0->GetPosition();                    
         NS_LOG_UNCOND("\n\nTime: "<< Simulator::Now ().GetSeconds () << " seconds. One Packet transmitted by Node 0 at Pos: " << pos0.x << " By Node 0.");
      	 NS_LOG_UNCOND(" Tx Packet TTL tag: " << (uint32_t)ttlTag.GetTtl() );                   
    	}
    else
		{   
         socket->Close();
    	}
}



int main (int argc,char *argv[])
{
 	
    Time::SetResolution(Time::NS);
    
  	
  	// 1. Create 5 nodes (vehícules)
	NS_LOG_UNCOND("\nCreating the nodes for the vehicles...");  
    std::string phyMode("OfdmRate6MbpsBW10MHz");
    uint32_t nNodes = NUM_NODES;
    uint32_t packetSize = PCKT_SIZE; // bytes
    uint32_t numPackets = NUM_PCKT;
    double interval = TIME_INTERVAL; // seconds between messages to send
    //bool verbose = false;

/*  Comand line not used: 
    CommandLine cmd(__FILE__);

    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.AddValue("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue("numPackets", "number of packets generated", numPackets);
    cmd.AddValue("interval", "interval (seconds) between packets", interval);
    cmd.AddValue("verbose", "turn on all WifiNetDevice log components", verbose);
    cmd.Parse(argc, argv);
*/  
  
    // Convert to time object
    Time interPacketInterval = Seconds(interval);
    
	//NodeContainer nodes;
    nodes.Create(nNodes);

  	// 2. Configure Mobility (vehicles moving in straight line)
  	NS_LOG_UNCOND("\nConfiguring vehicles moving in straight line...");
  	NS_LOG_UNCOND("Assigning initial positions...");
  	MobilityHelper mobility;
  	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  	double pos_x=0.0, pos_y=0.0, pos_z=0.0, delta_x = DISTANCE_BETWEEN_NODES;
  	//positionAlloc->Add (Vector (0.0, 0.0, 0.0));     // node 0 starts at the origin
  	for (uint32_t i = 0; i < nNodes; ++i)
  		{
  		 positionAlloc->Add (Vector (pos_x, pos_y, pos_z));     
  		 pos_x += delta_x;
  		}
  	mobility.SetPositionAllocator (positionAlloc);

  	// Define constant speed (p. ej. 20 m/s at X axis)
  	NS_LOG_UNCOND("Installing mobility nodes...");
  	mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  	mobility.Install (nodes);

  	// Assign individual speeds
  	NS_LOG_UNCOND("Assigning individual speeds...\n");
 	nodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0.0, 0.0, 0.0));
  	for (uint32_t i = 1; i < nNodes; ++i)
  		{
  		 nodes.Get (i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed_ms, 0.0, 0.0));pos_x += delta_x;
  		}

  	for (uint32_t i = 0; i < nNodes; ++i)
  		{
  		 Ptr<Node> node = nodes.Get (i);
  		 Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
  		 Vector pos = mobility->GetPosition();
  		 Vector spd = mobility->GetVelocity();
  		 NS_LOG_UNCOND("Node "<< i << " Pos X: " << pos.x << " m. Pos Y: " << pos.y <<" m. Speed on X :"<< spd.x <<" m/s" );
  		}
  	
  	// 3. Configure WAVE devices (802.11p)
	NS_LOG_UNCOND("\nConfiguring WAVE devices...");
  	YansWifiChannelHelper waveChannel = YansWifiChannelHelper::Default ();
    waveChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  	YansWavePhyHelper wavePhy = YansWavePhyHelper::Default ();
  	Ptr<YansWifiChannel> channel = waveChannel.Create();
  	wavePhy.SetChannel (waveChannel.Create ());
  	  	
  	// MODIFICATION OF POWER (dBm)
	// value is in dBm. stándard for WAVE (aprox. 100mW):
	wavePhy.Set ("TxPowerStart", DoubleValue (TX_POWER_DBM)); 
	wavePhy.Set ("TxPowerEnd", DoubleValue (TX_POWER_DBM));
	
  	// Use qosWaveMacHelper for OCB configuration (Outside the Context of a BSS) simple
  	QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();
  	Wifi80211pHelper waveHelper = Wifi80211pHelper::Default ();
   	NetDeviceContainer devices = waveHelper.Install (wavePhy, waveMac, nodes);

   	// 4. Install IP stack:
    NS_LOG_UNCOND("\nInstalling IP stack..."); 
    InternetStackHelper internetStack;
    internetStack.Install(nodes);

    Ipv4AddressHelper ipv4;
    NS_LOG_UNCOND("Assign IP Addresses.");
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);
    
    for (uint32_t i = 0; i < nNodes; ++i)
  		{
    	 Ipv4Address nodeIp = interfaces.GetAddress(i);
    	 NS_LOG_UNCOND("Node " << i  << " IP: " << nodeIp);
  		}
  	
	// 5. Configuring sockets Rx function callback

    NS_LOG_UNCOND("\nConfiguring socket callbak:");
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    
    Ptr<Socket> recvSink;
    //InetSocketAddress local;
    for (uint32_t i = 1; i < nNodes; ++i)
  		{
  		 //TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  		 //Ptr<Socket> recvSink;
		 recvSink = Socket::CreateSocket(nodes.Get(i), tid);
    	 recvSink->SetIpRecvTtl(true);    
    	 InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
    	 recvSink->Bind(local);
    	 recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));
  		}


	
	// 6. Configuring sockets Tx function callback

    NS_LOG_UNCOND("Configuring socket TX source:"); 
    
    TypeId tid00 = TypeId::LookupByName("ns3::UdpSocketFactory");   
    Ptr<Socket> source0 = Socket::CreateSocket(nodes.Get(0), tid00);
    InetSocketAddress remote0 = InetSocketAddress(Ipv4Address("255.255.255.255"), 80);
    source0->SetAllowBroadcast(true);
    source0->Connect(remote0);
    

      
	Simulator::ScheduleWithContext(source0->GetNode()->GetId(),
                                   Seconds(interval),
                                   &GenerateTraffic,
                                   source0,
                                   packetSize,
                                   numPackets,
                                   interPacketInterval);
  	
  	
	// 7. Open a csv file to store results:

	csvFile.open ("mobility_nodes.csv");
	csvFile << "Time,Distance_to_Stop,Distance_x,Crash" << std::endl; // File header

  	// 8. Execute Simulation
  	NS_LOG_UNCOND("\n******** Starting simulation... *************");
  	Simulator::Stop (Seconds (TIME_TO_RUN));
  	Simulator::Run ();
  	Simulator::Destroy ();
  	
  	csvFile.close ();
	NS_LOG_UNCOND("\n******** Simulation finished.  **************");
  	return 0;
}
