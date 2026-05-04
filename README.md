# EE-6750-Wireless-Mobile-Networking-GROUP8
DSRC Wave basic safety message relay study. 

Reliability and latency effects Vs vehicular network density and mobility of safety messages simulated using ns-3 network simulator.

This CPP program uses the WAVE libraries at the ns3 network simulator environment for create n nodes that will relay Basic Safety Messages (BSM), with a tag added for Time-To-Live (TTL) for avoid the message to be re-bradcasted forever. 

This is a CPP code for be run in ns3 simulator from the scratch directory:

$./ns3 run scratch/DSRC_WAVE_TTL_Multihop

This will create a csv file "mobility_nodes.csv" with the distance and the time when each Basic Safety Message is received at the end of the chain of simulated nodes.

If the very long stream of messages displayed at the terminal need to be sent to a log file for further analysis, the following can be used:

$$./ns3 run scratch/DSRC_WAVE_TTL_Multihop > log.out > 2>&1

The parameters can be modified by changing the following section at the code:

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

From there, all the cases can be created.

A zip file with the cases created fro the final project is included and the excell files for make the plots

Enjoy it!!!!

Any question, contact me:

Aaron Castro Zazueta
aczzx2ss@live.com.mx
