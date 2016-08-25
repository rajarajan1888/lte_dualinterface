/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */
#include <iostream>
#include <ns3/object.h>
#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/packet-burst.h>
#include <ns3/net-device.h>
#include <ns3/node.h>
#include <ns3/double.h>
#include <ns3/mobility-model.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-propagation-loss-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/antenna-model.h>
#include <ns3/angles.h>

#include <ns3/lte-spectrum-signal-parameters.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/lte-helper.h>

#include <iostream>
#include <fstream>

#include "single-model-spectrum-channel.h"


NS_LOG_COMPONENT_DEFINE ("SingleModelSpectrumChannel");


namespace ns3 {


NS_OBJECT_ENSURE_REGISTERED (SingleModelSpectrumChannel);

SingleModelSpectrumChannel::SingleModelSpectrumChannel ()
{
  NS_LOG_FUNCTION (this);
}

void
SingleModelSpectrumChannel::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_phyList.clear ();
  m_spectrumModel = 0;
  m_propagationDelay = 0;
  m_propagationLoss = 0;
  m_spectrumPropagationLoss = 0;
  SpectrumChannel::DoDispose ();
}

TypeId
SingleModelSpectrumChannel::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::SingleModelSpectrumChannel")
    .SetParent<SpectrumChannel> ()
    .AddConstructor<SingleModelSpectrumChannel> ()
    .AddAttribute ("MaxLossDb",
                   "If a single-frequency PropagationLossModel is used, this value "
                   "represents the maximum loss in dB for which transmissions will be "
                   "passed to the receiving PHY. Signals for which the PropagationLossModel "
                   "returns a loss bigger than this value will not be propagated to the receiver. "
                   "This parameter is to be used to reduce "
                   "the computational load by not propagating signals that are far beyond "
                   "the interference range. Note that the default value corresponds to "
                   "considering all signals for reception. Tune this value with care. ",
                   DoubleValue (1.0e9),
                   MakeDoubleAccessor (&SingleModelSpectrumChannel::m_maxLossDb),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("PathLoss",
                     "This trace is fired "
                     "whenever a new path loss value is calculated. The first and second parameters "
                     "to the trace are pointers respectively to the TX and RX SpectrumPhy instances, "
                     "whereas the third parameters is the loss value in dB. Note that the loss value "
                     "reported by this trace is the single-frequency loss value obtained by evaluating "
                     "only the TX and RX AntennaModels and the PropagationLossModel. In particular, note that "
                     "SpectrumPropagationLossModel (even if present) is never used to evaluate the loss value "
                     "reported in this trace. ",
                     MakeTraceSourceAccessor (&SingleModelSpectrumChannel::m_pathLossTrace))
     .AddTraceSource ("ReportAllCellRsrp",
                      "RSRP statistics ",
                      MakeTraceSourceAccessor (&SingleModelSpectrumChannel::m_reportAllCellRsrp))
  ;
  return tid;
}


void
SingleModelSpectrumChannel::AddRx (Ptr<SpectrumPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phyList.push_back (phy);
}

void
SingleModelSpectrumChannel::AddeNB (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{
  //m_spectrumPropagationLoss
	NS_ASSERT_MSG (m_propagationLoss, "No Spectrum Propagation Loss model defined !");
	m_propagationLoss->AddShadowing (eNBpositionAllocMacro , nUEs);
}

void
SingleModelSpectrumChannel::AddType (char type)
{
	channelType = type;
}

void
SingleModelSpectrumChannel::SetDirection (uint8_t direction)
{
	m_direction = direction;
}

void
SingleModelSpectrumChannel::StartTx (Ptr<SpectrumSignalParameters> txParams)
{
  NS_LOG_FUNCTION (this << txParams->psd << txParams->duration << txParams->txPhy);
  NS_ASSERT_MSG (txParams->psd, "NULL txPsd");
  NS_ASSERT_MSG (txParams->txPhy, "NULL txPhy");

  static std::ofstream myNodes;
  static int counter = 0;
  if(counter==0)
	  myNodes.open("//home//rajarajan//Documents//att_workspace//ltea//node_device.txt");
  counter++;

  static std::ofstream myRsrp;
    static int rsrpcounter = 0;
    if(rsrpcounter==0)
  	  myRsrp.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//rsrp_all.txt");
    rsrpcounter++;


    //if(txParams->packetBurst)
  if(channelType == 'D')
  {
	  Ptr<LteSpectrumSignalParametersDataFrame> params = DynamicCast<LteSpectrumSignalParametersDataFrame>(txParams);
	  if(params->packetBurst)
	  myNodes<<" DATA Cell ID : "<<params->cellId<<" Tx : ID : "<<txParams->txPhy->GetDevice()->GetNode()->GetId()<<"Device : "<<txParams->txPhy<<" Channel : "<<this<<"Time : "<<Simulator::Now().GetMilliSeconds()<<"Burst : "<< params->packetBurst<<" No. of packets = "<<params->packetBurst->GetNPackets()<<"\n";
	  else
	  myNodes<<" UL-DATA Cell ID : "<<params->cellId<<" Tx : ID : "<<txParams->txPhy->GetDevice()->GetNode()->GetId()<<"Device : "<<txParams->txPhy<<" Channel : "<<this<<"Time : "<<Simulator::Now().GetMilliSeconds()<<" Size : "<<params->ctrlMsgList.size()<<"\n";
  }
  else if(channelType == 'C')
  {

	  Ptr<LteSpectrumSignalParametersDlCtrlFrame> params = DynamicCast<LteSpectrumSignalParametersDlCtrlFrame>(txParams);
	  myNodes<<" CTRL Cell ID : "<<params->cellId<<" Tx : ID : "<<txParams->txPhy->GetDevice()->GetNode()->GetId()<<" Device : "<<txParams->txPhy<<" Channel : "<<this<<"Time : "<<Simulator::Now().GetMilliSeconds()<<" No. of packets = 0, Ctrl Msg list = "<<params->ctrlMsgList.size()<<"\n";

	  /*else if(DynamicCast<LteSpectrumSignalParametersUlSrsFrame>(txParams))
	  {
		  Ptr<LteSpectrumSignalParametersUlSrsFrame> params = DynamicCast<LteSpectrumSignalParametersUlSrsFrame>(txParams);
		  myNodes<<" Uplink SRS Tx ID = "<<txParams->txPhy->GetDevice()->GetNode()->GetId()<<" Device : "<<txParams->txPhy<<" Channel : "<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<"\n";
	  }*/
	 //
  }

  else if(channelType == 'S')
    {

  	  Ptr<LteSpectrumSignalParametersUlSrsFrame> params = DynamicCast<LteSpectrumSignalParametersUlSrsFrame>(txParams);
  	  myNodes<<" UL SRS Cell ID : "<<params->cellId<<" Tx : ID : "<<txParams->txPhy->GetDevice()->GetNode()->GetId()<<" Device : "<<txParams->txPhy<<" Channel : "<<this<<"Time : "<<Simulator::Now().GetMilliSeconds()<<"\n";

    }


  switch(channelType)
  {
    case 'D':


    myNodes<<" Type: Data\n";
    break;

    case 'C':
    myNodes<<" Type: CTRL\n";
    break;

    case 'S':
    myNodes<<" Type: SRS\n";
    break;

    case 'H':
    myNodes<<" Type: Half Duplex\n";
    break;

    case 'W':
    myNodes<<" Type: Waveform\n";
    break;

  }
// GetControlMessages ()
  // just a sanity check routine. We might want to remove it to save some computational load -- one "if" statement  ;-)
  if (m_spectrumModel == 0)
    {
      // first pak, record SpectrumModel
      m_spectrumModel = txParams->psd->GetSpectrumModel ();
    }
  else
    {
      // all attached SpectrumPhy instances must use the same SpectrumModel MSG_LIST (eNB PHY)
      NS_ASSERT (*(txParams->psd->GetSpectrumModel ()) == *m_spectrumModel);
    }

  Ptr<MobilityModel> senderMobility = txParams->txPhy->GetMobility ();

  for (PhyList::const_iterator rxPhyIterator = m_phyList.begin ();
       rxPhyIterator != m_phyList.end ();
       ++rxPhyIterator)
    {
      if ((*rxPhyIterator) != txParams->txPhy)
        {
    	  myRsrp<<" Tx : "<<txParams->txPhy->GetDevice()->GetNode()->GetId()<<" and Rx : "<<(*rxPhyIterator)->GetDevice()->GetNode()->GetId()<<"\n";
          Time delay  = MicroSeconds (0);

          Ptr<MobilityModel> receiverMobility = (*rxPhyIterator)->GetMobility ();
          NS_LOG_LOGIC ("copying signal parameters " << txParams);
          Ptr<SpectrumSignalParameters> rxParams = txParams->Copy ();

          if (senderMobility && receiverMobility)
            {
              double pathLossDb = 0;
              if (rxParams->txAntenna != 0)
                {
                  Angles txAngles (receiverMobility->GetPosition (), senderMobility->GetPosition ());
                  double txAntennaGain = rxParams->txAntenna->GetGainDb (txAngles);
                  NS_LOG_LOGIC ("txAntennaGain = " << txAntennaGain << " dB");
                  pathLossDb -= txAntennaGain;
                }
              Ptr<AntennaModel> rxAntenna = (*rxPhyIterator)->GetRxAntenna ();
              if (rxAntenna != 0)
                {
                  Angles rxAngles (senderMobility->GetPosition (), receiverMobility->GetPosition ());
                  double rxAntennaGain = rxAntenna->GetGainDb (rxAngles);
                  NS_LOG_LOGIC ("rxAntennaGain = " << rxAntennaGain << " dB");
                  pathLossDb -= rxAntennaGain;
                }
              if (m_propagationLoss)
                {
            	  if(m_direction=='D')
            		  m_propagationLoss->SetDirection('D');
            	  else if(m_direction=='U')
            		  m_propagationLoss->SetDirection('U');
                  double propagationGainDb = m_propagationLoss->CalcRxPower (0, senderMobility, receiverMobility);
                  NS_LOG_LOGIC ("propagationGainDb = " << propagationGainDb << " dB");
                  pathLossDb -= propagationGainDb;
                }
              NS_LOG_LOGIC ("total pathLoss = " << pathLossDb << " dB");    
              m_pathLossTrace (txParams->txPhy, *rxPhyIterator, pathLossDb);
              myRsrp<<" Path Loss = "<<pathLossDb<<" Maximum Path Loss = "<<m_maxLossDb<<"\n";

              if ( pathLossDb > m_maxLossDb)
                {
                  // beyond range
                  continue;
                }
              double pathGainLinear = pow (10.0, (-pathLossDb) / 10.0);
              *(rxParams->psd) *= pathGainLinear;

              Ptr<LteSpectrumSignalParametersDataFrame> params = DynamicCast<LteSpectrumSignalParametersDataFrame>(txParams);
       		  Ptr <const SpectrumValue> rxPsd = rxParams->psd;
       		  Values::const_iterator it;
      		  double sumrsrp = 0.0;
       		  uint32_t count = 0;
       		  for(it = rxPsd->ConstValuesBegin() ; it != rxPsd->ConstValuesEnd() ; it++ )
       		  {
         		  	  sumrsrp = sumrsrp + (*it);
        		  	  count++;
       		  }
      		  double avgRsrp = sumrsrp/(double)count;
      		  if(channelType == 'D')
      		  {
      			Ptr<LteSpectrumSignalParametersDataFrame> params = DynamicCast<LteSpectrumSignalParametersDataFrame>(txParams);
      			Ptr<NetDevice> ueDev = (*rxPhyIterator)->GetDevice();
      			Ptr<LteUeNetDevice> ueNetDev = DynamicCast<LteUeNetDevice>(ueDev);
      			if(ueNetDev)
      			{
      			uint64_t imsi = ueNetDev->GetImsi();
      			myRsrp<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Cell ID : "<<params->cellId<<" RSRP : "<<avgRsrp<<" IMSI : "<<imsi<<" Count = "<<count<<"\n";
      			//std::cout<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Cell ID : "<<params->cellId<<" RSRP : "<<avgRsrp<<" IMSI : "<<imsi<<" Count = "<<count<<std::endl;
      			//sleep(2);
      			m_reportAllCellRsrp(params->cellId , imsi , avgRsrp);
      			}
      	      }// SRS is TRUE

      		  else if(channelType == 'C')
      		  {

      				  Ptr<LteSpectrumSignalParametersDlCtrlFrame> params = DynamicCast<LteSpectrumSignalParametersDlCtrlFrame>(txParams);
      				  Ptr<NetDevice> ueDev = (*rxPhyIterator)->GetDevice();
      				  Ptr<LteUeNetDevice> ueNetDev = DynamicCast<LteUeNetDevice>(ueDev);
      			  if(ueNetDev)
      			  {
      				  uint64_t imsi = ueNetDev->GetImsi();
      				  myRsrp<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Cell ID : "<<params->cellId<<" RSRP : "<<avgRsrp<<" IMSI : "<<imsi<<" Count = "<<count<<"\n";
      				  m_reportAllCellRsrp(params->cellId , imsi , avgRsrp);
      			  }
      		  }

              if (m_spectrumPropagationLoss)
              {
                  rxParams->psd = m_spectrumPropagationLoss->CalcRxPowerSpectralDensity (rxParams->psd, senderMobility, receiverMobility);
              }

              if (m_propagationDelay)
              {
                  delay = m_propagationDelay->GetDelay (senderMobility, receiverMobility);
              }
          //myNodes<<"Channel : "<<this<<" Node : "<<(*rxPhyIterator)->GetDevice()->GetNode()->GetId()<<" Packet Burst : "<<txParams->packetBurst->GetNPackets()<<" Path Loss : "<<pathLossDb<<"\n";
            }

            Ptr<NetDevice> netDev = (*rxPhyIterator)->GetDevice ();



          //myNodes<<"Channel : "<<this<<" Node : "<<netDev->GetNode()->GetId()<<" Packet Burst : "<<txParams->GetPacketBurst ()<<" PSD: "<<*(txParams->psd)<<" and Spectrum PHY: "<<(*rxPhyIterator)<<"\n";
          if (netDev)
            {
              // the receiver has a NetDevice, so we expect that it is attached to a Node
        //	  Ptr<LteSpectrumPhy> rxvr = DynamicCast<LteSpectrumPhy>(*rxPhyIterator);

        	  myNodes<<" Time : "<<Simulator::Now().GetMilliSeconds() <<" Transmitter : "<<txParams->txPhy<<" and receiver : "<< (*rxPhyIterator) <<"\n";
              uint32_t dstNode =  netDev->GetNode ()->GetId ();
          //    std::cout<<"Net device : "<<netDev<<" UE ID#: "<<dstNode<<std::endl;
              if(rxParams->packetBurst)
                   myNodes<<"Rx : ID : "<<dstNode<<"Device : "<<rxParams->txPhy<<" Channel : "<<this<<"Time : "<<Simulator::Now().GetMilliSeconds()<<"Burst : "<< rxParams->packetBurst<<" No. of packets = "<<rxParams->packetBurst->GetNPackets()<<" Size : "<<rxParams->packetBurst->GetSize()<<"\n";
                else
                   myNodes<<"Rx : ID : "<<dstNode<<"Device : "<<rxParams->txPhy<<" Channel : "<<this<<"Time : "<<Simulator::Now().GetMilliSeconds()<<"Burst : "<< rxParams->packetBurst<<" No. of packets = 0\n";

              Simulator::ScheduleWithContext (dstNode, delay, &SingleModelSpectrumChannel::StartRx, this, rxParams, *rxPhyIterator);
            }
          else
            {
        	  // GetControlMessages ()
              // the receiver is not attached to a NetDevice, so we cannot assume that it is attached to a node
              Simulator::Schedule (delay, &SingleModelSpectrumChannel::StartRx, this,
                                   rxParams, *rxPhyIterator);
            }
        }
    }
}

void
SingleModelSpectrumChannel::StartRx (Ptr<SpectrumSignalParameters> params, Ptr<SpectrumPhy> receiver)
{
  NS_LOG_FUNCTION (this << params);
 // Ptr<LteSpectrumPhy> rxvr = DynamicCast<LteSpectrumPhy>(receiver);

  receiver->StartRx (params);
}



uint32_t
SingleModelSpectrumChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phyList.size ();
}


Ptr<NetDevice>
SingleModelSpectrumChannel::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION (this << i);
  return m_phyList.at (i)->GetDevice ()->GetObject<NetDevice> ();
}


void
SingleModelSpectrumChannel::AddPropagationLossModel (Ptr<PropagationLossModel> loss)
{
  NS_LOG_FUNCTION (this << loss);
  NS_ASSERT (m_propagationLoss == 0);
  m_propagationLoss = loss;
}


void
SingleModelSpectrumChannel::AddSpectrumPropagationLossModel (Ptr<SpectrumPropagationLossModel> loss)
{
  NS_LOG_FUNCTION (this << loss);
//  std::cout<<"Loss = "<<m_spectrumPropagationLoss<<std::endl;
  NS_ASSERT (m_spectrumPropagationLoss == 0);
  m_spectrumPropagationLoss = loss;
}

void
SingleModelSpectrumChannel::SetPropagationDelayModel (Ptr<PropagationDelayModel> delay)
{
  NS_LOG_FUNCTION (this << delay);
  NS_ASSERT (m_propagationDelay == 0);
  m_propagationDelay = delay;
}


Ptr<SpectrumPropagationLossModel>
SingleModelSpectrumChannel::GetSpectrumPropagationLossModel (void)
{
  NS_LOG_FUNCTION (this);
  return m_spectrumPropagationLoss;
}


} // namespace ns3