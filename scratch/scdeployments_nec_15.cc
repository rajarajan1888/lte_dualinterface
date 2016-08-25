#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-propagation-loss-model.h>
#include <ns3/buildings-helper.h>
#include <ns3/radio-environment-map-helper.h>
#include <ns3/internet-module.h>
#include <ns3/buildings-module.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/applications-module.h>
#include <ns3/log.h>
#include <iomanip>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <stdint.h>
#include <algorithm>
#include <utility>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("SMALL-CELL-TESTS");

using std::vector;

static void GetRsrpStats (Ptr<LteHelper> lteHelper , Ipv4InterfaceContainer ueIpIface , NetDeviceContainer ueDevices , NetDeviceContainer enbDevs);
int main (int argc , char *argv[])
{
	Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MicroSeconds(100)));
	Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
	Config::SetDefault ("ns3::LogDistancePropagationLossModel::StandardDeviation", DoubleValue (6.5));
	Config::SetDefault ("ns3::LogDistancePropagationLossModel::Mean", DoubleValue(0.0));
	CommandLine cmd;
	bool generateRem = false;
	cmd.AddValue ("generateRem", "Generate REM(s) only and then stop", generateRem);
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();

	MobilityHelper mobility;

	uint32_t smallEnbs = 13;

	NodeContainer smallCells;
	smallCells.Create(smallEnbs);

	Ptr<ListPositionAllocator> eNBpositionAllocSmall = CreateObject<ListPositionAllocator> ();

	//std::map <uinr32_t , uint32_t> coordinates;

	/*while ( getline (myfilex, line ) )
	{
		double xpos = atof(line.c_str());
		getline ( myfiley , line );
		double ypos = atof(line.c_str());
		std::cout<<" X pos = "<<xpos<<" Y Pos = "<<ypos<<std::endl;
		eNBpositionAllocMacro->Add(Vector(xpos, ypos, 30));
	}*/
	srand (time(NULL));

//	eNBpositionAllocSmall->Add(Vector(500,500,10));
/*
	eNBpositionAllocSmall->Add(Vector(250,500,10));
	eNBpositionAllocSmall->Add(Vector(750,500,10));
	eNBpositionAllocSmall->Add(Vector(500,67,10));
	eNBpositionAllocSmall->Add(Vector(500,933,10));
	eNBpositionAllocSmall->Add(Vector(375,716.5,10));
	eNBpositionAllocSmall->Add(Vector(625,716.5,10));
	eNBpositionAllocSmall->Add(Vector(375,283.5,10));
	eNBpositionAllocSmall->Add(Vector(625,283.5,10));
	eNBpositionAllocSmall->Add(Vector(0,500,10));
	eNBpositionAllocSmall->Add(Vector(1000,500,10));
	eNBpositionAllocSmall->Add(Vector(125,716.5,10));
	eNBpositionAllocSmall->Add(Vector(250,933,10));
	eNBpositionAllocSmall->Add(Vector(750,933,10));
	eNBpositionAllocSmall->Add(Vector(875,716.5,10));
	eNBpositionAllocSmall->Add(Vector(125,283.5,10));
	eNBpositionAllocSmall->Add(Vector(250,67,10));
	eNBpositionAllocSmall->Add(Vector(750,67,10));
	eNBpositionAllocSmall->Add(Vector(875,283.5,10));
*/

/*
	eNBpositionAllocSmall->Add(Vector(2000,500,10));
	eNBpositionAllocSmall->Add(Vector(1750,933,10));
	eNBpositionAllocSmall->Add(Vector(1500,500,10));
	eNBpositionAllocSmall->Add(Vector(2250,933,10));
	eNBpositionAllocSmall->Add(Vector(2500,500,10));
	eNBpositionAllocSmall->Add(Vector(1750,67,10));
	eNBpositionAllocSmall->Add(Vector(2250,67,10));
	eNBpositionAllocSmall->Add(Vector(1750,500,10));
	eNBpositionAllocSmall->Add(Vector(2250,500,10));
	eNBpositionAllocSmall->Add(Vector(2000,716.5,10));
	eNBpositionAllocSmall->Add(Vector(2000,283.5,10));
*/
//	sleep (10);

	/*
	eNBpositionAllocSmall->Add(Vector(1000,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(750,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(1250,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(875,	1216.5,	10));
	eNBpositionAllocSmall->Add(Vector(1125,	1216.5,	10));
	eNBpositionAllocSmall->Add(Vector(875,	783.5,	10));
	eNBpositionAllocSmall->Add(Vector(1125,	783.5,	10));
	eNBpositionAllocSmall->Add(Vector(500,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(1500,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(625,	1216.5,	10));
	eNBpositionAllocSmall->Add(Vector(1375,	1216.5,	10));
	eNBpositionAllocSmall->Add(Vector(625,	783.5,	10));
	eNBpositionAllocSmall->Add(Vector(1375,	783.5,	10));
	eNBpositionAllocSmall->Add(Vector(750,	1433,	10));
	eNBpositionAllocSmall->Add(Vector(1000,	1433,	10));
	eNBpositionAllocSmall->Add(Vector(1250,	1433,	10));
	eNBpositionAllocSmall->Add(Vector(750,	567,	10));
	eNBpositionAllocSmall->Add(Vector(1000,	567,	10));
	eNBpositionAllocSmall->Add(Vector(1250,	567,	10));
	eNBpositionAllocSmall->Add(Vector(875,	350.5,	10));
	eNBpositionAllocSmall->Add(Vector(1125,	350.5,	10));
	eNBpositionAllocSmall->Add(Vector(875,	1649.5,	10));
	eNBpositionAllocSmall->Add(Vector(1125,	1649.5,	10));
*/


	eNBpositionAllocSmall->Add(Vector(1000,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(750,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(1250,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(1000,	1250,	10));
	eNBpositionAllocSmall->Add(Vector(1000,	750,	10));
	eNBpositionAllocSmall->Add(Vector(500,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(1500,	1000,	10));
	eNBpositionAllocSmall->Add(Vector(1000,	500,	10));
	eNBpositionAllocSmall->Add(Vector(1000,	1500,	10));
	eNBpositionAllocSmall->Add(Vector(750,	1250,	10));
	eNBpositionAllocSmall->Add(Vector(1250,	1250,	10));
	eNBpositionAllocSmall->Add(Vector(750,	750,	10));
	eNBpositionAllocSmall->Add(Vector(1250,	750,	10));




/*		eNBpositionAllocSmall->Add(Vector(1000,	1000,	10));
		eNBpositionAllocSmall->Add(Vector(750,	1000,	10));
		eNBpositionAllocSmall->Add(Vector(1250,	1000,	10));
		eNBpositionAllocSmall->Add(Vector(875,	1216.5,	10));
		eNBpositionAllocSmall->Add(Vector(1125,	1216.5,	10));
		eNBpositionAllocSmall->Add(Vector(875,	783.5,	10));
		eNBpositionAllocSmall->Add(Vector(1125,	783.5,	10));
		eNBpositionAllocSmall->Add(Vector(500,	1000,	10));
		eNBpositionAllocSmall->Add(Vector(1500,	1000,	10));
		eNBpositionAllocSmall->Add(Vector(625,	1216.5,	10));
		eNBpositionAllocSmall->Add(Vector(1375,	1216.5,	10));
		eNBpositionAllocSmall->Add(Vector(625,	783.5,	10));
		eNBpositionAllocSmall->Add(Vector(1375,	783.5,	10));
		eNBpositionAllocSmall->Add(Vector(750,	1433,	10));
		eNBpositionAllocSmall->Add(Vector(1000,	1433,	10));
		eNBpositionAllocSmall->Add(Vector(1250,	1433,	10));
		eNBpositionAllocSmall->Add(Vector(750,	567,	10));
		eNBpositionAllocSmall->Add(Vector(1000,	567,	10));
		eNBpositionAllocSmall->Add(Vector(1250,	567,	10));
		eNBpositionAllocSmall->Add(Vector(250,	1000,	10));
		eNBpositionAllocSmall->Add(Vector(1750,	1000,	10));
		eNBpositionAllocSmall->Add(Vector(375,	1216.5,	10));
		eNBpositionAllocSmall->Add(Vector(1625,	1216.5,	10));
		eNBpositionAllocSmall->Add(Vector(375,	783.5,	10));
		eNBpositionAllocSmall->Add(Vector(1625,	783.5,	10));
		eNBpositionAllocSmall->Add(Vector(500,	1433,	10));
		eNBpositionAllocSmall->Add(Vector(1500,	1433,	10));
		eNBpositionAllocSmall->Add(Vector(500,	567,	10));
		eNBpositionAllocSmall->Add(Vector(1500,	567,	10));
		eNBpositionAllocSmall->Add(Vector(625,	350.5,	10));
		eNBpositionAllocSmall->Add(Vector(875,	350.5,	10));
		eNBpositionAllocSmall->Add(Vector(1125,	350.5,	10));
		eNBpositionAllocSmall->Add(Vector(1375,	350.5,	10));
		eNBpositionAllocSmall->Add(Vector(625,	1649.5,	10));
		eNBpositionAllocSmall->Add(Vector(875,	1649.5,	10));
		eNBpositionAllocSmall->Add(Vector(1125,	1649.5,	10));
		eNBpositionAllocSmall->Add(Vector(1375,	1649.5,	10));
		eNBpositionAllocSmall->Add(Vector(750,	134,	10));
		eNBpositionAllocSmall->Add(Vector(1000,	134,	10));
		eNBpositionAllocSmall->Add(Vector(1250,	134,	10));
		eNBpositionAllocSmall->Add(Vector(750,	1866,	10));
		eNBpositionAllocSmall->Add(Vector(1000,	1866,	10));
		eNBpositionAllocSmall->Add(Vector(1250,	1866,	10));
*/

	/*for( uint8_t i = 1 ; i <= 3 ; i++ )
	{
		eNBpositionAllocMacro->Add(Vector(1700 , 5888.8 , 30));
		eNBpositionAllocMacro->Add(Vector(3400 , 5888.8 , 30));
		eNBpositionAllocMacro->Add(Vector(5100 , 5888.8 , 30));
		eNBpositionAllocMacro->Add(Vector( 850, 4416.6, 30));
		eNBpositionAllocMacro->Add(Vector(2550, 4416.6, 30));
		eNBpositionAllocMacro->Add(Vector(4250, 4416.6, 30));
		eNBpositionAllocMacro->Add(Vector(5950 , 4416.6, 30));
		eNBpositionAllocMacro->Add(Vector(0, 2944.4, 30));
		eNBpositionAllocMacro->Add(Vector(1700, 2944.4, 30));
		eNBpositionAllocMacro->Add(Vector(3400, 2944.4, 30));
		eNBpositionAllocMacro->Add(Vector(5100, 2944.4, 30));
		eNBpositionAllocMacro->Add(Vector(6800, 2944.4, 30));
		eNBpositionAllocMacro->Add(Vector(850, 1472.2, 30));
		eNBpositionAllocMacro->Add(Vector(2550, 1472.2, 30));
		eNBpositionAllocMacro->Add(Vector(4250, 1472.2, 30));
		eNBpositionAllocMacro->Add(Vector(5950, 1472.2, 30));
		eNBpositionAllocMacro->Add(Vector(1700 ,0  , 30));
		eNBpositionAllocMacro->Add(Vector(3400 , 0 , 30));
		eNBpositionAllocMacro->Add(Vector(5100 ,0  , 30));

	}*/

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator (eNBpositionAllocSmall);
	mobility.SetFreq (748);
	mobility.Install(smallCells);

	NodeContainer ueNodes;

	uint32_t nUEs = 500;
	double ueHeight = 10;
	ueNodes.Create (nUEs);

	Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
	MobilityHelper uemobility;
	uemobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

	std::filebuf fb;
	fb.open("//home//rajarajan//Documents//lena//NEC//files//getuepos_stadium.txt",std::ios::out);
	std::ostream ospos(&fb);
	srand (time(NULL));
	//nUEs = macroEnbs;
	for ( uint32_t i = 1 ; i <= nUEs ; i++ )
	{
		label1:
/*		double xcoord = CreateObject<UniformRandomVariable>()->GetValue (0, 1000);
		double ycoord = CreateObject<UniformRandomVariable>()->GetValue (67, 933);

		if ( sqrt(((xcoord-500)*(xcoord-500))+((ycoord-500)*(ycoord-500))) > 500 || sqrt(((xcoord-500)*(xcoord-500))+((ycoord-500)*(ycoord-500))) < 250 )
			goto label1;*/

		double xcoord = CreateObject<UniformRandomVariable>()->GetValue (500, 1500);
		double ycoord = CreateObject<UniformRandomVariable>()->GetValue (567, 1433);

				if ( sqrt(((xcoord-1000)*(xcoord-1000))+((ycoord-1000)*(ycoord-1000))) > 500 )
					goto label1;


		std::cout<<xcoord<<","<<ycoord<<std::endl;

		Vector userv(xcoord , ycoord , ueHeight);
		uePositionAlloc->Add(userv);
	}

		sleep (20);

	uemobility.SetPositionAllocator(uePositionAlloc);
	uemobility.Install(ueNodes);

	Ptr < LteHelper > lteHelper = CreateObject<LteHelper> ( nUEs , smallEnbs );
	Ptr<EpcHelper>  epcHelper = CreateObject<EpcHelper> ();
	lteHelper->SetEpcHelper (epcHelper);
	lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::LogDistancePropagationLossModel"));

	std::string outputpath = "//home//rajarajan//Documents//att_workspace//ltea//hetnet";
	Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlRlcOutputFilename", StringValue (outputpath+"/DlRlcStatsatt.txt"));
	Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlRlcOutputFilename", StringValue (outputpath+"/UlRlcStatsatt.txt"));
	Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlPdcpOutputFilename", StringValue (outputpath+"/DlPdcpStatsatt.txt"));
	Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlPdcpOutputFilename", StringValue (outputpath+"/UlPdcpStatsatt.txt"));
	Config::SetDefault ("ns3::MacStatsCalculator::DlOutputFilename", StringValue (outputpath+"/DlMacStatsatt.txt"));
	Config::SetDefault ("ns3::MacStatsCalculator::UlOutputFilename", StringValue (outputpath+"/UlMacStatsatt.txt"));
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (35.00));
// sinrPxd.txt
    lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (200));
	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (50));
	lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
//shadowing
	NetDeviceContainer smallDevs = lteHelper->InstallEnbDevice (smallCells, 5180, 5730);

	/*for(uint8_t j = 0 ; j < 3 ; j++ )	//m_shadowing
	  {
		  for(uint8_t i = 0 ; i < 7 ; i++ )
		  {
	  // Beam width is made quite narrow so sectors can be noticed in the REM
			  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
			  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (j*120));
			  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (70));
			  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (15.0));
			  macroDevs.Add ( lteHelper->InstallEnbDevice (macroCells.Get ((j*7)+i)));
		  }
	  }*/

	NetDeviceContainer ueDevices = lteHelper->InstallUeDevice (ueNodes);

    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    InternetStackHelper internet;
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create (1);
    Ptr<Node> remoteHost ;
    remoteHost = remoteHostContainer.Get (0);
    internet.Install (remoteHostContainer);
    internet.Install(ueNodes);
    for(NodeContainer::Iterator it = ueNodes.Begin();it!=ueNodes.End();it++)
    {
    	 Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
     	 ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    Ipv4InterfaceContainer ueIpIface;
	ueIpIface =  epcHelper->AssignUeIpv4Address(ueDevices);

	for (uint32_t i = 0 ; i < ueNodes.GetN() ; i++ )
	  lteHelper->AddDeviceToChannel(ueDevices.Get(i));

	lteHelper->AddeNBToChannel (eNBpositionAllocSmall , nUEs);

    lteHelper->EnableTraces ();
    lteHelper->EnablePhyTraces ();
    lteHelper->EnableMacTraces ();
    lteHelper->EnableRlcTraces ();

    Simulator::Stop (MilliSeconds (100));
   // Simulator::Schedule (MilliSeconds (2) , &GetStats , lteHelper , ueIpIface , ueDevices , enbDevs);
    Simulator::Schedule (MilliSeconds (2) , &GetRsrpStats , lteHelper , ueIpIface , ueDevices , smallDevs);
    Simulator::Run ();

    lteHelper = 0;
    Simulator::Destroy ();
    return 0;
}

static void GetRsrpStats (Ptr<LteHelper> lteHelper , Ipv4InterfaceContainer ueIpIface , NetDeviceContainer ueDevices , NetDeviceContainer enbDevs)
{
	std::vector<std::vector<double> > localRsrp;

	std::vector<uint32_t> enbChoices;
	uint16_t dlPort = 1234;
	PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;
	//double csb = 6.9897;

	localRsrp = lteHelper->getRsrpAllVector();
	//enbDevs.at(1).Get(0)->GetObject<LteEnbNetDevice>()->GetScheduler()->SetBlankIndicator(1);
	//enbDevs.at(1).Get(0)->GetObject<LteEnbNetDevice>()->GetPhy()->setBlankIndicator(1);

	std::filebuf fb;
	fb.open("//home//rajarajan//Documents//lena//NEC//files//getrsrpvals_smallcell_nec_15.txt",std::ios::out);
	std::ostream osrsrp(&fb);

	std::filebuf fb1;
	fb1.open("//home//rajarajan//Documents//lena//NEC//files//getassociation_smallcell_nec_15.txt",std::ios::out);
	std::ostream osass(&fb1);

	srand(time(NULL));
	for(uint32_t i = 0 ; i < localRsrp.size() ; i++)
	{
		uint16_t enbChoice = 0;
		double rsrpmax = 0.0;
		for ( uint32_t j = 0 ; j < localRsrp.at(i).size() ; j++ )
		{
			osrsrp<<localRsrp.at(i).at(j)<<"\n";
			if ( localRsrp.at(i).at(j) > rsrpmax )
			{
				rsrpmax = localRsrp.at(i).at(j);
				enbChoice = j;
			}
		}
		std::cout<<" UE : "<<(i+1)<<" eNB : "<<(enbChoice+1)<<std::endl;
		osass<<enbChoice+1<<"\n";
		lteHelper->Attach (ueDevices.Get (i) , enbDevs.Get (enbChoice) );
	}
	//sleep (10);

	/*
	serverApps.Start (MilliSeconds(4));
	clientApps.Start (MilliSeconds(4));
	*/
//	Simulator::Schedule (MilliSeconds (2) , &GetTbsStats, lteHelper, smallUEs , creUEs, macroUEs, enbDevs);
//	Simulator::Schedule (MilliSeconds (6) , &GetSmallStats, lteHelper, smallUEs);
}
