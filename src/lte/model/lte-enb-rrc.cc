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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 *         Marco Miozzo <mmiozzo@cttc.es>
 *         Manuel Requena <manuel.requena@cttc.es>
 */

#include <iostream>
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/pointer.h"
#include "ns3/object-map.h"
#include "ns3/object-factory.h"
#include "ns3/simulator.h"

#include "ns3/lte-enb-rrc.h"
#include "ns3/lte-enb-net-device.h"
#include "ns3/lte-rlc.h"
#include "ns3/lte-pdcp.h"
#include "ns3/lte-pdcp-sap.h"
#include "ns3/lte-radio-bearer-info.h"
#include "lte-radio-bearer-tag.h"
#include "ff-mac-csched-sap.h"
#include "epc-enb-s1-sap.h"

#include "lte-rlc.h"
#include "lte-rlc-um.h"
#include "lte-rlc-am.h"

#include <ns3/simulator.h>

// WILD HACK for UE-RRC direct communications
#include <ns3/node-list.h>
#include <ns3/node.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/lte-ue-rrc.h>

#include <iostream>
#include <fstream>

NS_LOG_COMPONENT_DEFINE ("LteEnbRrc");

namespace ns3 {


// ///////////////////////////
// CMAC SAP forwarder
// ///////////////////////////

class EnbRrcMemberLteEnbCmacSapUser : public LteEnbCmacSapUser
{
public:
  EnbRrcMemberLteEnbCmacSapUser (LteEnbRrc* rrc);

  virtual void NotifyLcConfigResult (uint16_t rnti, uint8_t lcid, bool success);
  virtual void RrcConfigurationUpdateInd (UeConfig params);

private:
  LteEnbRrc* m_rrc;
};

EnbRrcMemberLteEnbCmacSapUser::EnbRrcMemberLteEnbCmacSapUser (LteEnbRrc* rrc)
  : m_rrc (rrc)
{
}

void
EnbRrcMemberLteEnbCmacSapUser::NotifyLcConfigResult (uint16_t rnti, uint8_t lcid, bool success)
{
  m_rrc->DoNotifyLcConfigResult (rnti, lcid, success);
}

void
EnbRrcMemberLteEnbCmacSapUser::RrcConfigurationUpdateInd (UeConfig params)
{
  m_rrc->DoRrcConfigurationUpdateInd (params);
}


///////////////////////////////////////////
// UeInfo 
/////////////////////////////////////////// m_ueRrc


NS_OBJECT_ENSURE_REGISTERED (UeInfo);

UeInfo::UeInfo (void)
  : m_lastAllocatedId (0)
{
  m_imsi = 0;
  m_srsConfigurationIndex = 0;
  m_transmissionMode = 0;
}


UeInfo::~UeInfo (void)
{
  // Nothing to do here
}

TypeId UeInfo::GetTypeId (void)
{
  static TypeId  tid = TypeId ("ns3::UeInfo")
    .SetParent<Object> ()
    .AddConstructor<UeInfo> ()
    .AddAttribute ("RadioBearerMap", "List of UE RadioBearerInfo by LCID.",
                   ObjectMapValue (),
                   MakeObjectMapAccessor (&UeInfo::m_rbMap),
                   MakeObjectMapChecker<LteRadioBearerInfo> ())
  ;
  return tid;
}

uint64_t
UeInfo::GetImsi (void)
{
  return m_imsi;
}

void
UeInfo::SetImsi (uint64_t imsi)
{
  m_imsi = imsi;
}

uint16_t
UeInfo::GetSrsConfigurationIndex (void)
{
  return m_srsConfigurationIndex;
}

void
UeInfo::SetSrsConfigurationIndex (uint16_t srsConfIndex)
{
  m_srsConfigurationIndex = srsConfIndex;
}

uint8_t
UeInfo::GetTransmissionMode (void)
{
  return m_transmissionMode;
}

void
UeInfo::SetTransmissionMode (uint8_t txMode)
{
  m_transmissionMode = txMode;
}

uint8_t
UeInfo::AddRadioBearer (Ptr<LteRadioBearerInfo> rbi)
{
  NS_LOG_FUNCTION (this);
  for (uint8_t lcid = m_lastAllocatedId; lcid != m_lastAllocatedId - 1; ++lcid)
    {
      if (lcid != 0)
        {
          if (m_rbMap.find (lcid) == m_rbMap.end ())
            {
              m_rbMap.insert (std::pair<uint8_t, Ptr<LteRadioBearerInfo> > (lcid, rbi));
              m_lastAllocatedId = lcid;
              return lcid;
            }
        }
    }
  NS_LOG_WARN ("no more logical channel ids available");
  return 0;
}

Ptr<LteRadioBearerInfo>
UeInfo::GetRadioBearer (uint8_t lcid)
{
  NS_LOG_FUNCTION (this << (uint32_t) lcid);
  NS_ASSERT (0 != lcid);
  std::map<uint8_t, Ptr<LteRadioBearerInfo> >::iterator it = m_rbMap.find (lcid);  
  NS_ABORT_IF (it == m_rbMap.end ());
  return it->second;
}


void
UeInfo::RemoveRadioBearer (uint8_t lcid)
{
  NS_LOG_FUNCTION (this << (uint32_t) lcid);
  std::map <uint8_t, Ptr<LteRadioBearerInfo> >::iterator it = m_rbMap.find (lcid);
  NS_ASSERT_MSG (it != m_rbMap.end (), "request to remove radio bearer with unknown lcid " << lcid);
  m_rbMap.erase (it);
}
  
// ///////////////////////////
// eNB RRC methods
// ///////////////////////////

NS_OBJECT_ENSURE_REGISTERED (LteEnbRrc);

LteEnbRrc::LteEnbRrc ()
  : m_x2SapProvider (0),
    m_cmacSapProvider (0),
    m_ffMacSchedSapProvider (0),
    m_macSapProvider (0),
    m_s1SapProvider (0),
    m_cphySapProvider (0),
    m_configured (false),
    m_lastAllocatedRnti (0),
    m_srsCurrentPeriodicityId (0),
    m_lastAllocatedConfigurationIndex (0),
    m_reconfigureUes (false)
{
  NS_LOG_FUNCTION (this);
  m_cmacSapUser = new EnbRrcMemberLteEnbCmacSapUser (this);
  m_pdcpSapUser = new LtePdcpSpecificLtePdcpSapUser<LteEnbRrc> (this);
  m_x2SapUser = new EpcX2SpecificEpcX2SapUser<LteEnbRrc> (this);
  m_s1SapUser = new MemberEpcEnbS1SapUser<LteEnbRrc> (this);
  m_cphySapUser = new MemberLteEnbCphySapUser<LteEnbRrc> (this);
}


LteEnbRrc::~LteEnbRrc ()
{
  NS_LOG_FUNCTION (this);
}


void
LteEnbRrc::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_cmacSapUser;
  delete m_pdcpSapUser;
  delete m_x2SapUser;
  delete m_s1SapUser;
  delete m_cphySapUser;
}

TypeId
LteEnbRrc::GetTypeId (void)
{
  NS_LOG_FUNCTION ("LteEnbRrc::GetTypeId");
  static TypeId tid = TypeId ("ns3::LteEnbRrc")
    .SetParent<Object> ()
    .AddConstructor<LteEnbRrc> ()
    .AddAttribute ("UeMap", "List of UE Info by C-RNTI.",
                   ObjectMapValue (),
                   MakeObjectMapAccessor (&LteEnbRrc::m_ueMap),
                   MakeObjectMapChecker<UeInfo> ())
    .AddAttribute ("DefaultTransmissionMode",
                  "The default UEs' transmission mode (0: SISO)",
                  UintegerValue (0),  // default tx-mode
                  MakeUintegerAccessor (&LteEnbRrc::m_defaultTransmissionMode),
                  MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("EpsBearerToRlcMapping", 
                   "Specify which type of RLC will be used for each type of EPS bearer. ",
                   EnumValue (RLC_SM_ALWAYS),
                   MakeEnumAccessor (&LteEnbRrc::m_epsBearerToRlcMapping),
                   MakeEnumChecker (RLC_SM_ALWAYS, "RlcSmAlways",
                                    RLC_UM_ALWAYS, "RlcUmAlways",
                                    RLC_AM_ALWAYS, "RlcAmAlways",
                                    PER_BASED,     "PacketErrorRateBased"))             
  ;
  return tid;
}

uint16_t
LteEnbRrc::GetLastAllocatedRnti () const
{
  NS_LOG_FUNCTION (this);
  return m_lastAllocatedRnti;
}
std::map<uint16_t,Ptr<UeInfo> > LteEnbRrc::GetUeMap (void) const
{
  return m_ueMap;
}

void LteEnbRrc::SetUeMap (std::map<uint16_t,Ptr<UeInfo> > ueMap)
{
  this->m_ueMap = ueMap;
}


void
LteEnbRrc::SetLastAllocatedRnti (uint16_t lastAllocatedRnti)
{
  NS_LOG_FUNCTION (this << lastAllocatedRnti);
  m_lastAllocatedRnti = lastAllocatedRnti;
}


void
LteEnbRrc::SetLteEnbCphySapProvider (LteEnbCphySapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_cphySapProvider = s;
}

LteEnbCphySapUser*
LteEnbRrc::GetLteEnbCphySapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_cphySapUser;
}

void
LteEnbRrc::SetEpcX2SapProvider (EpcX2SapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_x2SapProvider = s;
}

EpcX2SapUser*
LteEnbRrc::GetEpcX2SapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_x2SapUser;
}


void
LteEnbRrc::SetLteEnbCmacSapProvider (LteEnbCmacSapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_cmacSapProvider = s;
}

LteEnbCmacSapUser*
LteEnbRrc::GetLteEnbCmacSapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_cmacSapUser;
}

void
LteEnbRrc::SetFfMacSchedSapProvider (FfMacSchedSapProvider * s)
{
  NS_LOG_FUNCTION (this);
  m_ffMacSchedSapProvider = s;
}


void
LteEnbRrc::SetLteMacSapProvider (LteMacSapProvider * s)
{
  NS_LOG_FUNCTION (this);
  m_macSapProvider = s;
}

void 
LteEnbRrc::SetS1SapProvider (EpcEnbS1SapProvider * s)
{
  m_s1SapProvider = s;
}

  
EpcEnbS1SapUser* 
LteEnbRrc::GetS1SapUser ()
{
  return m_s1SapUser;
}

LtePdcpSapProvider* 
LteEnbRrc::GetLtePdcpSapProvider (uint16_t rnti, uint8_t lcid)
{
  std::cout<<" Time = "<<Simulator::Now().GetMilliSeconds()<<" Cell ID = "<<m_cellId<<" RNTI = "<<rnti<<std::endl;
  return GetUeInfo (rnti)->GetRadioBearer (lcid)->m_pdcp->GetLtePdcpSapProvider ();
}

void
LteEnbRrc::ConfigureCell (uint8_t ulBandwidth, uint8_t dlBandwidth, uint16_t ulEarfcn, uint16_t dlEarfcn, uint16_t cellId)
{
  NS_LOG_FUNCTION (this);
  std::cout<<" BEFORE : Cell ID = "<<cellId<<" and m_configured value is "<<m_configured<<" this pointer = "<<this<<std::endl;
  NS_ASSERT (!m_configured);
  m_cmacSapProvider->SetCellId (cellId);
  m_cmacSapProvider->ConfigureMac (ulBandwidth, dlBandwidth);
  m_cphySapProvider->SetBandwidth (ulBandwidth, dlBandwidth);
  m_cphySapProvider->SetEarfcn (ulEarfcn, dlEarfcn);
  m_cellId = cellId;
  m_cphySapProvider->SetCellId (cellId);
  m_configured = true;
  //std::cout<<" AFTER : Cell ID = "<<cellId<<" and m_configured value is "<<m_configured<<" this pointer = "<<this<<std::endl;
  //sleep(2);
}

// RAJARAJAN

void
LteEnbRrc::ConfigureCellCA (uint8_t ulBandwidth, uint8_t dlBandwidth, uint16_t ulEarfcn, uint16_t dlEarfcn, uint16_t cellId, uint8_t noOfCCs)
{
  NS_LOG_FUNCTION (this);
  //static int index = 0;
  std::cout<<" Cell ID = "<<cellId<<" and m_configured value is "<<m_configured<<" this pointer = "<<this<<std::endl;
  NS_ASSERT (!m_configured);
  m_cmacSapProvider->ConfigureMac (ulBandwidth, dlBandwidth);
  //WILD HACK

  //WILD HACK
  m_cphySapProvider->SetBandwidth (ulBandwidth, dlBandwidth);
  m_cphySapProvider->SetEarfcn (ulEarfcn, dlEarfcn);
  m_cellId = cellId;
  m_cphySapProvider->SetCellId (cellId);
  //if(index%noOfCCs == 0 && index!=0 )
	  m_configured = true;
  std::cout<<" AFTER : Cell ID = "<<cellId<<" and m_configured value is "<<m_configured<<" this pointer = "<<this<<std::endl;
  //index++;
  // RAJA: CRAZY HACK !
//  if(index%noOfCCs==0)
	//  index=0;
}

// --RAJARAJAN--

// RAJARAJAN

uint16_t
LteEnbRrc::DoRecvConnectionRequestCA (uint64_t imsi)
{
  NS_LOG_FUNCTION (this);
  // no Call Admission Control for now

  uint16_t rnti = AddUe ();
  std::cout<<"RECV CONNECTION REQUEST CA : IMSI = "<<imsi<<" and RNTI = "<<rnti<<std::endl;
  std::map<uint16_t, Ptr<UeInfo> >::iterator it = m_ueMap.find (rnti);
  NS_ASSERT_MSG (it != m_ueMap.end (), "RNTI " << rnti << " not found in eNB with cellId " << m_cellId);
  it->second->SetImsi (imsi);

  if (m_s1SapProvider != 0)
    {
	  std::cout<<"SAP PROVIDER?"<<std::endl;
      m_s1SapProvider->InitialUeMessageCA (imsi, rnti);
    }

  // send RRC connection setup
  LteUeConfig_t ueConfig;
  ueConfig.m_rnti = rnti;
  ueConfig.m_transmissionMode = (*it).second->GetTransmissionMode ();
  ueConfig.m_srsConfigurationIndex = (*it).second->GetSrsConfigurationIndex ();
  ueConfig.m_reconfigureFlag = false;
  Ptr<LteUeRrc> ueRrc = GetUeRrcByImsi (imsi);
  std::cout<<"GUESS WE HAVE CRACKED THE PROBLEM: "<<ueRrc<<" for IMSI: "<<imsi<<std::endl;
  ueRrc->DoRecvConnectionSetup (ueConfig);

  // configure MAC (and scheduler)
  LteEnbCmacSapProvider::UeConfig params;
  params.m_rnti = rnti;
  params.m_transmissionMode = (*it).second->GetTransmissionMode ();
  m_cmacSapProvider->UeUpdateConfigurationReq (params);


  // configure PHY
  m_cphySapProvider->SetTransmissionMode (rnti, (*it).second->GetTransmissionMode ());
  m_cphySapProvider->SetSrsConfigurationIndex (rnti, (*it).second->GetSrsConfigurationIndex ());

  return rnti;
}

// --RAJARAJAN-- m_imsiCounter multimap m_imsiRnti 9842221342

uint16_t
LteEnbRrc::DoRecvConnectionRequest (uint64_t imsi)
{
  NS_LOG_FUNCTION (this);
  // no Call Admission Control for now

  std::cout<<this<<"RECV CONNECTION REQUEST : IMSI = "<<imsi<<std::endl;
  uint16_t rnti = AddUe ();
  std::cout<<"RECV CONNECTION REQUEST : IMSI = "<<imsi<<" and RNTI = "<<rnti<<std::endl;

  std::map<uint16_t, Ptr<UeInfo> >::iterator it = m_ueMap.find (rnti);
  NS_ASSERT_MSG (it != m_ueMap.end (), "RNTI " << rnti << " not found in eNB with cellId " << m_cellId);
  it->second->SetImsi (imsi);

  if (m_s1SapProvider != 0)
    {
	  std::cout<<"SAP PROVIDER?"<<std::endl;
      m_s1SapProvider->InitialUeMessage (imsi, rnti);
    }

  // send RRC connection setup
  LteUeConfig_t ueConfig;
  ueConfig.m_rnti = rnti;
  ueConfig.m_transmissionMode = (*it).second->GetTransmissionMode ();
  ueConfig.m_srsConfigurationIndex = (*it).second->GetSrsConfigurationIndex ();
  ueConfig.m_reconfigureFlag = false;
  Ptr<LteUeRrc> ueRrc = GetUeRrcByImsi (imsi);
  ueRrc->DoRecvConnectionSetup (ueConfig);
  
  // configure MAC (and scheduler)
  LteEnbCmacSapProvider::UeConfig params;
  params.m_rnti = rnti;
  params.m_transmissionMode = (*it).second->GetTransmissionMode ();
  m_cmacSapProvider->UeUpdateConfigurationReq (params);
  

  // configure PHY
  m_cphySapProvider->SetTransmissionMode (rnti, (*it).second->GetTransmissionMode ());
  m_cphySapProvider->SetSrsConfigurationIndex (rnti, (*it).second->GetSrsConfigurationIndex ());

  return rnti;
}

void
LteEnbRrc::DoRecvConnectionSetupCompleted (uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbRrc::DoRecvConnectionReconfigurationCompleted (uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
}


uint16_t
LteEnbRrc::AddUe ()
{
  NS_LOG_FUNCTION (this);

  uint16_t rnti = CreateUeInfo (); // side effect: create UeInfo for this UE
  std::cout<<"RNTI = "<<rnti<<std::endl;
 // sleep(2);

  NS_ASSERT_MSG (rnti != 0, "CreateUeInfo returned RNTI==0");
  m_cmacSapProvider->AddUe (rnti);
  m_cphySapProvider->AddUe (rnti);
  return rnti;
}

void
LteEnbRrc::RemoveUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << (uint32_t) rnti);
  RemoveUeInfo (rnti);
  //NS_FATAL_ERROR ("missing RemoveUe method in CMAC SAP");
  m_cmacSapProvider->RemoveUe (rnti);
  m_cphySapProvider->RemoveUe (rnti);
}

TypeId
LteEnbRrc::GetRlcType (EpsBearer bearer)
{
  switch (m_epsBearerToRlcMapping)
    {
    case RLC_SM_ALWAYS:
      return LteRlcSm::GetTypeId ();
      break;

    case  RLC_UM_ALWAYS:
      return LteRlcUm::GetTypeId ();
      break;

    case RLC_AM_ALWAYS:
      return LteRlcAm::GetTypeId ();
      break;

    case PER_BASED:
      if (bearer.GetPacketErrorLossRate () > 1.0e-5)
        {
          return LteRlcUm::GetTypeId ();
        }
      else
        {
          return LteRlcAm::GetTypeId ();
        }
      break;

    default:
      return LteRlcSm::GetTypeId ();
      break;
    }
}

void 
LteEnbRrc::DoDataRadioBearerSetupRequest (EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters request)
{
  EpcEnbS1SapProvider::S1BearerSetupRequestParameters response;
  std::cout<<"RNTI in Data Radio Bearer Setup Request = "<<request.rnti<<std::endl;
 // sleep(2);
  for(std::map<uint16_t , Ptr<UeInfo> >::iterator it = m_ueMap.begin(); it!=m_ueMap.end();it++)
	  std::cout<<"Cell ID = "<<m_cellId<<" RNTI: "<<it->first<<" and UE Info: "<<it->second->GetImsi()<<std::endl;
  std::map<uint16_t, Ptr<UeInfo> >::iterator it = m_ueMap.find (request.rnti);
  NS_ASSERT_MSG (it != m_ueMap.end (), "unknown RNTI");
  response.rnti = request.rnti;
  response.lcid = SetupRadioBearer (response.rnti, request.bearer, request.teid);      
  NS_ASSERT_MSG (response.lcid > 0, "SetupRadioBearer() returned LCID == 0. Too many LCs?");
  response.teid = request.teid;
  if (m_s1SapProvider)
    {          
      m_s1SapProvider->S1BearerSetupRequest (response);
    }
}

uint8_t
LteEnbRrc::SetupRadioBearer (uint16_t rnti, EpsBearer bearer, uint32_t teid)
{
  NS_LOG_FUNCTION (this << (uint32_t) rnti);

  TypeId rlcTypeId = GetRlcType (bearer);
  Ptr<UeInfo> ueInfo = GetUeInfo (rnti);
  std::cout<<"SETUP RADIO BEARER (Debug): UeInfo : "<<(uint16_t)ueInfo->GetTransmissionMode()<<" RNTI = "<<rnti<<std::endl;
  // create RLC instance

  ObjectFactory rlcObjectFactory;
  rlcObjectFactory.SetTypeId (rlcTypeId);
  Ptr<LteRlc> rlc = rlcObjectFactory.Create ()->GetObject<LteRlc> ();
  rlc->SetLteMacSapProvider (m_macSapProvider);
  rlc->SetRnti (rnti);

  Ptr<LteRadioBearerInfo> rbInfo = CreateObject<LteRadioBearerInfo> ();
  rbInfo->m_epsBearer = bearer;
  rbInfo->m_teid = teid;
  rbInfo->m_rlc = rlc;
  uint8_t lcid = ueInfo->AddRadioBearer (rbInfo);
  if (lcid == 0)
    {
      NS_LOG_WARN ("cannot setup radio bearer");
      return 0;
    }
  rlc->SetLcId (lcid);

  // we need PDCP only for real RLC, i.e., RLC/UM or RLC/AM
  // if we are using RLC/SM we don't care of anything above RLC
  if (rlcTypeId != LteRlcSm::GetTypeId ())
    {
      Ptr<LtePdcp> pdcp = CreateObject<LtePdcp> ();
      pdcp->SetRnti (rnti);
      pdcp->SetLcId (lcid);
      pdcp->SetLtePdcpSapUser (m_pdcpSapUser);
      pdcp->SetLteRlcSapProvider (rlc->GetLteRlcSapProvider ());
      rlc->SetLteRlcSapUser (pdcp->GetLteRlcSapUser ());
      rbInfo->m_pdcp = pdcp;
    }
    
  LteEnbCmacSapProvider::LcInfo lcinfo;
  lcinfo.rnti = rnti;
  lcinfo.lcId = lcid;
  lcinfo.lcGroup = 0; // TBD
  lcinfo.qci = bearer.qci;
  lcinfo.isGbr = bearer.IsGbr ();
  lcinfo.mbrUl = bearer.gbrQosInfo.mbrUl;
  lcinfo.mbrDl = bearer.gbrQosInfo.mbrDl;
  lcinfo.gbrUl = bearer.gbrQosInfo.gbrUl;
  lcinfo.gbrDl = bearer.gbrQosInfo.gbrDl;
  m_cmacSapProvider->AddLc (lcinfo, rlc->GetLteMacSapUser ());

  GetUeRrcByRnti (rnti)->SetupRadioBearer (bearer, rlcTypeId, lcid);
  
  return lcid;
}

void
LteEnbRrc::ReleaseRadioBearer (uint16_t rnti, uint8_t lcId)
{
  NS_LOG_FUNCTION (this << (uint32_t) rnti << (uint32_t) lcId);
  Ptr<UeInfo> ueInfo = GetUeInfo (rnti);
  ueInfo->RemoveRadioBearer (lcId);
}



bool
LteEnbRrc::Send (Ptr<Packet> packet) // ->Send (
{
  NS_LOG_FUNCTION (this << packet);

  LteRadioBearerTag tag;
  bool found = packet->RemovePacketTag (tag);
  NS_ASSERT (found);
  
  LtePdcpSapProvider::TransmitRrcPduParameters params;
  params.rrcPdu = packet;
  params.rnti = tag.GetRnti ();
  params.lcid = tag.GetLcid ();
  LtePdcpSapProvider* pdcpSapProvider = GetLtePdcpSapProvider (tag.GetRnti (), tag.GetLcid ());
  pdcpSapProvider->TransmitRrcPdu (params);
  
  return true;
}

void 
LteEnbRrc::SetForwardUpCallback (Callback <void, Ptr<Packet> > cb)
{
  m_forwardUpCallback = cb;
}

// Do Notify Tx Opportunity
//
// User API
//
void
LteEnbRrc::SendHandoverRequest ( uint64_t imsi, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << imsi << cellId);
  NS_LOG_LOGIC ("Request to send HANDOVER REQUEST");

  /*for(std::map<uint16_t , Ptr<UeInfo> >::iterator it = m_ueMap.begin(); it!=m_ueMap.end();it++)
  	  std::cout<<"RNTI: "<<it->first<<" and UE Info: "<<it->second->GetImsi()<<std::endl;*/




  std::cout<<" Cell ID : "<<m_cellId<<std::endl;
  uint16_t rnti;
  std::map<uint16_t, Ptr<UeInfo> >::iterator it;
  for( it = m_ueMap.begin(); it!=m_ueMap.end();it++)
  {
	  std::cout<<" UE : "<<it->first<<" Cross check IMSI : "<<it->second->GetImsi()<<std::endl;
	  if(imsi==it->second->GetImsi())
	  {
		  rnti = it->first;
		  break;
	  }

  }
  //std::map<uint16_t, Ptr<UeInfo> >::iterator it = m_ueMap.find (rnti);
  NS_ASSERT_MSG (it != m_ueMap.end (), "RNTI " << rnti << " not found in eNB with cellId " << m_cellId);
  RemoveUe(rnti);
  EpcX2SapProvider::HandoverRequestParams params;
  //params.oldEnbUeX2apId = rnti;
  params.oldEnbUeX2apId = imsi;
  params.cause          = EpcX2SapProvider::HandoverDesirableForRadioReason;
  params.sourceCellId   = m_cellId;
  params.targetCellId   = cellId;
  params.ueAggregateMaxBitRateDownlink = 200 * 1000;
  params.ueAggregateMaxBitRateUplink = 100 * 1000;

  //sleep(2); DO HANDOVER REQUEST

  std::string rrcData ("abcdefghijklmnopqrstuvwxyz");
  params.rrcContext = Create<Packet> ((uint8_t const *) rrcData.data (), rrcData.length ());

  NS_LOG_LOGIC ("oldEnbUeX2apId = " << params.oldEnbUeX2apId);
  NS_LOG_LOGIC ("sourceCellId = " << params.sourceCellId);
  NS_LOG_LOGIC ("targetCellId = " << params.targetCellId);
  NS_LOG_LOGIC ("rrcContext   = " << params.rrcContext << " : " << rrcData);


  std::cout<<" Time ENB RRC SEND HO: "<<Simulator::Now().GetMilliSeconds()<<" SEND : RNTI = "<<params.oldEnbUeX2apId<<" Source cell ID = "<<params.sourceCellId<<" Target cell ID = "<<params.targetCellId<<std::endl;
  m_x2SapProvider->SendHandoverRequest (params);
  //RemoveUe(rnti);
}


//
// X2 User SAP
//
void
LteEnbRrc::DoRecvHandoverRequest (EpcX2SapUser::HandoverRequestParams params)
{

  NS_LOG_FUNCTION (this);

  NS_LOG_LOGIC ("Recv X2 message: HANDOVER REQUEST");

  NS_LOG_LOGIC ("oldEnbUeX2apId = " << params.oldEnbUeX2apId);
  NS_LOG_LOGIC ("sourceCellId = " << params.sourceCellId);
  NS_LOG_LOGIC ("targetCellId = " << params.targetCellId);
  

  // this crashes, apparently EpcX2 is not correctly filling in the params yet
  //  uint8_t rrcData [100];
  // params.rrcContext->CopyData (rrcData, params.rrcContext->GetSize ());
  // NS_LOG_LOGIC ("rrcContext   = " << rrcData); and Cell ID =

  std::cout<<"Alert: Receive Hadnover Rewq"<<std::endl;
  //sleep(3);

  uint16_t rnti = AddUe ();
  //std::cout<<"RecvHandover Req. RNTI = "<<rnti<<std::endl;
 // sleep(2); // Just cracking the pot

  static std::ofstream logs;
  static int counter = 0;
  if(counter == 0)
	  logs.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//recvhandover.txt");
  counter++;
  logs<<" RNTI = "<<params.oldEnbUeX2apId<<" Source cell ID = "<<params.sourceCellId<<" Target cell ID = "<<params.targetCellId<<"\n";
  std::cout<<this<<" recvhandover, Time = "<<Simulator::Now().GetMilliSeconds()<<" RECV : IMSI = "<<params.oldEnbUeX2apId<<" RNTI = "<<rnti<<" cell ID = "<<m_cellId<<std::endl;

  //sleep(2);
/*  Ptr<LteUeRrc> ueRrc = params.ueRrc;
  ueRrc->DoRecvConnectionReconfigurationWithMobilityControlInfo(params.targetCellId,rnti);
*/

  Ptr<LteUeNetDevice> ueDev;
  NodeList::Iterator listEnd = NodeList::End ();
  bool found = false;
  for (NodeList::Iterator i = NodeList::Begin (); i != listEnd; i++)
  {
	  Ptr<Node> node = *i;
	  int nDevs = node->GetNDevices ();
	  int j;

	  for (j = 0; j < nDevs; j++)
	   {
	            ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
	            if (!ueDev)
	              {
	                continue;
	              }
	            else
	              {
	            	if ( params.targetCellId == m_cellId && params.oldEnbUeX2apId == ueDev->GetImsi())
	                {
	              	  std::cout<<ueDev<<" UE IMSI : "<<ueDev->GetImsi()<<" RNTI : "<<rnti<<" Target Cell ID : "<<m_cellId<<"\n";
	              	  //sleep(2);
                      found = true;
	                  break;
	                }
	              }
	   }

	   if(j < nDevs)
	      break;
  }

  if(ueDev)
  {
	  Ptr<LteUeRrc> ueRrc = ueDev->GetRrc();
	  std::cout<<" RecvConnectionReconfigurationWithMobilityControlInfo : IMSI = "<<ueDev->GetImsi()<<std::endl;

	  ueRrc->DoRecvConnectionReconfigurationWithMobilityControlInfo(params.targetCellId,rnti);
  }

  //--RAJARAJAN-- RECV HANDOVER

  for (std::vector <EpcX2Sap::ErabToBeSetupItem>::iterator it = params.bearers.begin ();
       it != params.bearers.end ();
       ++it)
    {
      SetupRadioBearer (rnti, it->erabLevelQosParameters, it->gtpTeid);
    }

  NS_LOG_LOGIC ("Send X2 message: HANDOVER REQUEST ACK");

  EpcX2SapProvider::HandoverRequestAckParams ackParams;
  ackParams.oldEnbUeX2apId = params.oldEnbUeX2apId;
  ackParams.newEnbUeX2apId = params.oldEnbUeX2apId + 100;
  ackParams.sourceCellId = params.sourceCellId;
  ackParams.targetCellId = params.targetCellId;

  NS_LOG_LOGIC ("oldEnbUeX2apId = " << ackParams.oldEnbUeX2apId);
  NS_LOG_LOGIC ("newEnbUeX2apId = " << ackParams.newEnbUeX2apId);
  NS_LOG_LOGIC ("sourceCellId = " << ackParams.sourceCellId);
  NS_LOG_LOGIC ("targetCellId = " << ackParams.targetCellId);

// DoSendHandoverRequestAck

  m_x2SapProvider->SendHandoverRequestAck (ackParams);
}

void
LteEnbRrc::DoRecvHandoverRequestAck (EpcX2SapUser::HandoverRequestAckParams params)
{
  NS_LOG_FUNCTION (this);
  
  NS_LOG_LOGIC ("Recv X2 message: HANDOVER REQUEST ACK");
  
  NS_LOG_LOGIC ("oldEnbUeX2apId = " << params.oldEnbUeX2apId);
  NS_LOG_LOGIC ("newEnbUeX2apId = " << params.newEnbUeX2apId);
  NS_LOG_LOGIC ("sourceCellId = " << params.sourceCellId);
  NS_LOG_LOGIC ("targetCellId = " << params.targetCellId);
}


void
LteEnbRrc::DoReceiveRrcPdu (LtePdcpSapUser::ReceiveRrcPduParameters params)
{
  NS_LOG_FUNCTION (this);
  // this tag is needed by the EpcEnbApplication to determine the S1 bearer that corresponds to this radio bearer
  LteRadioBearerTag tag;
  tag.SetRnti (params.rnti);
  tag.SetLcid (params.lcid);
  params.rrcPdu->AddPacketTag (tag);
 // std::cout<<"LTE eNB RRC - DoRecvRrcPdu"<<std::endl;
 //   sleep(3);
  m_forwardUpCallback (params.rrcPdu);
}



void
LteEnbRrc::DoNotifyLcConfigResult (uint16_t rnti, uint8_t lcid, bool success)
{
  NS_LOG_FUNCTION (this << (uint32_t) rnti);
  NS_FATAL_ERROR ("not implemented");
}



// /////////////////////////////////////////
// management of multiple UE info instances
// /////////////////////////////////////////

// from 3GPP TS 36.213 table 8.2-1 UE Specific SRS Periodicity
#define SRS_ENTRIES 9
uint16_t g_srsPeriodicity[SRS_ENTRIES] = {0, 2, 5, 10, 20, 40, 80, 160, 320};
uint16_t g_srsCiLow[SRS_ENTRIES] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
uint16_t g_srsCiHigh[SRS_ENTRIES] = {0, 1, 6, 16, 36, 76, 156, 316, 636};

void
LteEnbRrc::SetCellId (uint16_t cellId)
{
  m_cellId = cellId;
}

uint16_t
LteEnbRrc::CreateUeInfo ()
{
  NS_LOG_FUNCTION (this);
  std::cout<<"LAST ALLOCATED RNTI = "<<m_lastAllocatedRnti<<std::endl;
  for (uint16_t rnti = m_lastAllocatedRnti; rnti != m_lastAllocatedRnti - 1; ++rnti)
    {
      if (rnti != 0)
        {
          if (m_ueMap.find (rnti) == m_ueMap.end ())
            {
              m_lastAllocatedRnti = rnti;
              std::cout<<"Last Allocation RNTI = "<<m_lastAllocatedRnti<<std::endl;
              Ptr<UeInfo> ueInfo = CreateObject<UeInfo> ();
              ueInfo->SetSrsConfigurationIndex (GetNewSrsConfigurationIndex ());
              ueInfo->SetTransmissionMode (m_defaultTransmissionMode);
              m_ueMap.insert (std::pair<uint16_t, Ptr<UeInfo> > (rnti, ueInfo));
              std::cout<<" New UE RNTI = "<<rnti<<" Cell ID = "<<m_cellId<<std::endl;
           //   sleep(2);
              NS_LOG_DEBUG (this << " New UE RNTI " << rnti << " cellId " << m_cellId << " srs CI " << ueInfo->GetSrsConfigurationIndex ());
              return rnti;
            }
        }
    }
    
  return 0;
}

uint16_t
LteEnbRrc::GetNewSrsConfigurationIndex ()
{
  NS_LOG_FUNCTION (this << m_ueSrsConfigurationIndexSet.size ());
  // SRS SRS IS TRUE
  std::cout<<"Just cracking the pot - LAST_ALLOCATED_CONFIGURATION_INDEX = "<<m_srsCurrentPeriodicityId<<std::endl;
  if (m_srsCurrentPeriodicityId==0)
    {
      // no UEs -> init SRS IS TRUE
      m_ueSrsConfigurationIndexSet.insert (0);
      m_lastAllocatedConfigurationIndex = 0;
      m_srsCurrentPeriodicityId++;
      return 0;
    }
  NS_ASSERT (m_srsCurrentPeriodicityId < SRS_ENTRIES);
  NS_LOG_DEBUG (this << " SRS p " << g_srsPeriodicity[m_srsCurrentPeriodicityId] << " set " << m_ueSrsConfigurationIndexSet.size ());
  if (m_ueSrsConfigurationIndexSet.size () == g_srsPeriodicity[m_srsCurrentPeriodicityId])
    {
//       NS_LOG_DEBUG (this << " SRS reconfigure CIs " << g_srsPeriodicity[m_srsCurrentPeriodicityId] << " to " << g_srsPeriodicity[m_srsCurrentPeriodicityId+1] << " at " << Simulator::Now ());
      // increase the current periocity for having enough CIs PHY device :
      m_ueSrsConfigurationIndexSet.clear ();
      m_srsCurrentPeriodicityId++;
      NS_ASSERT (m_srsCurrentPeriodicityId < SRS_ENTRIES);
      // update all the UE's CI RNTI =
      uint16_t srcCi = g_srsCiLow[m_srsCurrentPeriodicityId];
      std::map<uint16_t, Ptr<UeInfo> >::iterator it;

      for (it = m_ueMap.begin (); it != m_ueMap.end (); it++)
        {

          (*it).second->SetSrsConfigurationIndex (srcCi);
          m_ueSrsConfigurationIndexSet.insert (srcCi);
          m_lastAllocatedConfigurationIndex = srcCi;
          srcCi++;
          // send update to peer RRC
          LteUeConfig_t ueConfig;
          ueConfig.m_rnti = (*it).first;
          ueConfig.m_transmissionMode = (*it).second->GetTransmissionMode ();
          ueConfig.m_srsConfigurationIndex = (*it).second->GetSrsConfigurationIndex ();
          ueConfig.m_reconfigureFlag = true;
          std::cout<<" UE MAP CHECK Time = "<<Simulator::Now().GetMilliSeconds()<<" Cell ID = "<<m_cellId<<" RNTI = "<<ueConfig.m_rnti<<" CI = "<<ueConfig.m_srsConfigurationIndex<<std::endl;
      //   sleep(2);
          NS_LOG_DEBUG (this << "\t rnti "<<ueConfig.m_rnti<< " CI " << ueConfig.m_srsConfigurationIndex);
          PropagateRrcConnectionReconfiguration (ueConfig);
        }
      m_ueSrsConfigurationIndexSet.insert (m_lastAllocatedConfigurationIndex + 1);
      m_lastAllocatedConfigurationIndex++;
    }
  else
    {
      // find a CI from the available ones // Cross-check SRS
      std::set<uint16_t>::reverse_iterator rit = m_ueSrsConfigurationIndexSet.rbegin ();
      NS_LOG_DEBUG (this << " lower bound " << (*rit) << " of " << g_srsCiHigh[m_srsCurrentPeriodicityId]);
      if ((*rit) <= g_srsCiHigh[m_srsCurrentPeriodicityId])
        {
          // got it from the upper bound
          m_lastAllocatedConfigurationIndex = (*rit) + 1;
          m_ueSrsConfigurationIndexSet.insert (m_lastAllocatedConfigurationIndex);
        }
      else
        {
          // look for released ones
          for (uint16_t srcCi = g_srsCiLow[m_srsCurrentPeriodicityId]; srcCi < g_srsCiHigh[m_srsCurrentPeriodicityId]; srcCi++) 
            {
              std::set<uint16_t>::iterator it = m_ueSrsConfigurationIndexSet.find (srcCi);
              if (it==m_ueSrsConfigurationIndexSet.end ())
                {
                  m_lastAllocatedConfigurationIndex = srcCi;
                  m_ueSrsConfigurationIndexSet.insert (srcCi);
                  break;
                }
            }
        } 
    }
  std::cout<<"M_LASTALLOCATEDCONFIGURATIONINDEX = "<<m_lastAllocatedConfigurationIndex<<std::endl;
  return m_lastAllocatedConfigurationIndex;
  
}


void
LteEnbRrc::RemoveSrsConfigurationIndex (uint16_t srcCi)
{
  NS_LOG_FUNCTION (this << srcCi);
  //NS_FATAL_ERROR ("I though this method was unused so far...");
  std::set<uint16_t>::iterator it = m_ueSrsConfigurationIndexSet.find (srcCi);
  NS_ASSERT_MSG (it != m_ueSrsConfigurationIndexSet.end (), "request to remove unkwown SRS CI " << srcCi);
  m_ueSrsConfigurationIndexSet.erase (it);
  std::cout<<this<<" SRS Current Periodicity ID = "<<m_srsCurrentPeriodicityId<<std::endl;
  //NS_ASSERT (m_srsCurrentPeriodicityId > 1);
  NS_ASSERT (m_srsCurrentPeriodicityId >= 1);
  if (m_ueSrsConfigurationIndexSet.size () < g_srsPeriodicity[m_srsCurrentPeriodicityId - 1])
    {
      // reduce the periodicity
      m_ueSrsConfigurationIndexSet.clear ();
      m_srsCurrentPeriodicityId--;
      if (m_srsCurrentPeriodicityId==0)
        {
          // no active users : renitialize structures
          m_lastAllocatedConfigurationIndex = 0;
        }



      else
        {
          // update all the UE's CI
          uint16_t srcCi = g_srsCiLow[m_srsCurrentPeriodicityId];
          std::map<uint16_t, Ptr<UeInfo> >::iterator it;
          for (it = m_ueMap.begin (); it != m_ueMap.end (); it++)
            {
              (*it).second->SetSrsConfigurationIndex (srcCi);
              m_ueSrsConfigurationIndexSet.insert (srcCi);
              m_lastAllocatedConfigurationIndex = srcCi;
              srcCi++;
              // send update to peer RRC
              LteUeConfig_t ueConfig;
              ueConfig.m_rnti = (*it).first;
   /*           if(Simulator::Now().GetMilliSeconds()>50)
              {
            	  std::cout<<" Time = "<<Simulator::Now().GetMilliSeconds()<<" Cross-check SRS for RNTI = "<<ueConfig.m_rnti<<" Cell ID = "<<m_cellId<<std::endl;
            	  sleep(2);
              } */
              ueConfig.m_transmissionMode = (*it).second->GetTransmissionMode ();
              ueConfig.m_srsConfigurationIndex = (*it).second->GetSrsConfigurationIndex ();
              ueConfig.m_reconfigureFlag = false;
              NS_LOG_DEBUG (this << "\t rnti "<<ueConfig.m_rnti<< " CI " << ueConfig.m_srsConfigurationIndex);

              // avoid multiple reconfiguration during initialization
              if (Simulator::Now ().GetNanoSeconds () != 0)
                {                 
                  PropagateRrcConnectionReconfiguration (ueConfig);
                }
            }
        }
    }
}

Ptr<UeInfo>
LteEnbRrc::GetUeInfo (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << (uint32_t) rnti);
  NS_ASSERT (0 != rnti);
  std::cout<<"Cell ID = "<<m_cellId<<" RNTI = "<<rnti<<" UE Map size = "<<m_ueMap.size()<<std::endl;
  std::map<uint16_t, Ptr<UeInfo> >::iterator it = m_ueMap.find (rnti);  
  NS_ABORT_IF (it == m_ueMap.end ());
  return it->second;
}

void
LteEnbRrc::RemoveUeInfo (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << (uint32_t) rnti);
  //NS_FATAL_ERROR ("I though this method was unused so far...");
  std::map <uint16_t, Ptr<UeInfo> >::iterator it = m_ueMap.find (rnti);
  NS_ASSERT_MSG (it != m_ueMap.end (), "request to remove UE info with unknown rnti " << rnti);
  RemoveSrsConfigurationIndex ((*it).second->GetSrsConfigurationIndex ());
  m_ueMap.erase (it);
  // remove SRS configuration index
}

void
LteEnbRrc::RemoveUe (Ptr<LteUeNetDevice> ueDev)
{
	std::map<uint16_t, Ptr<UeInfo> >::iterator it;
	uint16_t rnti;
	for (it = m_ueMap.begin (); it != m_ueMap.end (); it++)
	{
		  if((*it).second->GetImsi()==ueDev->GetImsi())
		  {
			  rnti = it->first;
			  break;
		  }
	}
	RemoveUe(rnti);
}

void
LteEnbRrc::DoRrcConfigurationUpdateInd (LteEnbCmacSapUser::UeConfig cmacParams)
{
  NS_LOG_FUNCTION (this);
  // at this stage used only by the scheduler for updating txMode

  LteUeConfig_t rrcParams;
  rrcParams.m_rnti = cmacParams.m_rnti;
  rrcParams.m_reconfigureFlag = true;
  rrcParams.m_transmissionMode = cmacParams.m_transmissionMode;

  // for now, the SRS CI is passed every time, so we need to overwrite the value
  std::map<uint16_t, Ptr<UeInfo> >::iterator it;
  it = m_ueMap.find (cmacParams.m_rnti);
  std::cout<<" RrcConfigurationIpdate : RNTI = "<<cmacParams.m_rnti<<" IMSI = "<<it->second->GetImsi()<<std::endl;
  //sleep(2);
  NS_ASSERT_MSG (it!=m_ueMap.end (), "Unable to find UeInfo");
  rrcParams.m_srsConfigurationIndex = (*it).second->GetSrsConfigurationIndex ();

  // update Tx Mode info at eNB
  (*it).second->SetTransmissionMode (cmacParams.m_transmissionMode);

  rrcParams.m_reconfigureFlag = true;
  PropagateRrcConnectionReconfiguration (rrcParams);
}


void 
LteEnbRrc::PropagateRrcConnectionReconfiguration (LteUeConfig_t params)
{


  Ptr<LteUeRrc> ueRrc = GetUeRrcByRnti (params.m_rnti);
  ueRrc->DoRecvConnectionReconfiguration (params);


  // configure MAC (and scheduler)
  LteEnbCmacSapProvider::UeConfig req;
  req.m_rnti = params.m_rnti;
  req.m_transmissionMode = params.m_transmissionMode;
  m_cmacSapProvider->UeUpdateConfigurationReq (req);  

  // configure PHY
  m_cphySapProvider->SetTransmissionMode (params.m_rnti, params.m_transmissionMode);
  m_cphySapProvider->SetSrsConfigurationIndex (params.m_rnti, params.m_srsConfigurationIndex);
}


Ptr<LteUeRrc> 
LteEnbRrc::GetUeRrcByImsi (uint64_t imsi)
{
  NS_LOG_FUNCTION (this);
    // WILD HACK

  NodeList::Iterator listEnd = NodeList::End ();
  bool found = false;
  for (NodeList::Iterator i = NodeList::Begin (); i != listEnd; i++)
    {
      Ptr<Node> node = *i;
      int nDevs = node->GetNDevices ();
      std::cout<<"Just for cross-check : No. of Devices = "<<nDevs<<std::endl;
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (!ueDev)
            {
              continue;
            }

          else
            {
        	  Ptr<LteUeRrc> ueRrc = ueDev->GetRrc ();
        	  //RAJARAJAN
              if ((ueDev->GetImsi () == imsi) && (ueRrc->GetCellId() == m_cellId))
                {
            	  found = true;
            	  return ueRrc;
                }
              //--RAJARAJAN--
            }
        }
    }
  NS_ASSERT_MSG (found , " Unable to find UE with IMSI =" << imsi);
  return 0;
}
 


Ptr<LteUeRrc> 
LteEnbRrc::GetUeRrcByRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
  uint64_t imsi;
  uint16_t cellId;
  NodeList::Iterator listEnd = NodeList::End ();
  bool found = false;
  for (NodeList::Iterator i = NodeList::Begin (); i != listEnd; i++)
    {
      Ptr<Node> node = *i;
      int nDevs = node->GetNDevices ();
   //   std::cout<<"Number of devices = "<<nDevs<<std::endl;
      // Data radio Bearer Setup request
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (!ueDev)
            {
              continue;
            }
          else
            {
              Ptr<LteUeRrc> rrc = ueDev->GetRrc ();
              imsi = ueDev->GetImsi();
              cellId = rrc->GetCellId();
           //   std::cout<<"UE's cell ID = "<<rrc->GetCellId()<<std::endl;
              if ((rrc->GetRnti () == rnti) && (rrc->GetCellId () == m_cellId))
                {                 
                  return rrc;
                  found=true;
                }
            }
        }
    }
  std::cout<<"IMSI = "<<imsi<<" RNTI = "<<rnti<<" Corresponding Cell ID = "<<cellId<<" Current cell ID = "<<m_cellId<<std::endl;
  NS_ASSERT_MSG (found , " Unable to find UE with RNTI=" << rnti << " cellId=" << m_cellId);
  return 0;
}

} // namespace ns3

