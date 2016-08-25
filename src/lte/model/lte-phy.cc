/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Marco Miozzo <mmiozzo@cttc.es>
 */

#include <ns3/waveform-generator.h>
#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <math.h>
#include <ns3/simulator.h>
#include "ns3/spectrum-error-model.h"
#include "lte-phy.h"
#include "lte-net-device.h"

#include <iostream>
#include <fstream>

NS_LOG_COMPONENT_DEFINE ("LtePhy");

namespace ns3 {


NS_OBJECT_ENSURE_REGISTERED (LtePhy);


LtePhy::LtePhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

LtePhy::LtePhy (Ptr<LteSpectrumPhy> dlPhy, Ptr<LteSpectrumPhy> ulPhy)
  : m_downlinkSpectrumPhy (dlPhy),
    m_uplinkSpectrumPhy (ulPhy),
    m_tti (0.001),
    m_ulBandwidth (0),
    m_dlBandwidth (0),
    m_rbgSize (0),
    m_macChTtiDelay (0),
    m_cellId (0)
{
  NS_LOG_FUNCTION (this);
  blankIndicator = 0;
}


TypeId
LtePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LtePhy")
    .SetParent<Object> ()
  ;
  return tid;
}


LtePhy::~LtePhy ()
{
  NS_LOG_FUNCTION (this);
}

void
LtePhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_packetBurstQueue.clear ();
  m_controlMessagesQueue.clear ();
  m_downlinkSpectrumPhy->Dispose ();
  m_downlinkSpectrumPhy = 0;
  m_uplinkSpectrumPhy->Dispose ();
  m_uplinkSpectrumPhy = 0;
  m_netDevice = 0;
  Object::DoDispose ();
}

void
LtePhy::SetDevice (Ptr<LteNetDevice> d)
{
  NS_LOG_FUNCTION (this << d);
  m_netDevice = d;
}


Ptr<LteNetDevice>
LtePhy::GetDevice ()
{
  NS_LOG_FUNCTION (this);
  return m_netDevice;
}

Ptr<LteSpectrumPhy> 
LtePhy::GetDownlinkSpectrumPhy ()
{
  return m_downlinkSpectrumPhy;
}

Ptr<LteSpectrumPhy> 
LtePhy::GetUplinkSpectrumPhy ()
{
  return m_uplinkSpectrumPhy;
}

void
LtePhy::setBlankIndicator (uint32_t blankIndicator)
{
	this->blankIndicator = blankIndicator;
}

uint32_t
LtePhy::getBlankIndicator () const
{
	return this->blankIndicator;
}

void
LtePhy::SetDownlinkChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_downlinkSpectrumPhy->SetChannel (c);
}

void
LtePhy::SetUplinkChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_uplinkSpectrumPhy->SetChannel (c);
}

void
LtePhy::SetTti (double tti)
{
  NS_LOG_FUNCTION (this << tti);
  m_tti = tti;
}


double
LtePhy::GetTti (void) const
{
  NS_LOG_FUNCTION (this << m_tti);
  return m_tti;
}


uint16_t
LtePhy::GetSrsPeriodicity (uint16_t srcCi) const
{
  // from 3GPP TS 36.213 table 8.2-1 UE Specific SRS Periodicity
  uint16_t SrsPeriodicity[9] = {0, 2, 5, 10, 20, 40, 80, 160, 320};
  uint16_t SrsCiLow[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiHigh[9] = {0, 1, 6, 16, 36, 76, 156, 316, 636};
  uint8_t i;
  for (i = 8; i > 0; i --)
    {
      if ((srcCi>=SrsCiLow[i])&&(srcCi<=SrsCiHigh[i]))
        {
          break;
        }
    }
  return SrsPeriodicity[i];
}

uint16_t
LtePhy::GetSrsSubframeOffset (uint16_t srcCi) const
{
  // from 3GPP TS 36.213 table 8.2-1 UE Specific SRS Periodicity
  uint16_t SrsSubframeOffset[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiLow[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiHigh[9] = {0, 1, 6, 16, 36, 76, 156, 316, 636};
  uint8_t i;
  for (i = 8; i > 0; i --)
    {
      if ((srcCi>=SrsCiLow[i])&&(srcCi<=SrsCiHigh[i]))
        {
          break;
        }
    }
  return (srcCi - SrsSubframeOffset[i]);
}

uint8_t
LtePhy::GetRbgSize (void) const
{
  return m_rbgSize;
}

void
LtePhy::SetMacPdu (Ptr<Packet> p)
{
	static std::ofstream myFileMac;
	static int counter = 0;
	if(counter==0)
		myFileMac.open("//home//rajarajan//Documents//att_workspace//ltea//MACpdusize.txt");
	counter++;
	myFileMac<<"Hi\n";
	if(p)
	myFileMac<<" Packet : "<<p<<" Size : "<<p->GetSize()<<"\n";
	else
		myFileMac<<"Packet empty\n";
  m_packetBurstQueue.at (m_packetBurstQueue.size () - 1)->AddPacket (p);
}
//avgThr
Ptr<PacketBurst>
LtePhy::GetPacketBurst (void)
{
  static std::ofstream myFile11;
  static int counter = 0;
  if(counter==0)
	     myFile11.open("//home//rajarajan//Documents//att_workspace//ltea//GetPacketBurst_GEN.txt");
  counter++;
  myFile11<<"Packet burst size : "<<m_packetBurstQueue.at(0)->GetSize()<<" time : "<<Simulator::Now()<<"\n";
  if (m_packetBurstQueue.at (0)->GetSize () > 0)
    {

      Ptr<PacketBurst> ret = m_packetBurstQueue.at (0)->Copy ();
      	myFile11<<"Packet Burst is " << m_packetBurstQueue.at(0)->GetSize()<<" RET : "<<ret<<" time : "<<Simulator::Now().GetMilliSeconds()<<"\n\n";
      	// 	        myFile11.close();
      m_packetBurstQueue.erase (m_packetBurstQueue.begin ());
      m_packetBurstQueue.push_back (CreateObject <PacketBurst> ());
      return (ret);
    }
  else
    {


	        myFile11<<"Packet Burst is 0 \n\n";

      m_packetBurstQueue.erase (m_packetBurstQueue.begin ());
      m_packetBurstQueue.push_back (CreateObject <PacketBurst> ());
      return (0);
    }
}


void
LtePhy::SetControlMessages (Ptr<LteControlMessage> m)
{

  static std::ofstream ctrlmsg ;
  static int counter = 0;
  if(counter==0)
	  ctrlmsg.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//ctrlset.txt");
  counter++;


  //Ptr<DlCqiLteControlMessage> msg = DynamicCast<DlCqiLteControlMessage>(m);
  /*if(msg)
  {
	  ctrlmsg<<" Message type : "<<msg->GetDlCqi().m_cqiType<<"\n";
	  if(msg->GetDlCqi().m_cqiType==CqiListElement_s::A30)
	  {
		  for(unsigned int i = 0 ; i < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.size() ; i++)
			  for(unsigned int j = 0 ; j < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.size() ; j++)
				  ctrlmsg<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Confirmation CTRL : "<<(uint16_t)msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.at(j)<<"\n";
	  }
	  else if(msg->GetDlCqi().m_cqiType==CqiListElement_s::P10)
	  {
		  for(unsigned int i = 0 ; i < msg->GetDlCqi().m_wbCqi.size() ; i++)
			  ctrlmsg<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Confirmation CTRL : "<<(uint16_t)msg->GetDlCqi().m_wbCqi.at(i)<<"\n";
	  }
  }*/

  // In uplink the queue of control messages and packet are of different sizes
  // for avoiding TTI cancellation due to synchronization of subframe triggers
  m_controlMessagesQueue.at (m_controlMessagesQueue.size () - 1).push_back (m);
  if(DynamicCast<DlDciLteControlMessage>(m))
  	  ctrlmsg<<" Time : "<<Simulator::Now().GetMilliSeconds()<<this<<" DCI"<<" Current Size = "<<m_controlMessagesQueue.size()<<" Size = "<<m_controlMessagesQueue.at(0).size()<<"\n";
    else if(DynamicCast<DlCqiLteControlMessage>(m))
  	  ctrlmsg<<" Time : "<<Simulator::Now().GetMilliSeconds()<<this<<" CQI"<<" Current Size = "<<m_controlMessagesQueue.size()<<" Size = "<<m_controlMessagesQueue.at(0).size()<<"\n";
    else if(DynamicCast<UlDciLteControlMessage>(m))
    {
    	std::cout<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" UL DCI Message "<<std::endl;
    //	sleep(3);
    }

}
// UL_DCI
std::list<Ptr<LteControlMessage> >
LtePhy::GetControlMessages (void)
{
  NS_LOG_FUNCTION (this);
  static std::ofstream getctrl;
  static int ctrlctr = 0;
  if(ctrlctr == 0)
	  getctrl.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//get_ctrl_msg.txt");
  ctrlctr++;
  getctrl<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Ctrl Msg Queue size : "<<m_controlMessagesQueue.at(0).size()<<"\n";


    if (m_controlMessagesQueue.at (0).size () > 0)
    {

    	std::list<Ptr<LteControlMessage> >::iterator it = m_controlMessagesQueue.at(0).begin();
    	  /*for(unsigned int i = 0 ; i < m_controlMessagesQueue.at(0).size() ; i++)
    	  {

    		  std::advance(it,i);
    		  std::cout<<"Trying to type cast : "<<(*it)<<std::endl;
    		  getctrl<<"Trying to type cast : "<<(*it)<<"\n";
    		  if((*it)->GetMessageType()==LteControlMessage::DL_CQI)
    		  {
    		  Ptr<DlCqiLteControlMessage> msg = DynamicCast<DlCqiLteControlMessage>(*it);

    		  if(msg)
    		  {

    			  getctrl<<" Message type : "<<msg->GetDlCqi().m_cqiType<<"\n";
    			  if(msg->GetDlCqi().m_cqiType==CqiListElement_s::A30)
    			  {
    				  for(unsigned int i = 0 ; i < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.size() ; i++)
    					  for(unsigned int j = 0 ; j < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.size() ; j++)
    						  getctrl<<this<<" ....Time : "<<Simulator::Now().GetMilliSeconds()<<" Confirmation CTRL : "<<(uint16_t)msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.at(j)<<"\n";
    			  }
    			  else if(msg->GetDlCqi().m_cqiType==CqiListElement_s::P10)
    			  {
    				  for(unsigned int i = 0 ; i < msg->GetDlCqi().m_wbCqi.size() ; i++)
    					  getctrl<<this<<" ....Time : "<<Simulator::Now().GetMilliSeconds()<<" Confirmation CTRL : "<<(uint16_t)msg->GetDlCqi().m_wbCqi.at(i)<<"\n";
    			  }
    		  }
    		  }

    	  }*/
      getctrl<<"Looped\n";
      std::list<Ptr<LteControlMessage> > ret = m_controlMessagesQueue.at (0);
      ret = m_controlMessagesQueue.at (0);
      m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
      std::list<Ptr<LteControlMessage> > newlist;
      m_controlMessagesQueue.push_back (newlist);
      getctrl<<"Returning CTRL messages with size : "<<ret.size()<<"\n";
      return ret;
    }
  else
    {
      m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
      std::list<Ptr<LteControlMessage> > newlist;
      m_controlMessagesQueue.push_back (newlist);
      std::list<Ptr<LteControlMessage> > emptylist;
      return (emptylist);
    }
}

void
LtePhy::EraseMsgQueue ()
{
	m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
	std::list<Ptr<LteControlMessage> > newlist;
	m_controlMessagesQueue.push_back (newlist);
}

void
LtePhy::DoSetCellId (uint16_t cellId)
{
  m_cellId = cellId;
  m_downlinkSpectrumPhy->SetCellId (cellId);
  m_uplinkSpectrumPhy->SetCellId (cellId);
}


} // namespace ns3
