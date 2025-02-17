/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include <ns3/lte-ue-net-device.h>
#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/lte-helper.h"

using namespace ns3;
using namespace mmwave;

NS_LOG_COMPONENT_DEFINE ("ScenarioTwo");

int main (int argc, char *argv[])
{
    // Enable logging
    LogComponentEnable ("ScenarioTwo", LOG_LEVEL_INFO);

    // Simulation parameters
    double simTime = 5.0;
    double bandwidth = 20e6;
    double centerFrequency = 3.5e9;
    double isd = 20000;  // Inter-site distance for 100x100km area
    uint8_t nMmWaveEnbNodes = 10;
    uint8_t nLteEnbNodes = 1;
    uint32_t ueCount = 50;
    double maxXAxis = 100000;
    double maxYAxis = 100000;

    // Create helpers
    Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
    mmwaveHelper->SetPathlossModelType ("ns3::ThreeGppUmiStreetCanyonPropagationLossModel");
    mmwaveHelper->SetChannelConditionModelType ("ns3::ThreeGppUmiStreetCanyonChannelConditionModel");

    Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
    mmwaveHelper->SetEpcHelper (epcHelper);

    // Create nodes
    NodeContainer ueNodes;
    NodeContainer mmWaveEnbNodes;
    NodeContainer lteEnbNodes;
    mmWaveEnbNodes.Create (nMmWaveEnbNodes);
    lteEnbNodes.Create (nLteEnbNodes);
    ueNodes.Create (ueCount);

    // Configure mobility
    Vector centerPosition = Vector (maxXAxis/2, maxYAxis/2, 3);
    MobilityHelper enbMobility;
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
    
    // Position base stations in a grid
    enbPositionAlloc->Add (centerPosition);  // LTE eNB at center
    for (uint8_t i = 0; i < nMmWaveEnbNodes; ++i)
    {
        double x = 20000 + (i % 3) * 30000;
        double y = 20000 + (i / 3) * 30000;
        enbPositionAlloc->Add (Vector(x, y, 3));
    }

    enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator (enbPositionAlloc);
    enbMobility.Install (mmWaveEnbNodes);
    enbMobility.Install (lteEnbNodes);

    // Configure UE mobility with random positions
    MobilityHelper ueMobility;
    Ptr<RandomBoxPositionAllocator> uePositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
    uePositionAlloc->SetAttribute ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=100000.0]"));
    uePositionAlloc->SetAttribute ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=100000.0]"));
    uePositionAlloc->SetAttribute ("Z", StringValue ("ns3::UniformRandomVariable[Min=1.5|Max=1.5]"));
    
    ueMobility.SetMobilityModel ("ns3::RandomWalk2dOutdoorMobilityModel",
                                "Speed", StringValue ("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),
                                "Bounds", RectangleValue (Rectangle (0, maxXAxis, 0, maxYAxis)));
    ueMobility.SetPositionAllocator (uePositionAlloc);
    ueMobility.Install (ueNodes);

    // Install devices
    NetDeviceContainer enbDevs = mmwaveHelper->InstallEnbDevice (mmWaveEnbNodes);
    NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
    NetDeviceContainer ueDevs = mmwaveHelper->InstallUeDevice (ueNodes);

    // Install the IP stack
    InternetStackHelper internet;
    internet.Install (ueNodes);
    
    // Assign IP addresses
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

    // Random attachment of UEs to base stations
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        // Random base station selection
        uint32_t randBs = rand() % nMmWaveEnbNodes;
        mmwaveHelper->AttachToClosestEnb (ueDevs.Get(u), enbDevs.Get(randBs), lteEnbDevs.Get(0));
    }

    // Start simulation
    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}



