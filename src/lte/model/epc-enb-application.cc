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
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 */


#include "epc-enb-application.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/uinteger.h"

#include "epc-gtpu-header.h"
#include "lte-radio-bearer-tag.h"

#include <iostream>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcEnbApplication");

EpcEnbApplication::m_map EpcEnbApplication::createMap()
{
	std::cout<<"Creating Maps"<<std::endl;
	m_map m_imsiRnti;
	return m_imsiRnti;
}

EpcEnbApplication::m_map EpcEnbApplication::m_imsiRntiMultiMap(EpcEnbApplication::createMap());

EpcEnbApplication::m_multimap EpcEnbApplication::createMultiMap()
{
	std::cout<<"Creating Maps"<<std::endl;
	m_multimap m_imsiRntiMulti;
	return m_imsiRntiMulti;
}

EpcEnbApplication::m_multimap EpcEnbApplication::m_imsiRntiMultipleMap(EpcEnbApplication::createMultiMap());

TypeId
EpcEnbApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcEnbApplication")
    .SetParent<Object> ();
  return tid;
}

void
EpcEnbApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_lteSocket = 0;
  m_s1uSocket = 0;
  delete m_s1SapProvider;
}


EpcEnbApplication::EpcEnbApplication (Ptr<Socket> lteSocket, Ptr<Socket> s1uSocket, Ipv4Address sgwAddress)
  : m_lteSocket (lteSocket),
    m_s1uSocket (s1uSocket),    
    m_sgwAddress (sgwAddress),
    m_gtpuUdpPort (2152) // fixed by the standard
{
  NS_LOG_FUNCTION (this << lteSocket << s1uSocket << sgwAddress);
  std::ofstream mysocket;
    mysocket.open("//home//rajarajan//Documents//att_workspace//ltea//epcenbapp.txt");
    mysocket<<"EPC ENB App "<<this<<" LTE Socket = "<<lteSocket<<" S1U Socket = "<<s1uSocket<<" SGW Address = "<<sgwAddress<<"\n";
    mysocket.close();
  m_s1uSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromS1uSocket, this));
  m_lteSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromLteSocket, this));
  m_s1SapProvider = new MemberEpcEnbS1SapProvider<EpcEnbApplication> (this);
}


EpcEnbApplication::~EpcEnbApplication (void)
{
  NS_LOG_FUNCTION (this);
}


void 
EpcEnbApplication::SetS1SapUser (EpcEnbS1SapUser * s)
{
  m_s1SapUser = s;
}

  
EpcEnbS1SapProvider* 
EpcEnbApplication::GetS1SapProvider ()
{
  return m_s1SapProvider;
}

void 
EpcEnbApplication::ErabSetupRequest (uint32_t teid, uint64_t imsi, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << teid << imsi);
  // request the RRC to setup a radio bearer
  struct EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters params;
  params.bearer = bearer;
  params.teid = teid;
  std::map<uint64_t, uint16_t>::iterator it = m_imsiRntiMap.find (imsi);
  std::cout<<"IMSI = "<<imsi<<" RNTI = "<<it->second<<std::endl;
  std::cout<<"Just a hack"<<this<<std::endl;
  NS_ASSERT_MSG (it != m_imsiRntiMap.end (), "unknown IMSI");
  params.rnti = it->second;
  m_s1SapUser->DataRadioBearerSetupRequest (params);

}


void 
EpcEnbApplication::DoS1BearerSetupRequest (EpcEnbS1SapProvider::S1BearerSetupRequestParameters params)
{
  NS_LOG_FUNCTION (this);
  LteFlowId_t rbid (params.rnti, params.lcid);
  // side effect: create entries if not exist
  m_rbidTeidMap[rbid] = params.teid;
  m_teidRbidMap[params.teid] = rbid;
  std::cout<<" Time = "<<Simulator::Now().GetMilliSeconds()<<" S1 Bearer Setup"<<std::endl;
 // sleep(2);

  //SETUP RADIO BEARER (Debug) and UE Info:
}

// RAJARAJAN

void
EpcEnbApplication::DoInitialUeMessageCA (uint64_t imsi, uint16_t rnti)
{
	NS_LOG_FUNCTION (this);
	  // side effect: create entry if not exist
//	  EpcEnbApplication::m_imsiRntiMultiMap[imsi] = rnti;
	  m_imsiRntiMultipleMap.insert(std::pair<uint64_t, uint16_t>(imsi, rnti));
	  std::cout<<"InitialUeMessage : IMSI = "<<imsi<<" RNTI = "<<rnti<<std::endl;
	 // std::cout<<"InitialUeMessage , Another hack : "<<&EpcEnbApplication::m_imsiRntiMultiMap<<" and IMSI verification : " << EpcEnbApplication::m_imsiRntiMultiMap[0]<<std::endl;

	  std::cout<<std::endl;
}

// --RAJARAJAN--

// RAJARAJAN

void
EpcEnbApplication::ErabSetupRequestCA (uint32_t teid, uint64_t imsi, EpsBearer bearer, uint8_t maxCCs)
{
	static int index = 0;
	NS_LOG_FUNCTION (this << teid << imsi);
	  // request the RRC to setup a radio bearer

	  struct EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters params;
	  params.bearer = bearer;
	  params.teid = teid;
	  std::pair<std::multimap<uint64_t, uint16_t >::iterator , std::multimap<uint64_t , uint16_t >::iterator > m_imsiRntiMap;
	  NS_ASSERT_MSG(m_imsiRntiMultipleMap.count(imsi) != 0, "no RNTI found for this IMSI, did you attach it before?");
	  m_imsiRntiMap = m_imsiRntiMultipleMap.equal_range(imsi);
	  std::multimap<uint64_t , uint16_t >::iterator it = m_imsiRntiMap.first;
	  for(int k = 0 ; k < (index%maxCCs) ; k++,it++)
		  std::cout<<"Debug : "<<(*it).second<<std::endl;
	  params.rnti = (*it).second;
	  std::cout<<"ErabSetupCA : IMSI = "<<imsi<<" RNTI = "<<(*it).second<<std::endl;
//	  NS_ASSERT_MSG (it != EpcEnbApplication::m_imsiRntiMultiMap.end (), "unknown IMSI");

	  m_s1SapUser->DataRadioBearerSetupRequest (params);
	  index++;
}

// --RAJARAJAN--

void 
EpcEnbApplication::DoInitialUeMessage (uint64_t imsi, uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
  // side effect: create entry if not exist
  std::cout<<"Just a hack"<<this<<std::endl;
  m_imsiRntiMap[imsi] = rnti;
  std::cout<<"InitialUeMessage : IMSI = "<<imsi<<" RNTI = "<<rnti<<std::endl;
}

void 
EpcEnbApplication::RecvFromLteSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);  
  NS_ASSERT (socket == m_lteSocket);
  Ptr<Packet> packet = socket->Recv ();

  // workaround for bug 231 https://www.nsnam.org/bugzilla/show_bug.cgi?id=231
  SocketAddressTag satag;
  packet->RemovePacketTag (satag);

  LteRadioBearerTag tag;
  bool found = packet->RemovePacketTag (tag);
  NS_ASSERT (found);
  LteFlowId_t flowId;
  flowId.m_rnti = tag.GetRnti ();
  flowId.m_lcId = tag.GetLcid ();
  NS_LOG_LOGIC ("received packet with RNTI=" << flowId.m_rnti << ", LCID=" << (uint16_t)  flowId.m_lcId);
  std::map<LteFlowId_t, uint32_t>::iterator it = m_rbidTeidMap.find (flowId);
  NS_ASSERT (it != m_rbidTeidMap.end ());
  uint32_t teid = it->second;
  SendToS1uSocket (packet, teid);
}

void 
EpcEnbApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);  
  NS_ASSERT (socket == m_s1uSocket);
  Ptr<Packet> packet = socket->Recv ();

  std::ofstream mysocket;
  mysocket.open("//home//rajarajan//Documents//att_workspace//ltea//SocketHandler.txt");
  mysocket<<"Receiving packet from S1u socket: "<<packet<<"\n";
  mysocket.close();

  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);
  uint32_t teid = gtpu.GetTeid ();
  std::map<uint32_t, LteFlowId_t>::iterator it = m_teidRbidMap.find (teid);

  NS_ASSERT (it != m_teidRbidMap.end ());

  // workaround for bug 231 https://www.nsnam.org/bugzilla/show_bug.cgi?id=231
  SocketAddressTag tag;
  packet->RemovePacketTag (tag);
  
  SendToLteSocket (packet, it->second.m_rnti, it->second.m_lcId);
}

void 
EpcEnbApplication::SendToLteSocket (Ptr<Packet> packet, uint16_t rnti, uint8_t lcid)
{
  NS_LOG_FUNCTION (this << packet << rnti << (uint16_t) lcid);  
  LteRadioBearerTag tag (rnti, lcid);
  packet->AddPacketTag (tag);
  int sentBytes = m_lteSocket->Send (packet);
  NS_ASSERT (sentBytes > 0);
}


void 
EpcEnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid);  
  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  // From 3GPP TS 29.281 v10.0.0 Section 5.1
  // Length of the payload + the non obligatory GTP-U header
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);  
  packet->AddHeader (gtpu);
  uint32_t flags = 0;
  m_s1uSocket->SendTo (packet, flags, InetSocketAddress(m_sgwAddress, m_gtpuUdpPort));
}


}; // namespace ns3
