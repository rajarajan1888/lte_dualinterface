/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jnin@cttc.es>
 */

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
//#include "ns3/gtk-config-store.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("SMALL-CELL-TESTS");

using std::vector;

void PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str ());
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}

void PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str ());
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,15\" textcolor rgb \"white\" front point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

void PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str ());
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,15\" textcolor rgb \"grey\" front  point pt 2 ps 0.3 lc rgb \"grey\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

//static void ConfigureABSsetup (vector<NetDeviceContainer> ueDevs , LteHelper lteHelper);

Ptr<Node> remoteHost ;

//static void GetStats (Ptr<LteHelper> lteHelper , std::vector < NodeContainer > ueNodes , std::vector<NetDeviceContainer> ueDevs , std::multimap<NetDeviceContainer , Ptr<NetDevice> > m_uedeviceenbmap, NetDeviceContainer enbDevs);
static void GetStats (Ptr<LteHelper> lteHelper , std::vector<Ipv4InterfaceContainer> ueIpIface , std::vector<NodeContainer> ueNodes , std::vector<NetDeviceContainer> ueDevs , NetDeviceContainer enbDevs);
int
main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MicroSeconds(100)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue(1000000));

  //std::multimap<NetDeviceContainer , Ptr<NetDevice> > m_uedeviceenbmap;

  bool generateRem = false;
  bool attachue1tometro = true;
 // bool attachue1tometro = false;
  // Geometry of the scenario (in meters)
  double nodeHeight = 1.5;
  double roomHeight = 5;
  double roomLength = 20;
  uint32_t nRooms = 1;
  uint32_t macroXcoord = 500;
  uint32_t macroYcoord = 2000;
  // Create one eNodeB per room + one 3 sector eNodeB (i.e. 3 eNodeB) + one regular eNodeB
  uint32_t nEnb = nRooms*nRooms + 4;
  uint32_t nUe = 1;

  CommandLine cmd;
  cmd.AddValue ("generateRem", "Generate REM(s) only and then stop", generateRem);
  cmd.AddValue ("macroXcoord", "X coordinate for macro eNB", macroXcoord);
  cmd.AddValue ("macroYcoord", "Y coordinate for macro eNB", macroYcoord);
  cmd.Parse (argc, argv);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  cmd.Parse (argc, argv);

  Ptr < LteHelper > lteHelper = CreateObject<LteHelper> (nEnb,nEnb);
  Ptr<EpcHelper>  epcHelper = CreateObject<EpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  //lteHelper->EnableLogComponents ();
  //lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::LogDistancePropagationLossModel"));
  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer oneSectorNodes;
  NodeContainer threeSectorNodes;
  std::vector < NodeContainer > ueNodes;


  oneSectorNodes.Create (nEnb-3);
  threeSectorNodes.Create (3);

  for(NodeContainer::Iterator it=oneSectorNodes.Begin() ; it!=oneSectorNodes.End() ; it++)
	  (*it)->AddCellBias(7);

  enbNodes.Add (oneSectorNodes);
  enbNodes.Add (threeSectorNodes);



  for (uint32_t i = 0; i < nEnb; i++)
    {
      NodeContainer ueNode;
      ueNode.Create (nUe);
      ueNodes.push_back (ueNode);
    }

  MobilityHelper mobility;
  vector<Vector> enbPosition;
  Ptr < ListPositionAllocator > positionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr < Building > building;
  building = Create<Building> ();
  building->SetBoundaries (Box (0.0, nRooms * roomLength,
                                0.0, nRooms * roomLength,
                                0.0, roomHeight));
  building->SetBuildingType (Building::Residential);
  building->SetExtWallsType (Building::ConcreteWithWindows);
  building->SetNFloors (1);
  building->SetNRoomsX (nRooms);
  building->SetNRoomsY (nRooms);
  mobility.SetMobilityModel ("ns3::BuildingsMobilityModel");
  mobility.Install (enbNodes);
  uint32_t plantedEnb = 0;
  for (uint32_t row = 0; row < nRooms; row++)
    {
      for (uint32_t column = 0; column < nRooms; column++, plantedEnb++)
        {
    	  std::cout<<"eNB["<<row<<"]["<<column<<"], X = "<<(roomLength*(column+0.5))<<" Y="<<(roomLength*(row+0.5))<<" Node height = "<<nodeHeight<<std::endl;
          /*Vector v (roomLength * (column + 0.5),
                    roomLength * (row + 0.5),
                    nodeHeight );*/
    	  Vector v (500 * (column + 0.75),
    	                      2000 * (row + 0.85),
    	                      nodeHeight );
          positionAlloc->Add (v);
          enbPosition.push_back (v);
          Ptr<BuildingsMobilityModel> mmEnb = enbNodes.Get (plantedEnb)->GetObject<BuildingsMobilityModel> ();
          mmEnb->SetPosition (v);
        }
    }

  // Add a 1-sector site
  Vector v (700, 2350, nodeHeight);
  std::cout<<"eNB One Sector, X = 700, Y = 2350, Node height = "<<nodeHeight<<std::endl;
  positionAlloc->Add (v);
  enbPosition.push_back (v);
  mobility.Install (ueNodes.at(plantedEnb));
  plantedEnb++;

  // Add the 3-sector site
  for (uint32_t index = 0; index < 3; index++, plantedEnb++)
    {
      Vector v (macroXcoord, macroYcoord, nodeHeight);
      std::cout<<"eNB 3 Sector, X = "<<macroXcoord<<", Y = "<<macroYcoord<<", Node height = "<<nodeHeight<<std::endl;
      positionAlloc->Add (v);
      enbPosition.push_back (v);
      mobility.Install (ueNodes.at(plantedEnb));
    }


  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (enbNodes);

  // Position of UEs attached to eNB
  for (uint32_t i = 0; i < nEnb; i++)
    {
      Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable> ();
      posX->SetAttribute ("Min", DoubleValue (enbPosition.at(i).x - roomLength * 0));
      posX->SetAttribute ("Max", DoubleValue (enbPosition.at(i).x + roomLength * 0));
      Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable> ();
      posY->SetAttribute ("Min", DoubleValue (enbPosition.at(i).y - roomLength * 0));
      posY->SetAttribute ("Max", DoubleValue (enbPosition.at(i).y + roomLength * 0));
      positionAlloc = CreateObject<ListPositionAllocator> ();
      for (uint32_t j = 0; j < nUe; j++)
        {
          if ( i == nEnb - 3 )
            {
              //positionAlloc->Add (Vector (enbPosition.at(i).x + 400, enbPosition.at(i).y, nodeHeight));
        	  positionAlloc->Add (Vector (enbPosition.at(i).x + 250, enbPosition.at(i).y + 600, nodeHeight));
              std::cout<<"UE "<<i<<" X = "<<enbPosition.at(i).x + 250<<", Y = "<<enbPosition.at(i).y + 600<<", Node height = "<<nodeHeight<<std::endl;
            }
          else if ( i == nEnb - 2 )
            {
        	  positionAlloc->Add (Vector (enbPosition.at(i).x - 400, enbPosition.at(i).y + 200, nodeHeight));
        	  std::cout<<"UE "<<i<<" X = "<<enbPosition.at(i).x - 400<<", Y = "<<enbPosition.at(i).y + 200<<", Node height = "<<nodeHeight<<std::endl;

            }
          else if ( i == nEnb - 1 )
            {
        	  //positionAlloc->Add (Vector (enbPosition.at(i).x - 500, enbPosition.at(i).y - 1300, nodeHeight));
        	  positionAlloc->Add (Vector (enbPosition.at(i).x - 400, enbPosition.at(i).y, nodeHeight));
        	  std::cout<<"UE "<<i<<" X = "<<enbPosition.at(i).x - 400<<", Y = "<<enbPosition.at(i).y<<", Node height = "<<nodeHeight<<std::endl;

            }
          else
            {
        	  std::cout<<"UE "<<i<<" X = "<<posX->GetValue()<<", Y = "<<posY->GetValue()<<", Node height = "<<nodeHeight<<std::endl;
              positionAlloc->Add (Vector (posX->GetValue (), posY->GetValue (), nodeHeight));
            }
          mobility.SetPositionAllocator (positionAlloc);
        }
      mobility.Install (ueNodes.at(i));
    }

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  InternetStackHelper internet;

   NodeContainer remoteHostContainer;
   remoteHostContainer.Create (1);
   remoteHost = remoteHostContainer.Get (0);
   internet.Install (remoteHostContainer);

   PointToPointHelper p2ph;
     p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
     p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
     p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
     NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
     Ipv4AddressHelper ipv4h;
     ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
     Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
     // interface 0 is localhost, 1 is the p2p device
     Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

     Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
     remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  std::vector<Ipv4InterfaceContainer> ueIpIface;
  for(unsigned int nodectr = 0 ; nodectr < ueNodes.size() ; nodectr++)
  {
	  internet.Install(ueNodes.at(nodectr));
	  for(NodeContainer::Iterator it = ueNodes.at(nodectr).Begin();it!=ueNodes.at(nodectr).End();it++)
	  {
		  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
	      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	  }
  }



  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;

  std::vector < NetDeviceContainer > ueDevs;

  // power setting in dBm for small cells
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (30.0));
  enbDevs = lteHelper->InstallEnbDevice (oneSectorNodes);

  std::string outputpath = "/home/rajarajan/Documents/att_workspace/ltea/hetnet";

  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlRlcOutputFilename", StringValue (outputpath+"/DlRlcStatsatt.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlRlcOutputFilename", StringValue (outputpath+"/UlRlcStatsatt.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlPdcpOutputFilename", StringValue (outputpath+"/DlPdcpStatsatt.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlPdcpOutputFilename", StringValue (outputpath+"/UlPdcpStatsatt.txt"));
  Config::SetDefault ("ns3::MacStatsCalculator::DlOutputFilename", StringValue (outputpath+"/DlMacStatsatt.txt"));
  Config::SetDefault ("ns3::MacStatsCalculator::UlOutputFilename", StringValue (outputpath+"/UlMacStatsatt.txt"));

  // power setting for three-sector macrocell
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (47.78));

  // Beam width is made quite narrow so sectors can be noticed in the REM
  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (0));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (50));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add ( lteHelper->InstallEnbDevice (threeSectorNodes.Get (0)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (50));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add ( lteHelper->InstallEnbDevice (threeSectorNodes.Get (1)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (2*360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (50));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add ( lteHelper->InstallEnbDevice (threeSectorNodes.Get (2)));

  lteHelper->SetEpcHelper (epcHelper);
  std::cout<<"EPC HELPER SET"<<std::endl;

  if (attachue1tometro == false) {
	std::cout<<"Not Attaching UE to Metro"<<std::endl;
  	NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(0));
  	ueDevs.push_back (ueDev); //edw
  	lteHelper->Attach (ueDev, enbDevs.Get (4));
  	enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  	EpsBearer bearer (q);
  	lteHelper->ActivateDataRadioBearer (ueDev, bearer);

  	for (uint32_t i = 1; i < nEnb; i++)
    {
      NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(i));
      ueDevs.push_back (ueDev);
      lteHelper->Attach (ueDev, enbDevs.Get (i));
      enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
      EpsBearer bearer (q);
      lteHelper->ActivateDataRadioBearer (ueDev, bearer);
    }
  }
  else {
	  std::cout<<"Attaching UE to Metro"<<std::endl;
  	for (uint32_t i = 0; i < nEnb; i++)
    {
  		if(i==2)
  		{
  			std::cout<<"Adding multiple network interfaces"<<std::endl;
  			NetDeviceContainer ueDevices = lteHelper->InstallUeDevices(ueNodes.at(i),2);
  			ueDevs.push_back(ueDevices);
  			ueIpIface.push_back(epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevices)));
  			lteHelper->AttachCA(ueDevices.Get(0) , enbDevs.Get(1));
  			std::cout<<"ENB PHY DEVICE for UE "<<i<<" ADDRESS : "<<enbDevs.Get(2)<<"UE DEVICE: "<<ueDevices.Get(0)<<std::endl;
  			lteHelper->AttachCA(ueDevices.Get(1) , enbDevs.Get(2));
  			std::cout<<"ENB PHY DEVICE for UE "<<i<<" ADDRESS : "<<enbDevs.Get(1)<<"UE DEVICE: "<<ueDevices.Get(1)<<std::endl;

  		}

  		else if(i==4)
  		{
  			std::cout<<"Adding multiple network interfaces "<<std::endl;
  			NetDeviceContainer ueDevices = lteHelper->InstallUeDevices(ueNodes.at(i),2);
  			ueDevs.push_back(ueDevices);
  			ueIpIface.push_back(epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevices)));
  			lteHelper->AttachCA(ueDevices.Get(0),enbDevs.Get(0));
  			std::cout<<"ENB PHY DEVICE for UE "<<i<<" ADDRESS : "<<enbDevs.Get(4)<<"UE DEVICE: "<<ueDevices.Get(0)<<std::endl;
  			lteHelper->AttachCA(ueDevices.Get(1),enbDevs.Get(4));

  			std::cout<<"ENB PHY DEVICE for UE "<<i<<" ADDRESS : "<<enbDevs.Get(0)<<"UE DEVICE: "<<ueDevices.Get(1)<<std::endl;

  		}


  		else
  		{
  			NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(i));
  			ueIpIface.push_back(epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDev)));
  			ueDevs.push_back (ueDev);
  			lteHelper->Attach (ueDev, enbDevs.Get (i));

  			std::cout<<"ENB PHY DEVICE for UE "<<i<<" ADDRESS : "<<enbDevs.Get(i)<<"UE DEVICE: "<<ueDev.Get(0)<<std::endl;

  		}
  	  /*if(i!=2&&i!=4)
  	  {

  		  NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(i));
  		  ueDevs.push_back (ueDev);
  		  lteHelper->Attach (ueDev, enbDevs.Get (i));
  	  }
  	  else if(i==2)
      {
  		std::cout<<"Adding multiple network interfaces "<<std::endl;
    	  NetDeviceContainer ueDevices = lteHelper->InstallUeDevices(ueNodes.at(i),2);
    	  ueDevs.push_back(ueDevices);
    	  lteHelper->Attach(ueDevices.Get(0),enbDevs.Get(2));
    	  lteHelper->Attach(ueDevices.Get(1),enbDevs.Get(1));
      }

  	  else if(i==4)
  	  { DEBUGGING : RNTI
  		std::cout<<"Adding multiple network interfaces "<<std::endl;
  		NetDeviceContainer ueDevices = lteHelper->InstallUeDevices(ueNodes.at(i),2);
  		ueDevs.push_back(ueDevices);
  		lteHelper->Attach(ueDevices.Get(0),enbDevs.Get(4));
  		lteHelper->Attach(ueDevices.Get(1),enbDevs.Get(0));
  	  }*/
     /* enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
      EpsBearer bearer (q);
      lteHelper->ActivateDataRadioBearer (ueDev, bearer);*/

  	/*	NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(i));
  		ueDevs.push_back (ueDev);
  		lteHelper->Attach (ueDev, enbDevs.Get (i)); */
    }

/*  	NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(2));
  	ueDevs.push_back(ueDev);
  	lteHelper->Attach(ueDev , enbDevs.Get(1));
  	ueDev = lteHelper->InstallUeDevice (ueNodes.at(4));
  	ueDevs.push_back(ueDev);
  	lteHelper->Attach(ueDev , enbDevs.Get(0)); */

  	/*ueDev = ueDevs.at(1);
  	lteHelper->Attach(ueDev , enbDevs.Get(4));*/
  }

  std::cout<<"Let's check the INDIVIDUAL RSRP VECTORS for every eNB (indexed by subscripts)"<<std::endl;
/* FIRST SIGNAL for(uint32_t i=0 ; i < nEnb ; i++) CURRENT SUB FRAME
  {
	  for(uint32_t j = 0 ; j < lteHelper->receiveRsrp(i).size() ; j++)
		  std::cout<<"RSRP["<<i+1<<"]["<<j+1<<"]="<<lteHelper->receiveRsrp(i).at(j)<<std::endl;
  }*/

  BuildingsHelper::MakeMobilityModelConsistent ();

  // by default, simulation will anyway stop right after the REM has been generated
  Simulator::Stop (Seconds (50.0));

  Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
  if (generateRem) {
	  std::cout<<"NOT GENERATING TRACES ? "<<std::endl;
  	PrintGnuplottableBuildingListToFile ("buildings.txt");
  	PrintGnuplottableEnbListToFile ("enbs.txt");
  	PrintGnuplottableUeListToFile ("ues.txt");
  	remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
  	remHelper->SetAttribute ("OutputFile", StringValue ("rem.out"));
  	remHelper->SetAttribute ("XMin", DoubleValue (-2000.0));
  	remHelper->SetAttribute ("XMax", DoubleValue (+2000.0));
  	remHelper->SetAttribute ("YMin", DoubleValue (-500.0));
  	remHelper->SetAttribute ("YMax", DoubleValue (+3500.0));
  	remHelper->SetAttribute ("Z", DoubleValue (1.5));
  	remHelper->Install ();
  	lteHelper->EnablePhyTraces ();
  	lteHelper->EnableMacTraces ();
  	lteHelper->EnableRlcTraces ();
  }
  else {
	  std::cout<<"GENERATING TRACES ? "<<std::endl;
	  lteHelper->EnableTraces ();
		lteHelper->EnablePhyTraces ();
		lteHelper->EnableMacTraces ();
		lteHelper->EnableRlcTraces ();
  }
  //Simulator::Schedule (MilliSeconds (2) , &GetStats , lteHelper , ueNodes, ueDevs, m_uedeviceenbmap, enbDevs); RLC

  Simulator::Schedule (MilliSeconds (2) , &GetStats , lteHelper , ueIpIface , ueNodes , ueDevs, enbDevs);
  Simulator::Run ();

 // Simulator::Schedule (MilliSeconds (50.0), &ConfigureABSsetup, ueDevs , lteHelper);
  lteHelper = 0;
  Simulator::Destroy ();
  return 0;
}

//static void ConfigureABS ( uint32_t currentTime, Ptr<LteHelper> lteHelper , std::vector<NetDeviceContainer> creUEdevs , std::vector<NetDeviceContainer> noncreUEdevs , std::vector<NetDeviceContainer> macroUEdevs , std::multimap<NetDeviceContainer, Ptr<NetDevice> > m_uedeviceenbmap , NetDeviceContainer enbDevs);
static void ConfigureABS ( Ptr<LteHelper> lteHelper , std::vector<NetDeviceContainer> creUEdevs , std::vector<NetDeviceContainer> noncreUEdevs , std::vector<NetDeviceContainer> macroUEdevs , NetDeviceContainer enbDevs);

//static void GetStats (Ptr<LteHelper> lteHelper , std::vector<NetDeviceContainer> ueDevs , std::multimap<NetDeviceContainer , Ptr<NetDevice> > m_uedeviceenbmap, NetDeviceContainer enbDevs)
static void GetStats (Ptr<LteHelper> lteHelper , std::vector<Ipv4InterfaceContainer> ueIpIface , std::vector<NodeContainer> ueNodes , std::vector<NetDeviceContainer> ueDevs , NetDeviceContainer enbDevs)
{

	 std::vector<NetDeviceContainer> creUEdevs , noncreUEdevs, macroUEdevs;
	 int counter;
	 static int tracker = 0;
	 static std::filebuf fb;
	 if(tracker == 0)
		 fb.open("//home//rajarajan//Documents//att_workspace//ltea//test.txt",std::ios::out);
	 static std::ostream os(&fb);

	enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
	EpsBearer bearer (q);

	 uint16_t dlPort = 1234;
	 // uint16_t ulPort = 2000;
	 // uint16_t otherPort = 3000;
	  PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
	//  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
	//  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
	  ApplicationContainer clientApps;
	  ApplicationContainer serverApps;

	 creUEdevs.clear();
	 noncreUEdevs.clear();
	 macroUEdevs.clear();

	  std::cout<<"Let's check the INDIVIDUAL RSRP VECTORS for every eNB (indexed by subscripts) in the GetStats static function "<<std::endl;
	  std::vector<std::vector<double> > localRsrp;
	  double rsrpThreshold = 180;

	  localRsrp = lteHelper->getRsrpVector();
	  for(uint32_t i=0 ; i < localRsrp.size() ; i++)
	  {
		  for(uint32_t j = 0 ; j < localRsrp.at(i).size() ; j++)
			  std::cout<<"RSRP["<<i+1<<"]["<<j+1<<"]="<<localRsrp.at(i).at(j)<<" and in dB, it is " << 10*log10 (localRsrp.at(i).at(j))<<std::endl;
	  }

	  for(uint32_t i = 0 ; i < localRsrp.size() ; i++)
	  {

		  serverApps.Add (dlPacketSinkHelper.Install (ueNodes.at(i).Get(0)));
		  double max = 0.0 , min = 0.0;
		  uint32_t maxIndex = 0 , minIndex = 0.0;
		  counter = 0;
		  for(uint32_t j = 0 ; j < localRsrp.at(i).size() ; j++)
		  {
			  if(localRsrp.at(i).at(j)>0)
				  counter++;
		  }

		  NS_ASSERT(counter>=1&&counter<=2);
		  if(counter==2)
		  {
			  //lteHelper->ActivateDedicatedEpsBearerCA (ueDevs.at(i) , bearer, EpcTft::Default());



			  for(uint32_t j = 0 ; j < localRsrp.at(i).size() ; j++)
			  {
				  if(localRsrp.at(i).at(j)>0.0)
				  {
				  if(10*log10(localRsrp.at(i).at(j))>max)
				  {
					  if(max>0&&(maxIndex==0||maxIndex==1))
					  {
						  min = max;
						  minIndex = maxIndex;
					  }
					  max = 10*log10(localRsrp.at(i).at(j));
					  maxIndex = j;
				  }
				  }
			  }

			  UdpClientHelper dlClient (ueIpIface.at(i).GetAddress (maxIndex), dlPort);
			  dlClient.SetAttribute("Interval", TimeValue (MilliSeconds(10)));
			  dlClient.SetAttribute("PacketSize", UintegerValue(1000));
			  dlClient.SetAttribute ("MaxPackets", UintegerValue(100000));
			  clientApps.Add (dlClient.Install (remoteHost));

			  std::string ueStat;
			  if (maxIndex == 0 || maxIndex == 1)
			  {
				  if(max>=rsrpThreshold)
				  {
					  ueStat = "Non-CRE";
				  	  noncreUEdevs.push_back(ueDevs.at(i));
				  }
				  else
				  {
					  ueStat = "CRE";
					  creUEdevs.push_back(ueDevs.at(i));
				  }
			  }
			  else
			  {

				  if(min + enbDevs.Get(minIndex)->GetObject<LteEnbNetDevice> ()->GetCellBias() >= max)
				  {
					  ueStat = "CRE";
					  creUEdevs.push_back(ueDevs.at(i));
				  }

				  else
				  {
					  ueStat = "MACRO";
					  macroUEdevs.push_back(ueDevs.at(i));
				  }

			  }

			  os<<"UE with IMSI "<<(i+1)<<" is a "<< ueStat << " with RSRP: "<<max<<"\n";
			  std::cout<<"UE with IMSI "<<(i+1)<<" is a "<< ueStat << "with RSRP: "<<max<<std::endl;
		  }

		  else if(counter==1)
		  {
			  lteHelper->ActivateDedicatedEpsBearer (ueDevs.at(i) , bearer, EpcTft::Default());
			  std::string ueStat;
			  for(uint32_t j = 0 ; j < localRsrp.at(i).size() ; j++)
			  {
				  if(localRsrp.at(i).at(j)>0.0)
				  {
					  if(j>1)
					  {
						  ueStat = "MACRO";

						  macroUEdevs.push_back(ueDevs.at(i));
					  }
					  else
					  {
						  if(10*log10(localRsrp.at(i).at(j))<rsrpThreshold)
						  {
							  ueStat = "CRE";

							  creUEdevs.push_back(ueDevs.at(i));
						  }
						  else
						  {
							  ueStat = "Non-CRE";
							  noncreUEdevs.push_back(ueDevs.at(i));
						  }
					  }
				  }
				  UdpClientHelper dlClient (ueIpIface.at(i).GetAddress (j), dlPort);
				  dlClient.SetAttribute("Interval", TimeValue (MilliSeconds(10)));
				  dlClient.SetAttribute("PacketSize", UintegerValue(1000));
				  dlClient.SetAttribute ("MaxPackets", UintegerValue(100000));
				  clientApps.Add (dlClient.Install (remoteHost));
				  break;
			  }
		  }

		  os<<"\n";
		  os<<" Time : "<<Seconds(Simulator::Now())<<" , Number of CRE users: "<<creUEdevs.size()<<" , Number of Non-CRE users : "<<noncreUEdevs.size()<<" , Number of macro Users : "<<macroUEdevs.size()<<"\n";

	/*	  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
		  EpsBearer bearer (q);
		  lteHelper->ActivateDataRadioBearer (ueDevs, bearer); */
	  }

	  serverApps.Start (Simulator::Now());
	  clientApps.Start (Simulator::Now());

	  ConfigureABS (lteHelper , creUEdevs , noncreUEdevs , macroUEdevs , enbDevs);
	  //Simulator::Schedule (MilliSeconds (5) , &GetStats , lteHelper, ueNodes, creUEs, noncreUEs);
}

uint32_t getABSDuration()
{
	return 2;
}

uint32_t getABSFrameNumber ()
{
	return 6;
}

//static void ConfigureABS ( uint32_t currentTime , Ptr<LteHelper> lteHelper , std::vector<NetDeviceContainer> creUEdevs , std::vector<NetDeviceContainer> noncreUEdevs ,std::vector<NetDeviceContainer> macroUEdevs , std::multimap<NetDeviceContainer , Ptr<NetDevice> > m_uedeviceenbmap, NetDeviceContainer enbDevs)
static void ConfigureABS ( Ptr<LteHelper> lteHelper , std::vector<NetDeviceContainer> creUEdevs , std::vector<NetDeviceContainer> noncreUEdevs ,std::vector<NetDeviceContainer> macroUEdevs , NetDeviceContainer enbDevs)
{
	//std::pair<std::multimap<NetDeviceContainer, Ptr<NetDevice> >::iterator , std::multimap<NetDeviceContainer , Ptr<NetDevice> >::iterator > m_ueDeviceEnbMapIterator;  getCellBias

	static int ctrTracker = 0;
	static int abs = -1;
	static std::filebuf fb;
	if(ctrTracker == 0)
	 fb.open("//home//rajarajan//Documents//att_workspace//ltea//output.txt",std::ios::out);
	static std::ostream os(&fb);

/*	if(callIndex == 0)
	{
		enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
		EpsBearer bearer (q);
		for(uint32_t noncreCounter = 0 ; noncreCounter < noncreUEdevs.size() ; noncreCounter++ )
			lteHelper->ActivateDedicatedEpsBearerCA(noncreUEdevs.at(noncreCounter) , bearer, EpcTft::Default());
	  		//lteHelper->ActivateDataRadioBearer (noncreUEdevs.at(noncreCounter), bearer);

		for(uint32_t macroCounter = 0 ; macroCounter < macroUEdevs.size() ; macroCounter++ )
			lteHelper->ActivateDedicatedEpsBearerCA(macroUEdevs.at(macroCounter) , bearer, EpcTft::Default());
			//lteHelper->ActivateDataRadioBearer (macroUEdevs.at(macroCounter), bearer);

		for(uint32_t creCounter = 0 ; creCounter < creUEdevs.size() ; creCounter++ )
		 lteHelper->ActivateDedicatedEpsBearerCA(creUEdevs.at(creCounter) , bearer, EpcTft::Default());

	} */

	if(abs == -1 || abs == 1)
	{
		std::cout<<"Non-ABS sub-frame "<<(uint32_t)Simulator::Now().GetMilliSeconds()<<std::endl;
		os<<"Non-ABS sub-frame "<<(uint32_t)Simulator::Now().GetMilliSeconds()<<std::endl;
		abs = 0;
		std::cout<<"Macro eNB device 2 (BLANK) : "<<enbDevs.Get(2)->GetObject<LteEnbNetDevice>()->GetScheduler ();
		enbDevs.Get(2)->GetObject<LteEnbNetDevice>()->GetScheduler()->SetBlankIndicator(0);
		enbDevs.Get(2)->GetObject<LteEnbNetDevice>()->GetPhy()->setBlankIndicator(0);
		std::cout<<"Macro eNB device 3 (BLANK) : "<<enbDevs.Get(3)->GetObject<LteEnbNetDevice>()->GetScheduler ();
		enbDevs.Get(3)->GetObject<LteEnbNetDevice>()->GetScheduler()->SetBlankIndicator(0);
		enbDevs.Get(3)->GetObject<LteEnbNetDevice>()->GetPhy()->setBlankIndicator(0);
		std::cout<<"Macro eNB device 4 (BLANK) : "<<enbDevs.Get(4)->GetObject<LteEnbNetDevice>()->GetScheduler ();
		enbDevs.Get(4)->GetObject<LteEnbNetDevice>()->GetScheduler()->SetBlankIndicator(0);
		enbDevs.Get(4)->GetObject<LteEnbNetDevice>()->GetPhy()->setBlankIndicator(0);

		uint32_t currentTime = (uint32_t)Simulator::Now().GetMilliSeconds();
		if ((currentTime%10) < getABSFrameNumber())
			Simulator::Schedule (MilliSeconds(getABSFrameNumber()-(currentTime%10)) , &ConfigureABS , lteHelper , creUEdevs , noncreUEdevs , macroUEdevs, enbDevs);
		else
			Simulator::Schedule (MilliSeconds(10-(currentTime%10)+getABSFrameNumber()) , &ConfigureABS , lteHelper, creUEdevs , noncreUEdevs , macroUEdevs, enbDevs);

	}

	else if (abs == 0)
	{
		std::cout<<"ABS sub-frame "<<(uint32_t)Simulator::Now().GetMilliSeconds()<<std::endl;
		os<<"ABS sub-frame "<<(uint32_t)Simulator::Now().GetMilliSeconds()<<std::endl;
		abs = 1;
		std::cout<<"Macro eNB device 2 (BLANK) : "<<enbDevs.Get(2)->GetObject<LteEnbNetDevice>()->GetScheduler ();
		enbDevs.Get(2)->GetObject<LteEnbNetDevice>()->GetScheduler()->SetBlankIndicator(1);
		enbDevs.Get(2)->GetObject<LteEnbNetDevice>()->GetPhy()->setBlankIndicator(1);
		std::cout<<"Macro eNB device 3 (BLANK) : "<<enbDevs.Get(3)->GetObject<LteEnbNetDevice>()->GetScheduler ();
		enbDevs.Get(3)->GetObject<LteEnbNetDevice>()->GetScheduler()->SetBlankIndicator(1);
		enbDevs.Get(3)->GetObject<LteEnbNetDevice>()->GetPhy()->setBlankIndicator(1);
		std::cout<<"Macro eNB device 4 (BLANK) : "<<enbDevs.Get(4)->GetObject<LteEnbNetDevice>()->GetScheduler ();
		enbDevs.Get(4)->GetObject<LteEnbNetDevice>()->GetScheduler()->SetBlankIndicator(1);
		enbDevs.Get(4)->GetObject<LteEnbNetDevice>()->GetPhy()->setBlankIndicator(1);

	// output_blank
		Simulator::Schedule (MilliSeconds(getABSDuration()) , &ConfigureABS , lteHelper , creUEdevs , noncreUEdevs , macroUEdevs , enbDevs );
	}
}

/*static void ConfigureABSsetup (vector<NetDeviceContainer> ueDevs , LteHelper lteHelper) rsrp ABS sub-frame
{

}
*/

