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
#include <cstdlib>
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
//  Simulator::Schedule (MilliSeconds (firstABS), &ConfigureABSsetup, enbDevs, enbDevs_cre, currentFrameABSLength-1, ABSVtr);
//static void ConfigureABSsetup (NetDeviceContainer noncreDevs, NetDeviceContainer creDevs, double duration, uint32_t numofdevs)
uint8_t determineFrameFreq();
uint8_t determineABS();
static void ConfigureABSsetup (uint8_t frameFreq , NetDeviceContainer noncreDevs, NetDeviceContainer creDevs, std::vector<int> ABSVtr)
{
	static unsigned int index = 0;
	static int absflag = 0;
	static int framenum = 0;
	if(index == ABSVtr.size())
	{

		framenum++;
		// FRAMEFREQ : FOR HOW MANY FRAMES SHOULD THE CURRENT DETERMINATION OF ABS LENGTHS AND ABS SUB-FRAMES HOLD GOOD ?
		if(framenum == frameFreq)
		{
			// COMPUTER HOW MANY ABS SUB-FRAMES PER FRAME (ABS LENGTH) FOR THE NEXT "N" FRAMES
			frameFreq = determineFrameFreq();
			uint8_t currentFrameABSLength = determineABSlength();
			ABSVtr.clear();
			for(uint8_t absctr = 1 ; absctr <= currentFrameABSLength ; absctr++)
			{
				unsigned int tracker;
				uint8_t absNo;
				do
				{
				  // IDENTIFY THE ABS SUB-FRAME
				  absNo = determineABS();
				  for(tracker = 0 ; tracker < ABSVtr.size() ; tracker++)
				  {
					  //AVOID ABS DUPLICATION
					  if(ABSVtr.at(tracker)==absNo)
					  break;
				  }
				}while(tracker<ABSVtr.size());
				ABSVtr.push_back(absNo);
			}
			//SORT ABS's IN INCREASING ORDER OF TIME SUB-FRAMES
			std::sort(ABSVtr.begin() , ABSVtr.end());
			framenum = 0;
		}
		index = 0;
		absflag = 0;
	}

	if(absflag == 0)
	{
		unsigned int ctr = 0;
		for ( NetDeviceContainer::Iterator dev = noncreDevs.Begin() ; dev != noncreDevs.End() ; dev++ )
		{
			Ptr<LteEnbNetDevice> noncreEnbDev = noncreDevs.Get(ctr)->GetObject<LteEnbNetDevice> ();
			noncreEnbDev->GetScheduler()->SetBlankIndicator(0);
			ctr++;
		}
		unsigned int ctr = 0;
		for ( NetDeviceContainer::Iterator dev = creDevs.Begin() ; dev != creDevs.End() ; dev++ )
		{
			Ptr<LteEnbNetDevice> creEnbDev = creDevs.Get(ctr)->GetObject<LteEnbNetDevice> ();
			creEnbDev->GetScheduler()->SetBlankIndicator(1);
			ctr++;
		}

		absflag = 1;
		if(index==0)
		{
			index++;
			std::cout << Simulator::Now().GetSeconds () << ":  duration: " << ABSVtr.at(index-1) << "ms\n";
			Simulator::Schedule (MilliSeconds (ABSVtr.at(index-1)), &ConfigureABSsetup, noncreDevs, creDevs, ABSVtr);
		}

		else if(index == ABSVtr.size()-1)
		{
			index++;
			std::cout << Simulator::Now().GetSeconds () << ":  duration: " << ABSVtr.at(index-1) << "ms\n";
			Simulator::Schedule (MilliSeconds (10-ABSVtr.at(index-1)), &ConfigureABSsetup, noncreDevs, creDevs, ABSVtr);
		}

		else
		{
			index++;
			std::cout << Simulator::Now().GetSeconds () << ":  duration: " << ABSVtr.at(index-1)-ABSVtr.at(index-2) << "ms\n";
			Simulator::Schedule (MilliSeconds (ABSVtr.at(index-1)-ABSVtr.at(index-2)), &ConfigureABSsetup, noncreDevs, creDevs, ABSVtr);
		}
	}

	else if(absflag == 1)
	{
		unsigned int ctr = 0;
		for ( NetDeviceContainer::Iterator dev = noncreDevs.Begin() ; dev != noncreDevs.End() ; dev++ )
		{
			Ptr<LteEnbNetDevice> noncreEnbDev = noncreDevs.Get(ctr)->GetObject<LteEnbNetDevice> ();
			noncreEnbDev->GetScheduler()->SetBlankIndicator(1);
			ctr++;
		}
		unsigned int ctr = 0;
		for ( NetDeviceContainer::Iterator dev = creDevs.Begin() ; dev != creDevs.End() ; dev++ )
		{
			Ptr<LteEnbNetDevice> creEnbDev = creDevs.Get(ctr)->GetObject<LteEnbNetDevice> ();
			creEnbDev->GetScheduler()->SetBlankIndicator(0);
			ctr++;
		}

		absflag = 0;
		if(index == ABSVtr.size()-1)
		{
			index++;
			std::cout << Simulator::Now().GetSeconds () << ":  duration: " << ABSVtr.at(index-1) << "ms\n";
			Simulator::Schedule (MilliSeconds (10-ABSVtr.at(index-1)), &ConfigureABSsetup, noncreDevs, creDevs, ABSVtr);
		}

		else
		{
			index++;
			std::cout << Simulator::Now().GetSeconds () << ":  duration: " << ABSVtr.at(index-1)-ABSVtr.at(index-2) << "ms\n";
			Simulator::Schedule (MilliSeconds (ABSVtr.at(index-1)-ABSVtr.at(index-2)), &ConfigureABSsetup, noncreDevs, creDevs, ABSVtr);
		}
	}



/*	if (duration == 50.0) // Macro has been transmitting
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
	} */
	// Obtain specific CRE-UE's observed info:
	//Ptr<LtePhy> uePhy = uecreDevs.Get (0)->GetObject<LteUeNetDevice> ()->GetPhy ()->GetObject<LtePhy> ();
  //Ptr<SpectrumValue> rxSig = uePhy->GetDownlinkSpectrumPhy ()-> GetPsdRxSignal();
  //Ptr<SpectrumValue> allSigs = uePhy->GetDownlinkSpectrumPhy ()-> GetPsdAllSignals();
  //std::cout << " My eNB signal = " << *rxSig << "All other eNB signals = " << (*allSigs) - (*rxSig) << "\n";

	// std::cout << Simulator::Now().GetSeconds () << ":  duration: " << duration << "\n";
	// For now we consider fixed values of ABS and non-ABS frame durations

  //Simulator::Schedule (MilliSeconds (duration), &ConfigureABSsetup, noncreDevs, creDevs, duration, numofdevs);
}

uint8_t determineABSlength()
{
	srand(time(NULL));

	//Write our algorithm to determine ABS length. Just a hack through randomization !
	uint8_t randABSlength = ((rand()%5)+1);
	std::cout<<"ABS length = "<<randABSlength<<std::endl;
	return randABSlength;
}

uint8_t determineABS()
{
	srand(time(NULL));
	uint8_t randABS = ((rand()%10)+1);
	return randABS;
}

uint8_t determineFrameFreq()
{
	srand(time(NULL));
	uint8_t frameFreq = ((rand()%5)+1);
	return frameFreq;
}

int
main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MicroSeconds(100)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
  srand(time(NULL));

  bool generateRem = true;
  bool attachue1tometro = true;
  bool eicic = false;
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
  double abs_duration = 20.0; // in milliseconds
  double non_abs_duration = 50.0;

  CommandLine cmd;
  cmd.AddValue ("generateRem", "Generate REM(s) only and then stop", generateRem);
  cmd.AddValue ("macroXcoord", "X coordinate for macro eNB", macroXcoord);
  cmd.AddValue ("macroYcoord", "Y coordinate for macro eNB", macroYcoord);
  cmd.Parse (argc, argv);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  cmd.Parse (argc, argv);

  Ptr < LteHelper > lteHelper = CreateObject<LteHelper> ();
  //lteHelper->EnableLogComponents ();
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));

  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer oneSectorNodes;
  NodeContainer threeSectorNodes;
  vector < NodeContainer > ueNodes;

  oneSectorNodes.Create (nEnb-3);
  threeSectorNodes.Create (3);

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
          Vector v (roomLength * (column + 0.5),
                    roomLength * (row + 0.5),
                    nodeHeight );
          positionAlloc->Add (v);
          enbPosition.push_back (v);
          Ptr<BuildingsMobilityModel> mmEnb = enbNodes.Get (plantedEnb)->GetObject<BuildingsMobilityModel> ();
          mmEnb->SetPosition (v);
        }
    }

  // Add a 1-sector site
  Vector v (1500, 3000, nodeHeight);
  std::cout<<"eNB One Sector, X = 1500, Y = 3000, Node height = "<<nodeHeight<<std::endl;
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
              positionAlloc->Add (Vector (enbPosition.at(i).x + 400, enbPosition.at(i).y, nodeHeight));
              std::cout<<"UE "<<i<<" X = "<<enbPosition.at(i).x + 400<<", Y = "<<enbPosition.at(i).y<<", Node height = "<<nodeHeight<<std::endl;
            }
          else if ( i == nEnb - 2 )
            {
        	  std::cout<<"UE "<<i<<" X = "<<enbPosition.at(i).x - 400<<", Y = "<<enbPosition.at(i).y + 600<<", Node height = "<<nodeHeight<<std::endl;
              positionAlloc->Add (Vector (enbPosition.at(i).x - 400, enbPosition.at(i).y + 600, nodeHeight));
            }
          else if ( i == nEnb - 1 )
            {
        	  std::cout<<"UE "<<i<<" X = "<<enbPosition.at(i).x - 500<<", Y = "<<enbPosition.at(i).y - 1300<<", Node height = "<<nodeHeight<<std::endl;
              positionAlloc->Add (Vector (enbPosition.at(i).x - 500, enbPosition.at(i).y - 1300, nodeHeight));
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

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  vector < NetDeviceContainer > ueDevs;

  // power setting in dBm for small cells
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (22.0));
  enbDevs = lteHelper->InstallEnbDevice (oneSectorNodes);


  // power setting for three-sector macrocell
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (43.0));

  // Beam width is made quite narrow so sectors can be noticed in the REM
  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (0));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add ( lteHelper->InstallEnbDevice (threeSectorNodes.Get (0)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add ( lteHelper->InstallEnbDevice (threeSectorNodes.Get (1)));

  lteHelper->SetEnbAntennaModelType ("ns3::CosineAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Orientation", DoubleValue (2*360/3));
  lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (100));
  lteHelper->SetEnbAntennaModelAttribute ("MaxGain",     DoubleValue (0.0));
  enbDevs.Add ( lteHelper->InstallEnbDevice (threeSectorNodes.Get (2)));

  if (attachue1tometro == false) {
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
  	for (uint32_t i = 0; i < nEnb; i++)
    {
      NetDeviceContainer ueDev = lteHelper->InstallUeDevice (ueNodes.at(i));
      ueDevs.push_back (ueDev);
      lteHelper->Attach (ueDev, enbDevs.Get (i));
      enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
      EpsBearer bearer (q);
      lteHelper->ActivateDataRadioBearer (ueDev, bearer);
    }
  }

  BuildingsHelper::MakeMobilityModelConsistent ();

  // by default, simulation will anyway stop right after the REM has been generated
 // Simulator::Schedule (MilliSeconds (non_abs_duration), &ConfigureABSsetup, enbDevs, enbDevs_cre, abs_duration, nEnb);
  Simulator::Stop (Seconds (0.01));

  Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
  if (generateRem) {
	  std::cout<<"JUST NOT THERE? "<<std::endl;
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
  }
  else {
	  std::cout<<"ARE WE HERE ? "<<std::endl;
	  if(eicic)
	  {
		  // FRAMEFREQ : FOR HOW MANY FRAMES CURRENT ABS LENGTH AND ABS SUB FRAMES WILL HOLD GOOD ?
		  uint8_t frameFreq = determineFrameFreq();
		  // DETERMINE ABS LENGTH
		  uint8_t currentFrameABSLength = determineABSlength();
		  std::vector<int> ABSVtr;
		  for(uint8_t absctr = 1 ; absctr <= currentFrameABSLength ; absctr++)
		  {
			  unsigned int tracker;
			  uint8_t absNo;
			  do
			  {
				  //DETERMINE EACH ABS
				  absNo = determineABS();
				  for(tracker = 0 ; tracker < ABSVtr.size() ; tracker++)
				  {
					  //AVOID ABS DUPLICATION
					  if(ABSVtr.at(tracker)==absNo)
					  break;
				  }
			  }while(tracker<ABSVtr.size());
			  ABSVtr.push_back(absNo);
		  }
		  //SORT VECTOR SO THAT ABS's ARE IN INCREASING ORDER OF TIME
		  std::sort(ABSVtr.begin() , ABSVtr.end());
		  ConfigureABSsetup(frameFreq , enbDevs, enbDevs_cre, ABSVtr);
		  //Simulator::Schedule (MilliSeconds (non_abs_duration), &ConfigureABSsetup, enbDevs, enbDevs_cre, abs_duration, nEnb);
		  Simulator::Stop (Seconds (simTime));
      }
	  else {
			Simulator::Stop (Seconds (simTime));
	  }

		lteHelper->EnablePhyTraces ();
		lteHelper->EnableMacTraces ();
		lteHelper->EnableRlcTraces ();
  }
  Simulator::Run ();

  lteHelper = 0;
  Simulator::Destroy ();
  return 0;
}
