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
/*
 * Copyright (c) 2012 AT&T Labs Research.
 * This is AT&T Labs Research Proprietary Software.
 * This software has further been edited by AT&T personnel and does no longer
 * abide to the aforementioned statement.
 * Distribution or disclosing of this code is prohibited without prior approval.
 * For permissions and licensing please contact:
 * AT&T Labs Research, 180 Park Ave., Florham Park, NJ 07932, USA
 *
 * Authors: Ioannis Broustis <broustis@research.att.com>
 *
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
//#include "ns3/netanim-module.h"


using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("ICIC-and-eICIC");
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

static void ConfigureABSsetup (NetDeviceContainer noncreDevs, NetDeviceContainer creDevs, NetDeviceContainer uecreDevs, double duration, uint32_t numofdevs)
{
	if (duration == 50.0) // Macro has been transmitting
	{
		duration = 20.0; // Macro needs to be muted now
		for (uint32_t i=0; i<numofdevs; i++) {
			Ptr<LteEnbNetDevice> noncreEnbDev = noncreDevs.Get (i)->GetObject<LteEnbNetDevice> ();
			noncreEnbDev->GetScheduler()->SetBlankIndicator(1); // Tell scheduler to mute
		}
		for (uint32_t i=0; i<numofdevs; i++) {
			Ptr<LteEnbNetDevice> creEnbDev = creDevs.Get (i)->GetObject<LteEnbNetDevice> ();
			creEnbDev->GetScheduler()->SetBlankIndicator(0); // Tell scheduler to recover
		}
	}
	else // Macro has been blanked, or it is the beginning of the simulation
	{
		duration = 50.0; // Macro was muted and needs to start transmitting again
		for (uint32_t i=0; i<numofdevs; i++) {
			Ptr<LteEnbNetDevice> noncreEnbDev = noncreDevs.Get (i)->GetObject<LteEnbNetDevice> ();
			noncreEnbDev->GetScheduler()->SetBlankIndicator(0); // Tell scheduler to recover
		}
		for (uint32_t i=0; i<numofdevs; i++) {
			Ptr<LteEnbNetDevice> creEnbDev = creDevs.Get (i)->GetObject<LteEnbNetDevice> ();
			creEnbDev->GetScheduler()->SetBlankIndicator(1); // Tell scheduler to mute
		}
	}
	// Obtain specific CRE-UE's observed info: edw
	//Ptr<LtePhy> uePhy = uecreDevs.Get (0)->GetObject<LteUeNetDevice> ()->GetPhy ()->GetObject<LtePhy> ();
  //Ptr<SpectrumValue> rxSig = uePhy->GetDownlinkSpectrumPhy ()-> GetPsdRxSignal();
  //Ptr<SpectrumValue> allSigs = uePhy->GetDownlinkSpectrumPhy ()-> GetPsdAllSignals();
  //std::cout << " My eNB signal = " << *rxSig << "All other eNB signals = " << (*allSigs) - (*rxSig) << "\n";

	std::cout << Simulator::Now().GetSeconds () << ":  duration: " << duration << "\n";
	// For now we consider fixed values of ABS and non-ABS frame durations

  Simulator::Schedule (MilliSeconds (duration), &ConfigureABSsetup, noncreDevs, creDevs, uecreDevs, duration, numofdevs);
}


int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MicroSeconds(50)));  
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue(1000000));

	double simTime = 0.25;
	bool icic = false; 
	bool eicic = false;
  bool generateRem = true;	
	bool epc = false; 
	double remRes = 100;
	bool animateMe = false; 
  bool useUdp = true;
  bool epcDl = true;
  bool epcUl = true;
	double remXMin = -500.0;
	double remXMax = 4000.0;
	double remYMin = -500.0;
	double remYMax = 4000.0;
	double rem2XMin = -10.0;
	double rem2XMax = 60.0;
	double rem2YMin = -10.0;
	double rem2YMax = 60.0;

  // Geometry of the scenario (in meters)
	double threesectorX = 2000;
	double threesectorY = 2000;
	double macroX = 500;	
	double macroY = 1300;	
  double nodeHeight = 1.5; // All distance values are in meters
  double roomHeight = 3;
  double roomLength = 10;
	double metroUeXmin = 0.35; 
	double metroUeXmax = 0.35; 
	double metroUeYmin = 0.35; 
	double metroUeYmax = 0.35; 
  uint32_t nRooms = 1;

	uint32_t nEnb = 1; 
  uint32_t nUe = 1; // Number of UEs per eNB

	// ICIC related parameters
	double metroTxPower_f1 = 30.0;
	double metroTxPower_f2 = 30.0;
	double macroTxPower_f1 = 46.0; 
	double macroTxPower_f2 = 46.0; 
  uint16_t DlEarfcn_f1 = 100; 
  uint16_t DlEarfcn_f2 = 500;  
  //uint16_t DlEarfcn_f1 = 5020; // 730 mhz
  //uint16_t DlEarfcn_f2 = 5220; // 750 mhz   
  uint16_t EnbBandwidth_f1 = 25;
  uint16_t EnbBandwidth_f2 = 25;

  //eICIC related parameters
  double abs_duration = 20.0; // in milliseconds
  double non_abs_duration = 50.0;
  uint32_t nUe_cre = 3; // Number of CRE UEs per defined eNB by default (actual decision made later)
  uint32_t nUe_xtra = 10; // Extra UEs attached to specific eNBs for increasing load

  CommandLine cmd;

  cmd.AddValue ("icic", "Enable ICIC with Soft Frequency Reuse in 2 carriers", icic);
  cmd.AddValue ("eicic", "Enable eICIC", eicic);
  cmd.AddValue ("abs_duration", "ABS duration, where UEs in CRE region are scheduled", abs_duration);
  cmd.AddValue ("non_abs_duration", "Non ABS duration", non_abs_duration);
  cmd.AddValue ("generateRem", "Generate REM(s) only and then stop", generateRem);
  cmd.AddValue ("animateMe", "Generate xml file for offline animation", animateMe);
  cmd.AddValue ("remRes", "REM resolution", remRes);
  cmd.AddValue ("remXMin", "REM boundary", remXMin);
  cmd.AddValue ("remXMax", "REM boundary", remXMax);
  cmd.AddValue ("remYMin", "REM boundary", remYMin);
  cmd.AddValue ("remYMax", "REM boundary", remYMax);
  cmd.AddValue ("rem2XMin", "REM2 boundary", rem2XMin);
  cmd.AddValue ("rem2XMax", "REM2 boundary", rem2XMax);
  cmd.AddValue ("rem2YMin", "REM2 boundary", rem2YMin);
  cmd.AddValue ("rem2YMax", "REM2 boundary", rem2YMax);
  cmd.AddValue ("epc", "Enable EPC for e2e connectivity", epc);
  cmd.AddValue ("useUdp", "Use UDP traffic", useUdp);
  cmd.AddValue ("epcDl", "Activate EPC for DL", epcDl);
  cmd.AddValue ("epcUl", "Activate EPC for UL", epcUl);
  cmd.AddValue ("simTime", "Simulated network time", simTime);
  cmd.AddValue ("threesectorX", "X-coordinate for 3-sector node", threesectorX);
  cmd.AddValue ("threesectorY", "Y-coordinate for 3-sector node", threesectorY);
  cmd.AddValue ("macroX", "X-coordinate for macro omni node", macroX);
  cmd.AddValue ("macroY", "Y-coordinate for macro omni node", macroY);
  cmd.AddValue ("nodeHeight", "Height of metro eNB", nodeHeight);
  cmd.AddValue ("roomHeight", "Height of room", roomHeight);
  cmd.AddValue ("roomLength", "Length of each room", roomLength);
  cmd.AddValue ("metroUeXmin", "X-axis min extension in meters for metro UE placement", metroUeXmin);
  cmd.AddValue ("metroUeXmax", "X-axis max extension in meters for metro UE placement", metroUeXmax);
  cmd.AddValue ("metroUeYmin", "Y-axis min extension in meters for metro UE placement", metroUeYmin);
  cmd.AddValue ("metroUeYmax", "Y-axis max extension in meters for metro UE placement", metroUeYmax);
  cmd.AddValue ("nRooms", "Number of rooms per axis in the building", nRooms);
  cmd.AddValue ("nEnb", "Total number of eNodeBs", nEnb);
  cmd.AddValue ("nUe", "Number of UEs associated with each eNB", nUe);
  cmd.AddValue ("metroTxPower_f1", "Tx power for metro eNB for F1 carrier", metroTxPower_f1);
  cmd.AddValue ("metroTxPower_f2", "Tx power for metro eNB for F2 carrier", metroTxPower_f2);
  cmd.AddValue ("macroTxPower_f1", "Tx power for macro eNB for F1 carrier", macroTxPower_f1);
  cmd.AddValue ("macroTxPower_f2", "Tx power for macro eNB for F2 carrier", macroTxPower_f2);
  cmd.AddValue ("DlEarfcn_f1", "EARFCN that corresponds to F1 carrier", DlEarfcn_f1);
  cmd.AddValue ("DlEarfcn_f2", "EARFCN that corresponds to F2 carrier", DlEarfcn_f2);
  cmd.AddValue ("EnbBandwidth_f1", "Bandwidth in number of RBs for F1 carrier", EnbBandwidth_f1);
  cmd.AddValue ("EnbBandwidth_f2", "Bandwidth in number of RBs for F2 carrier", EnbBandwidth_f2);


  cmd.Parse (argc, argv);
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  cmd.Parse (argc, argv);

  Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();

	// Enable logging:
  //lteHelper->EnableLogComponents ();

  //lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));
  //lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::OhBuildingsPropagationLossModel"));
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::HybridBuildingsPropagationLossModel"));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaExtWalls", DoubleValue (0));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaOutdoor", DoubleValue (1));
  lteHelper->SetPathlossModelAttribute ("ShadowSigmaIndoor", DoubleValue (1.5));
  lteHelper->SetPathlossModelAttribute ("Los2NlosThr", DoubleValue (1e6));

  // Create Nodes: eNodeB and UE
  nEnb = nRooms*nRooms + 3;


  NodeContainer enbNodes;
  NodeContainer enbNodes_f2;
  NodeContainer enbNodes_cre;
  NodeContainer oneSectorNodes;
  NodeContainer oneSectorNodes_f2;
  NodeContainer oneSectorNodes_cre; // eNBs that schedule the UEs that are in the CRE region
  NodeContainer threeSectorNodes;
  NodeContainer threeSectorNodes_f2;
  NodeContainer threeSectorNodes_cre;
  vector <NodeContainer> ueNodes;
  vector <NodeContainer> ueNodes_f2;
  vector <NodeContainer> ueNodes_cre;
  vector <NodeContainer> ueNodes_xtra;

  oneSectorNodes.Create (nEnb-3);
  threeSectorNodes.Create (3);
  oneSectorNodes_f2.Create (nEnb-3);
  threeSectorNodes_f2.Create (3);
  oneSectorNodes_cre.Create (nEnb-3);
  threeSectorNodes_cre.Create (3);

  enbNodes.Add (oneSectorNodes);
  enbNodes.Add (threeSectorNodes);
  enbNodes_f2.Add (oneSectorNodes_f2);
  enbNodes_f2.Add (threeSectorNodes_f2);
  enbNodes_cre.Add (oneSectorNodes_cre);
  enbNodes_cre.Add (threeSectorNodes_cre);

  for (uint32_t i = 0; i < nEnb; i++)
    {
      NodeContainer ueNode;
      ueNode.Create (nUe);
      ueNodes.push_back (ueNode);

      NodeContainer ueNode_f2;
      ueNode_f2.Create (nUe);
      ueNodes_f2.push_back (ueNode_f2);

      NodeContainer ueNode_cre;
      ueNode_cre.Create (nUe_cre);
      ueNodes_cre.push_back (ueNode_cre);

      NodeContainer ueNode_xtra;
      ueNode_xtra.Create (nUe_xtra);
      ueNodes_xtra.push_back (ueNode_xtra);
    }

	// Define the EPC helper 
  Ptr<EpcHelper> epcHelper;
  if (epc)
  {
    epcHelper = CreateObject<EpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
  }

	// Topology construction (we replicate for f2-based devices) 

  MobilityHelper mobility;
  vector<Vector> enbPosition;
  Ptr <ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr <Building> building;
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
  mobility.Install (enbNodes_f2);
  mobility.Install (enbNodes_cre);

  uint32_t plantedEnb = 0;
  for (uint32_t row = 0; row < nRooms; row++)
    {
      for (uint32_t column = 0; column < nRooms; column++, plantedEnb++)
      //for (uint32_t column = 0; column < nRooms; column++)
      {
				//if ( (row == 2 && column == 4)  || (row == 3 && column == 1) )
			  //
					//plantedEnb++;
					double randX = 0.5;
					double randY = 0.5;
					//double randX = (1/roomLength) * (rand() % (int)roomLength);
					//double randY = (1/roomLength) * (rand() % (int)roomLength);
          Vector v (roomLength * (column + randX),   
                    roomLength * (row + randY), 
                    nodeHeight );
          positionAlloc->Add (v);
          enbPosition.push_back (v);

          Ptr<BuildingsMobilityModel> mmEnb = enbNodes.Get (plantedEnb)->GetObject<BuildingsMobilityModel> ();
          mmEnb->SetPosition (v);

          // For ICIC:
          Ptr<BuildingsMobilityModel> mmEnb_f2 = enbNodes_f2.Get (plantedEnb)->GetObject<BuildingsMobilityModel> ();
          mmEnb_f2->SetPosition (v);

          // For eICIC:
          Ptr<BuildingsMobilityModel> mmEnb_cre = enbNodes_cre.Get (plantedEnb)->GetObject<BuildingsMobilityModel> ();
          mmEnb_cre->SetPosition (v);
				//}
      }
    }

  // Add an extra 1-sector outdoor site
  //Vector v (macroX, macroY, nodeHeight);
  //positionAlloc->Add (v);
  //enbPosition.push_back (v);
  //mobility.Install (ueNodes.at(plantedEnb));
  //mobility.Install (ueNodes_f2.at(plantedEnb));
  //mobility.Install (ueNodes_cre.at(plantedEnb));
  //plantedEnb++;

  // Add the 3-sector site
  for (uint32_t index = 0; index < 3; index++, plantedEnb++)
    {
      Vector v (threesectorX, threesectorY, nodeHeight);
      positionAlloc->Add (v);
      enbPosition.push_back (v);
      mobility.Install (ueNodes.at(plantedEnb));
      mobility.Install (ueNodes_f2.at(plantedEnb)); // ICIC
      mobility.Install (ueNodes_cre.at(plantedEnb)); // eICIC
      mobility.Install (ueNodes_xtra.at(plantedEnb)); // eICIC
    }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (enbNodes);
  mobility.Install (enbNodes_f2);
  mobility.Install (enbNodes_cre);

  // Position of UEs attached to eNB. //NOTE: The following configuration is scenario dependent!
	// First for F1: 
  for (uint32_t i = 0; i < nEnb; i++)
    {
      Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable> ();
      posX->SetAttribute ("Min", DoubleValue (enbPosition.at(i).x - roomLength * metroUeXmin));
      posX->SetAttribute ("Max", DoubleValue (enbPosition.at(i).x + roomLength * metroUeXmax));
      Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable> ();
      posY->SetAttribute ("Min", DoubleValue (enbPosition.at(i).y - roomLength * metroUeYmin));
      posY->SetAttribute ("Max", DoubleValue (enbPosition.at(i).y + roomLength * metroUeYmax));
      positionAlloc = CreateObject<ListPositionAllocator> ();
	
      for (uint32_t j = 0; j < nUe; j++)
        {
          if ( i == nEnb - 3 ) 
            {
              positionAlloc->Add (Vector (enbPosition.at(i).x + (rand() % (int)(roomLength * 120)),
																					enbPosition.at(i).y + (rand() % (int)(roomLength * 120)),
																					nodeHeight));
            }
          else if ( i == nEnb - 2 )
            {
              positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 120)),
																					enbPosition.at(i).y + (rand() % (int)(roomLength * 120)),
																					nodeHeight));
            }
          else if ( i == nEnb - 1 )
            {
              positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 120)),
																					enbPosition.at(i).y - (rand() % (int)(roomLength * 120)),
																					nodeHeight));
            }
          else
            {
              positionAlloc->Add (Vector (posX->GetValue (), posY->GetValue (), nodeHeight));
            }
          mobility.SetPositionAllocator (positionAlloc);
        }
      mobility.Install (ueNodes.at(i));
    }

	// Then for F2: 
	//int cnt=0;
  for (uint32_t i = 0; i < nEnb; i++)
    {
      Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable> ();
      posX->SetAttribute ("Min", DoubleValue (enbPosition.at(i).x - 3*roomLength * metroUeXmin)); // NOTE
      posX->SetAttribute ("Max", DoubleValue (enbPosition.at(i).x + 5*roomLength * metroUeXmax)); // NOTE
      Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable> ();
      posY->SetAttribute ("Min", DoubleValue (enbPosition.at(i).y - 6*roomLength * metroUeYmin)); // NOTE
      posY->SetAttribute ("Max", DoubleValue (enbPosition.at(i).y + 6*roomLength * metroUeYmax)); // NOTE
      positionAlloc = CreateObject<ListPositionAllocator> ();
				
      for (uint32_t j = 0; j < nUe; j++) 
        {
          if ( i == nEnb - 3 ) 
            {
              positionAlloc->Add (Vector (enbPosition.at(i).x + (rand() % (int)(roomLength * 80)),
										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 80)),
										  										nodeHeight));
            }
          else if ( i == nEnb - 2 )
            {
              positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 80)),
										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 80)),
										  										nodeHeight));
            }
          else if ( i == nEnb - 1 )
            {
              positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 80)),
              														enbPosition.at(i).y - (rand() % (int)(roomLength * 80)),
              														nodeHeight));
            }
          else
            {
          		//positionAlloc->Add (Vector (posX->GetValue (), posY->GetValue (), nodeHeight));
          		positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 2)) + 6,
          	                      				enbPosition.at(i).y + (rand() % (int)(roomLength)),
          	                      				nodeHeight));
            }
          mobility.SetPositionAllocator (positionAlloc);
        }
      mobility.Install (ueNodes_f2.at(i)); 
    }

  	// Then for CRE:
    for (uint32_t i = 0; i < nEnb; i++)
      {
        Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable> ();
        posX->SetAttribute ("Min", DoubleValue (enbPosition.at(i).x + 10*roomLength * metroUeXmin));
        posX->SetAttribute ("Max", DoubleValue (enbPosition.at(i).x + 10*roomLength * metroUeXmax));
        Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable> ();
        posY->SetAttribute ("Min", DoubleValue (enbPosition.at(i).y + 6*roomLength * metroUeYmin));
        posY->SetAttribute ("Max", DoubleValue (enbPosition.at(i).y + 6*roomLength * metroUeYmax));
        positionAlloc = CreateObject<ListPositionAllocator> ();

        for (uint32_t j = 0; j < nUe_cre; j++)
          {
            if ( i == nEnb - 3 )
              {
                positionAlloc->Add (Vector (enbPosition.at(i).x + (rand() % (int)(roomLength * 110)),
  										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 110)),
  										  										nodeHeight));
              }
            else if ( i == nEnb - 2 )
              {
                positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 110)),
  										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 110)),
  										  										nodeHeight));
              }
            else if ( i == nEnb - 1 )
              {
                positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 110)),
  										  										enbPosition.at(i).y - (rand() % (int)(roomLength * 110)),
  										  										nodeHeight));
              }
            else
              {
            		//positionAlloc->Add (Vector (posX->GetValue (), posY->GetValue (), nodeHeight));
                positionAlloc->Add (Vector (enbPosition.at(i).x + (rand() % (int)(roomLength * 165 * metroUeXmin)),
  										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 165 * metroUeXmin)),
  										  										nodeHeight));
              }
            mobility.SetPositionAllocator (positionAlloc);
          }
        mobility.Install (ueNodes_cre.at(i));
      }

  	// Finally for xtra UEs:
    for (uint32_t i = 0; i < nEnb; i++)
      {
        Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable> ();
        posX->SetAttribute ("Min", DoubleValue (enbPosition.at(i).x + 10*roomLength * metroUeXmin));
        posX->SetAttribute ("Max", DoubleValue (enbPosition.at(i).x + 10*roomLength * metroUeXmax));
        Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable> ();
        posY->SetAttribute ("Min", DoubleValue (enbPosition.at(i).y + 6*roomLength * metroUeYmin));
        posY->SetAttribute ("Max", DoubleValue (enbPosition.at(i).y + 6*roomLength * metroUeYmax));
        positionAlloc = CreateObject<ListPositionAllocator> ();

        for (uint32_t j = 0; j < nUe_xtra; j++)
          {
            if ( i == nEnb - 3 )
              {
                positionAlloc->Add (Vector (enbPosition.at(i).x + (rand() % (int)(roomLength * 100)),
  										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 100)),
  										  										nodeHeight));
              }
            else if ( i == nEnb - 2 )
              {
                positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 100)),
  										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 100)),
  										  										nodeHeight));
              }
            else if ( i == nEnb - 1 )
              {
                positionAlloc->Add (Vector (enbPosition.at(i).x - (rand() % (int)(roomLength * 80)),
  										  										enbPosition.at(i).y - (rand() % (int)(roomLength * 97)),
  										  										nodeHeight));
              }
            else
              {
            		//positionAlloc->Add (Vector (posX->GetValue (), posY->GetValue (), nodeHeight));
                positionAlloc->Add (Vector (enbPosition.at(i).x + (rand() % (int)(roomLength * 165 * metroUeXmin)),
  										  										enbPosition.at(i).y + (rand() % (int)(roomLength * 165 * metroUeXmin)),
  										  										nodeHeight));
              }
            mobility.SetPositionAllocator (positionAlloc);
          }
        mobility.Install (ueNodes_xtra.at(i));
      }

  // Create Devices and install them in the Nodes (eNB and UE). 
  // Provide setting with multiple EARFCN values 
  lteHelper->SetSpectrumChannelType ("ns3::MultiModelSpectrumChannel");

	// Configuration for f1 devices 
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (metroTxPower_f1));
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (DlEarfcn_f1));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (DlEarfcn_f1 + 18000));
	if (icic) {
  	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (EnbBandwidth_f1));
  	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (EnbBandwidth_f1));
	}
	else { // There is no splitting of the BW, hence twice as much bandwidth 
  	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (2*EnbBandwidth_f1));
  	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (2*EnbBandwidth_f1));
	}
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  enbDevs = lteHelper->InstallEnbDevice (oneSectorNodes);

  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (macroTxPower_f1));
  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (0));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add (lteHelper->InstallEnbDevice (threeSectorNodes.Get (0)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add (lteHelper->InstallEnbDevice (threeSectorNodes.Get (1)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (2*360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add (lteHelper->InstallEnbDevice (threeSectorNodes.Get (2)));

  NodeContainer ues; // needed for epc

  for (uint32_t i = 0; i < nEnb; i++)
    {
  		ues.Add (ueNodes.at(i));
      NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(i));
      ueDevs.Add (ueDev);
      lteHelper->Attach (ueDev, enbDevs.Get (i));
    }


	// ICIC: Configuration for f2 devices
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (metroTxPower_f2));
  // Restore antenna model setting for metros to isotropic (omni) model 
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
	// Different carrier setting 
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (DlEarfcn_f2));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (DlEarfcn_f2 + 18000));
	if (icic) {
  	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (EnbBandwidth_f2));
  	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (EnbBandwidth_f2));
	}
	else { // There is no splitting of the BW, hence twice as much bandwidth 
  	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (2*EnbBandwidth_f2));
  	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (2*EnbBandwidth_f2));
	}
  NetDeviceContainer enbDevs_f2;
  NetDeviceContainer ueDevs_f2;
  enbDevs_f2 = lteHelper->InstallEnbDevice (oneSectorNodes_f2);

  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (macroTxPower_f2));
  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (0));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs_f2.Add (lteHelper->InstallEnbDevice (threeSectorNodes_f2.Get (0)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs_f2.Add (lteHelper->InstallEnbDevice (threeSectorNodes_f2.Get (1)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (2*360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs_f2.Add (lteHelper->InstallEnbDevice (threeSectorNodes_f2.Get (2)));


	// eICIC: Configuration for CRE devices
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (metroTxPower_f1));
  // Restore antenna model setting for metros to isotropic (omni) model
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
	// Same carrier setting as for devices in F1
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (DlEarfcn_f1));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (DlEarfcn_f1 + 18000));
	if (icic) {
  	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (EnbBandwidth_f1));
  	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (EnbBandwidth_f1));
	}
	else {
		// There is no splitting of the BW, hence twice as much bandwidth (sharing the F1 carrier with non-cre eNBs)
  	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (2*EnbBandwidth_f1));
  	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (2*EnbBandwidth_f1));
	}
  NetDeviceContainer enbDevs_cre;
  NetDeviceContainer ueDevs_cre;
  enbDevs_cre = lteHelper->InstallEnbDevice (oneSectorNodes_cre);

  // Rel. 10 does not consider explicit CRE region scheduling for macros.
  // However this is a potential future area of exploration.
  // Thus we add support for such a case here.
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (macroTxPower_f1));
  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (0));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs_cre.Add (lteHelper->InstallEnbDevice (threeSectorNodes_cre.Get (0)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs_cre.Add (lteHelper->InstallEnbDevice (threeSectorNodes_cre.Get (1)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (2*360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs_cre.Add (lteHelper->InstallEnbDevice (threeSectorNodes_cre.Get (2)));

  // For each of the following cases, scenario-specific topologies need to be created.
	if (icic && !eicic) { // ICIC topological logic
  	for (uint32_t i = 0; i < nEnb; i++)
    {
  		ues.Add (ueNodes_f2.at(i));
      NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes_f2.at(i));
      ueDevs_f2.Add (ueDev);
      lteHelper->Attach (ueDev, enbDevs_f2.Get (i));
    }
	}
	if (eicic && !icic) { // eICIC topological setup - edw
		// Attach only a subset of the metro CRE UEs (e.g. only one) to the metro-cre eNB,
		// and the rest to the interfering macro sector:

    ues.Add (ueNodes_cre.at(0).Get(0));
    NetDeviceContainer ueDev_metro_cre = lteHelper->InstallUeDevice (ueNodes_cre.at(0).Get(0));
    ueDevs_cre.Add (ueDev_metro_cre);
		// Attach the metro CRE UE to the "metro CRE" eNB
    lteHelper->Attach (ueDev_metro_cre, enbDevs_cre.Get (0));
  	//lteHelper->Attach (ueDev_metro_cre, enbDevs.Get (0));
  	//lteHelper->Attach (ueDev_metro_cre, enbDevs.Get (3));

    for (uint32_t j = 1; j < nUe_cre; j++)
    {
    	ues.Add (ueNodes_cre.at(0).Get(j));
    	NetDeviceContainer ueDev_metrocre = lteHelper->InstallUeDevice (ueNodes_cre.at(0).Get(j));
    	ueDevs_cre.Add (ueDev_metrocre);
    	// Attach the remaining metro CRE UEs to metro default:
    	lteHelper->Attach (ueDev_metrocre, enbDevs.Get (0));
    }

    // Assign all the macro CRE UEs to the macro eNB:
		for (uint32_t i = 1; i < nEnb; i++)
    {
      ues.Add (ueNodes_cre.at(i));
      NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes_cre.at(i));
      ueDevs_cre.Add (ueDev);
      lteHelper->Attach (ueDev, enbDevs.Get (i));
    }

    // Assign extra macro CRE UEs to the macro eNB:
		for (uint32_t j = 0; j < nUe_xtra; j++)
    {
    	ues.Add (ueNodes_xtra.at(3).Get(j));
    	NetDeviceContainer ueDev_metrox = lteHelper->InstallUeDevice (ueNodes_xtra.at(3).Get(j));
    	ueDevs_cre.Add (ueDev_metrox);
    	// Attach the remaining metro CRE UEs to metro default:
    	lteHelper->Attach (ueDev_metrox, enbDevs.Get (3));
    }
	}
	if ( (!icic) && (!eicic) ) { // Default network operations
	  for (uint32_t i = 0; i < nEnb; i++)
    {
      ues.Add (ueNodes_f2.at(i));
      NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes_f2.at(i));
      ueDevs_f2.Add (ueDev);
      // NOTE: We attach all UE devices to eNB devices on F1, since we ignore F2 eNodeBs. 
      lteHelper->Attach (ueDev, enbDevs.Get (i));
    }
	}
  if (icic && eicic) {
  	std::cout << "\n WARNING: Both ICIC and eICIC are enabled! Case not supported currently. \n";
    for (uint32_t i = 0; i < nEnb; i++)
      {
        ues.Add (ueNodes.at(i));
        NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(i));
        ueDevs_f2.Add (ueDev);
        // NOTE: We attach all UE devices to eNB devices on F1, since we ignore F2 eNodeBs.
        lteHelper->Attach (ueDev, enbDevs.Get (i));
      }
  }

	// EPC setup: 
  if (epc)
    {
      NS_LOG_LOGIC ("setting up Internet, remote host and applications");

      // Create a single RemoteHost
      NodeContainer remoteHostContainer;
      remoteHostContainer.Create (1);
      Ptr<Node> remoteHost = remoteHostContainer.Get (0);
      InternetStackHelper internet;
      internet.Install (remoteHostContainer);

      // Create the Internet
      PointToPointHelper p2ph;
      p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
      p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
      p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
      Ptr<Node> pgw = epcHelper->GetPgwNode ();
      NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
      Ipv4AddressHelper ipv4h;
      ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
      Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
      // in this container, interface 0 is the pgw, 1 is the remoteHost
      Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
      remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

      // For internetworking purposes, consider together home UEs and macro UEs
      //NodeContainer ues;
      //ues.Add (ueNodes);
      //ues.Add (ueNodes_f2);
      NetDeviceContainer ueDevices;
      ueDevices.Add (ueDevs);
      ueDevices.Add (ueDevs_f2);
      ueDevices.Add (ueDevs_cre);

      // Install the IP stack on the UEs      
      internet.Install (ues);
      Ipv4InterfaceContainer ueIpIfaces;
      ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevices));

      // Install and start applications on UEs and remote host
      uint16_t dlPort = 10000;
      uint16_t ulPort = 20000;

      ApplicationContainer clientApps;
      ApplicationContainer serverApps;
      for (uint32_t u = 0; u < ues.GetN (); ++u)
        {
          Ptr<Node> ue = ues.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
          ++dlPort;
          ++ulPort;
          if (useUdp)
            {
              if (epcDl)
                {
                  NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
                  UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
                  clientApps.Add (dlClientHelper.Install (remoteHost));
                  PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                                       InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                  serverApps.Add (dlPacketSinkHelper.Install (ue));
                }
              if (epcUl)
                {
                  NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
                  UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
                  clientApps.Add (ulClientHelper.Install (ue));
                  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
                                                       InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                  serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
                }
            }
        	else // use TCP
            {
              if (epcDl)
                {
                  NS_LOG_LOGIC ("installing TCP DL app for UE " << u);
                  BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory",
                                                 InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
                  dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));
                  clientApps.Add (dlClientHelper.Install (remoteHost));
                  PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory",
                                                       InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                  serverApps.Add (dlPacketSinkHelper.Install (ue));
                }
              if (epcUl)
                {
                  NS_LOG_LOGIC ("installing TCP UL app for UE " << u);
                  BulkSendHelper ulClientHelper ("ns3::TcpSocketFactory",
                                                 InetSocketAddress (remoteHostAddr, ulPort));
                  ulClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));
                  clientApps.Add (ulClientHelper.Install (ue));
                  PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory",
                                                       InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                  serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
                }
            }
        }
      serverApps.Start (Seconds (0.0));
      clientApps.Start (Seconds (0.0));

    } 
	// end of EPC setup 

  //enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  enum EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
  lteHelper->ActivateDataRadioBearer (ueDevs_f2, bearer);
  lteHelper->ActivateDataRadioBearer (ueDevs_cre, bearer);

  BuildingsHelper::MakeMobilityModelConsistent ();

  Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
  Ptr<RadioEnvironmentMapHelper> remHelper2 = CreateObject<RadioEnvironmentMapHelper> ();
  if (generateRem) {

		PrintGnuplottableBuildingListToFile ("buildings.txt"); 
		PrintGnuplottableEnbListToFile ("enbs.txt");
    PrintGnuplottableUeListToFile ("ues.txt");

  	remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
  	remHelper->SetAttribute ("OutputFile", StringValue ("rem.out"));
    remHelper->SetAttribute ("XMin", DoubleValue (remXMin));
    remHelper->SetAttribute ("XMax", DoubleValue (remXMax));
    remHelper->SetAttribute ("YMin", DoubleValue (remYMin));
    remHelper->SetAttribute ("YMax", DoubleValue (remYMax));
    remHelper->SetAttribute ("XRes", UintegerValue (remRes));
    remHelper->SetAttribute ("YRes", UintegerValue (remRes));
  	remHelper->SetAttribute ("Z", DoubleValue (1.5));
  	remHelper->Install ();

  	remHelper2->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
  	remHelper2->SetAttribute ("OutputFile", StringValue ("rem2.out"));
    remHelper2->SetAttribute ("XMin", DoubleValue (rem2XMin));
    remHelper2->SetAttribute ("XMax", DoubleValue (rem2XMax));
    remHelper2->SetAttribute ("YMin", DoubleValue (rem2YMin));
    remHelper2->SetAttribute ("YMax", DoubleValue (rem2YMax));
    remHelper2->SetAttribute ("XRes", UintegerValue (remRes));
    remHelper2->SetAttribute ("YRes", UintegerValue (remRes));
  	remHelper2->SetAttribute ("Z", DoubleValue (1.5));
  	remHelper2->Install ();

	}
	else {
		if (eicic) { //edw
			Simulator::Schedule (MilliSeconds (non_abs_duration), &ConfigureABSsetup, enbDevs, enbDevs_cre, ueDevs_cre, abs_duration, nEnb);
			Simulator::Stop (Seconds (simTime));
		}
		else {
			Simulator::Stop (Seconds (simTime));
		}
		lteHelper->EnableMacTraces ();
		lteHelper->EnableRlcTraces ();
		if (epc) {
			lteHelper->EnablePdcpTraces ();
		}
	}
/*
	if (animateMe) {
  	// Create the animation object and configure for specified output
		std::string animFile = "animation.xml" ;  // Name of file for animation output
  	AnimationInterface anim (animFile);
	}
*/
  Simulator::Run ();

//  GtkConfigStore config;
//  config.ConfigureAttributes ();

  lteHelper = 0;
  Simulator::Destroy ();
  return 0;
}
