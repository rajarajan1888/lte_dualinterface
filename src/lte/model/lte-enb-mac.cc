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
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 *         Nicola Baldo  <nbaldo@cttc.es>
 */


#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/packet.h>
#include "ns3/simulator.h"

#include "lte-amc.h"
#include "lte-control-messages.h"
#include "lte-enb-net-device.h"
#include "lte-ue-net-device.h"

#include <ns3/lte-enb-mac.h>
#include <ns3/lte-radio-bearer-tag.h>
#include <ns3/lte-ue-phy.h>

#include "ns3/lte-mac-sap.h"
#include <ns3/lte-common.h>

#include <iostream>
#include <fstream>


NS_LOG_COMPONENT_DEFINE ("LteEnbMac");

namespace ns3 {


NS_OBJECT_ENSURE_REGISTERED (LteEnbMac);



// //////////////////////////////////////
// member SAP forwarders
// //////////////////////////////////////


class EnbMacMemberLteEnbCmacSapProvider : public LteEnbCmacSapProvider
{
public:
  EnbMacMemberLteEnbCmacSapProvider (LteEnbMac* mac);

  // inherited from LteEnbCmacSapProvider
  virtual void ConfigureMac (uint8_t ulBandwidth, uint8_t dlBandwidth);
  virtual void SetCellId (uint16_t cellId);
  virtual void AddUe (uint16_t rnti);
  virtual void RemoveUe (uint16_t rnti);
  virtual void AddLc (LcInfo lcinfo, LteMacSapUser* msu);
  virtual void ReconfigureLc (LcInfo lcinfo);
  virtual void ReleaseLc (uint16_t rnti, uint8_t lcid);
  virtual void UeUpdateConfigurationReq (UeConfig params);

private:
  LteEnbMac* m_mac;
};


EnbMacMemberLteEnbCmacSapProvider::EnbMacMemberLteEnbCmacSapProvider (LteEnbMac* mac)
  : m_mac (mac)
{
}

void
EnbMacMemberLteEnbCmacSapProvider::ConfigureMac (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  m_mac->DoConfigureMac (ulBandwidth, dlBandwidth);
}

void
EnbMacMemberLteEnbCmacSapProvider::SetCellId (uint16_t cellId)
{
  m_mac->DoSetCellId (cellId);
}

void
EnbMacMemberLteEnbCmacSapProvider::AddUe (uint16_t rnti)
{
  m_mac->DoAddUe (rnti);
}

void
EnbMacMemberLteEnbCmacSapProvider::RemoveUe (uint16_t rnti)
{
  m_mac->DoRemoveUe (rnti);
}

void
EnbMacMemberLteEnbCmacSapProvider::AddLc (LcInfo lcinfo, LteMacSapUser* msu)
{
  m_mac->DoAddLc (lcinfo, msu);
}

void
EnbMacMemberLteEnbCmacSapProvider::ReconfigureLc (LcInfo lcinfo)
{
  m_mac->DoReconfigureLc (lcinfo);
}

void
EnbMacMemberLteEnbCmacSapProvider::ReleaseLc (uint16_t rnti, uint8_t lcid)
{
  m_mac->DoReleaseLc (rnti, lcid);
}

void
EnbMacMemberLteEnbCmacSapProvider::UeUpdateConfigurationReq (UeConfig params)
{
  m_mac->DoUeUpdateConfigurationReq (params);
}



class EnbMacMemberFfMacSchedSapUser : public FfMacSchedSapUser
{
public:
  EnbMacMemberFfMacSchedSapUser (LteEnbMac* mac);


  virtual void SchedDlConfigInd (const struct SchedDlConfigIndParameters& params);
  virtual void SchedUlConfigInd (const struct SchedUlConfigIndParameters& params);
private:
  LteEnbMac* m_mac;
};


EnbMacMemberFfMacSchedSapUser::EnbMacMemberFfMacSchedSapUser (LteEnbMac* mac)
  : m_mac (mac)
{
}


void
EnbMacMemberFfMacSchedSapUser::SchedDlConfigInd (const struct SchedDlConfigIndParameters& params)
{

	std::ofstream myFileDl;
	myFileDl.open("//home//rajarajan//Documents//att_workspace//ltea//SchedDlConfigInd.txt");
	myFileDl<<"Sched DL Config Index \n";
	myFileDl.close();
	m_mac->DoSchedDlConfigInd (params);
}



void
EnbMacMemberFfMacSchedSapUser::SchedUlConfigInd (const struct SchedUlConfigIndParameters& params)
{
  m_mac->DoSchedUlConfigInd (params);
}



class EnbMacMemberFfMacCschedSapUser : public FfMacCschedSapUser
{
public:
  EnbMacMemberFfMacCschedSapUser (LteEnbMac* mac);

  virtual void CschedCellConfigCnf (const struct CschedCellConfigCnfParameters& params);
  virtual void CschedUeConfigCnf (const struct CschedUeConfigCnfParameters& params);
  virtual void CschedLcConfigCnf (const struct CschedLcConfigCnfParameters& params);
  virtual void CschedLcReleaseCnf (const struct CschedLcReleaseCnfParameters& params);
  virtual void CschedUeReleaseCnf (const struct CschedUeReleaseCnfParameters& params);
  virtual void CschedUeConfigUpdateInd (const struct CschedUeConfigUpdateIndParameters& params);
  virtual void CschedCellConfigUpdateInd (const struct CschedCellConfigUpdateIndParameters& params);

private:
  LteEnbMac* m_mac;
};


EnbMacMemberFfMacCschedSapUser::EnbMacMemberFfMacCschedSapUser (LteEnbMac* mac)
  : m_mac (mac)
{
}

void
EnbMacMemberFfMacCschedSapUser::CschedCellConfigCnf (const struct CschedCellConfigCnfParameters& params)
{
  m_mac->DoCschedCellConfigCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedUeConfigCnf (const struct CschedUeConfigCnfParameters& params)
{
  m_mac->DoCschedUeConfigCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedLcConfigCnf (const struct CschedLcConfigCnfParameters& params)
{
  m_mac->DoCschedLcConfigCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedLcReleaseCnf (const struct CschedLcReleaseCnfParameters& params)
{
  m_mac->DoCschedLcReleaseCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedUeReleaseCnf (const struct CschedUeReleaseCnfParameters& params)
{
  m_mac->DoCschedUeReleaseCnf (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedUeConfigUpdateInd (const struct CschedUeConfigUpdateIndParameters& params)
{
  m_mac->DoCschedUeConfigUpdateInd (params);
}

void
EnbMacMemberFfMacCschedSapUser::CschedCellConfigUpdateInd (const struct CschedCellConfigUpdateIndParameters& params)
{
  m_mac->DoCschedCellConfigUpdateInd (params);
}



// ---------- PHY-SAP


class EnbMacMemberLteEnbPhySapUser : public LteEnbPhySapUser
{
public:
  EnbMacMemberLteEnbPhySapUser (LteEnbMac* mac);

  // inherited from LteEnbPhySapUser
  virtual void ReceivePhyPdu (Ptr<Packet> p);
  virtual void SubframeIndication (uint32_t frameNo, uint32_t subframeNo);
  virtual void ReceiveLteControlMessage (Ptr<LteControlMessage> msg);
  virtual void UlCqiReport (FfMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi);
  virtual void UlInfoListElementHarqFeeback (UlInfoListElement_s params);
  virtual void DlInfoListElementHarqFeeback (DlInfoListElement_s params);

private:
  LteEnbMac* m_mac;
};

EnbMacMemberLteEnbPhySapUser::EnbMacMemberLteEnbPhySapUser (LteEnbMac* mac) : m_mac (mac)
{
}


void
EnbMacMemberLteEnbPhySapUser::ReceivePhyPdu (Ptr<Packet> p)
{
  m_mac->DoReceivePhyPdu (p);
}

void
EnbMacMemberLteEnbPhySapUser::SubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
  m_mac->DoSubframeIndication (frameNo, subframeNo);
}

void
EnbMacMemberLteEnbPhySapUser::ReceiveLteControlMessage (Ptr<LteControlMessage> msg)
{
  m_mac->DoReceiveLteControlMessage (msg);
}

void
EnbMacMemberLteEnbPhySapUser::UlCqiReport (FfMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi)
{
  m_mac->DoUlCqiReport (ulcqi);
}

void
EnbMacMemberLteEnbPhySapUser::UlInfoListElementHarqFeeback (UlInfoListElement_s params)
{
  m_mac->DoUlInfoListElementHarqFeeback (params);
}

void
EnbMacMemberLteEnbPhySapUser::DlInfoListElementHarqFeeback (DlInfoListElement_s params)
{
  m_mac->DoDlInfoListElementHarqFeeback (params);
}


// ////////////////////////////////////// m_device
// generic LteEnbMac methods
// //////////////////////////////////////


TypeId
LteEnbMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteEnbMac")
    .SetParent<Object> ()
    .AddConstructor<LteEnbMac> ()
    .AddTraceSource ("DlScheduling",
                     "Information regarding DL scheduling.",
                     MakeTraceSourceAccessor (&LteEnbMac::m_dlScheduling))
    .AddTraceSource ("UlScheduling",
                     "Information regarding UL scheduling.",
                     MakeTraceSourceAccessor (&LteEnbMac::m_ulScheduling))
  ;

  return tid;
}


LteEnbMac::LteEnbMac ()
{
  NS_LOG_FUNCTION (this);
  m_macSapProvider = new EnbMacMemberLteMacSapProvider<LteEnbMac> (this);
  m_cmacSapProvider = new EnbMacMemberLteEnbCmacSapProvider (this);
  m_schedSapUser = new EnbMacMemberFfMacSchedSapUser (this);
  m_cschedSapUser = new EnbMacMemberFfMacCschedSapUser (this);
  m_enbPhySapUser = new EnbMacMemberLteEnbPhySapUser (this);
}


LteEnbMac::~LteEnbMac ()
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_dlCqiReceived.clear ();
  m_ulCqiReceived.clear ();
  m_ulCeReceived.clear ();
  m_dlInfoListReceived.clear ();
  m_ulInfoListReceived.clear ();
m_miDlHarqProcessesPackets.clear ();
  delete m_macSapProvider;
  delete m_cmacSapProvider;
  delete m_schedSapUser;
  delete m_cschedSapUser;
  delete m_enbPhySapUser;
}


void
LteEnbMac::SetFfMacSchedSapProvider (FfMacSchedSapProvider* s)
{
  m_schedSapProvider = s;
}

FfMacSchedSapUser*
LteEnbMac::GetFfMacSchedSapUser (void)
{
  return m_schedSapUser;
}

void
LteEnbMac::SetFfMacCschedSapProvider (FfMacCschedSapProvider* s)
{
  m_cschedSapProvider = s;
}

FfMacCschedSapUser*
LteEnbMac::GetFfMacCschedSapUser (void)
{
  return m_cschedSapUser;
}



void
LteEnbMac::SetLteMacSapUser (LteMacSapUser* s)
{
  m_macSapUser = s;
}

LteMacSapProvider*
LteEnbMac::GetLteMacSapProvider (void)
{
  return m_macSapProvider;
}

void
LteEnbMac::SetLteEnbCmacSapUser (LteEnbCmacSapUser* s)
{
  m_cmacSapUser = s;
}

LteEnbCmacSapProvider*
LteEnbMac::GetLteEnbCmacSapProvider (void)
{
  return m_cmacSapProvider;
}

void
LteEnbMac::SetLteEnbPhySapProvider (LteEnbPhySapProvider* s)
{
  m_enbPhySapProvider = s;
}


LteEnbPhySapUser*
LteEnbMac::GetLteEnbPhySapUser ()
{
  return m_enbPhySapUser;
}



void
LteEnbMac::DoSubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
  NS_LOG_FUNCTION (this << " EnbMac - frame " << frameNo << " subframe " << subframeNo);

  // Store current frame / subframe number
  m_frameNo = frameNo;
  m_subframeNo = subframeNo;

  static std::ofstream confusion;
  static int cqicounter = 0;
  if(cqicounter==0)
   	  confusion.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//cqisframe.txt");
  cqicounter++;

  confusion<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Frame : "<<frameNo<<" Sub frame : "<<subframeNo<<" CQI List : "<<m_dlCqiReceived.size()<<"\n";
  // --- DOWNLINK ---  BuildDataList.txt
  // Send Dl-CQI info to the scheduler
  if (m_dlCqiReceived.size () > 0)
    {
	  FfMacSchedSapProvider::SchedDlCqiInfoReqParameters dlcqiInfoReq;
      dlcqiInfoReq.m_sfnSf = ((0x3FF & frameNo) << 4) | (0xF & subframeNo);

      int cqiNum = m_dlCqiReceived.size ();
      if (cqiNum > MAX_CQI_LIST)
        {
          cqiNum = MAX_CQI_LIST;
        }
      dlcqiInfoReq.m_cqiList.insert (dlcqiInfoReq.m_cqiList.begin (), m_dlCqiReceived.begin (), m_dlCqiReceived.end ());
      confusion<<"CQI list cross-check size : "<<dlcqiInfoReq.m_cqiList.size()<<"\n";
      m_dlCqiReceived.erase (m_dlCqiReceived.begin (), m_dlCqiReceived.end ());
      m_schedSapProvider->SchedDlCqiInfoReq (dlcqiInfoReq);


    }

  // Get downlink transmission opportunities
  uint32_t dlSchedFrameNo = m_frameNo;
  uint32_t dlSchedSubframeNo = m_subframeNo;
  //   NS_LOG_DEBUG (this << " sfn " << frameNo << " sbfn " << subframeNo);
  if (dlSchedSubframeNo + m_macChTtiDelay > 10)
    {
      dlSchedFrameNo++;
      dlSchedSubframeNo = (dlSchedSubframeNo + m_macChTtiDelay) % 10;
    }
  else
    {
      dlSchedSubframeNo = dlSchedSubframeNo + m_macChTtiDelay;
    }
  FfMacSchedSapProvider::SchedDlTriggerReqParameters dlparams;
  dlparams.m_sfnSf = ((0x3FF & dlSchedFrameNo) << 4) | (0xF & dlSchedSubframeNo);

  // Forward DL HARQ feebacks collected during last TTI
  std::cout<<"WATCHING M_DLINFOLISTRECEIVED: "<<m_dlInfoListReceived.size()<<std::endl;

  static std::ofstream dlInfo;
  static int counter = 0;
  if(counter==0)
   dlInfo.open("//home//rajarajan//Documents//att_workspace//ltea//dlInfo.txt");
  counter++;

  dlInfo<<" DL Info List Received Size : "<<m_dlInfoListReceived.size()<<" \n";
  if (m_dlInfoListReceived.size () > 0)
    {
      dlparams.m_dlInfoList = m_dlInfoListReceived;
      // empty local buffer
      m_dlInfoListReceived.clear ();
    }

  std::ofstream myFile;
      myFile.open("//home//rajarajan//Documents//att_workspace//ltea//SubFrameIndication.txt");
      myFile << "SubFrame Indication? \n";
      myFile.close();

  m_schedSapProvider->SchedDlTriggerReq (dlparams);


  // --- UPLINK ---
  // Send UL-CQI info to the scheduler
  std::vector <FfMacSchedSapProvider::SchedUlCqiInfoReqParameters>::iterator itCqi; 
  for (uint16_t i = 0; i < m_ulCqiReceived.size (); i++)
    {
      if (subframeNo>1)
        {        
          m_ulCqiReceived.at (i).m_sfnSf = ((0x3FF & frameNo) << 4) | (0xF & subframeNo);
        }
      else
        {
          m_ulCqiReceived.at (i).m_sfnSf = ((0x3FF & (frameNo-1)) << 4) | (0xF & 10);
        }
      m_schedSapProvider->SchedUlCqiInfoReq (m_ulCqiReceived.at (i));
    }
    m_ulCqiReceived.clear ();
  
  // Send BSR reports to the scheduler
  if (m_ulCeReceived.size () > 0)
    {
      FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters ulMacReq;
      ulMacReq.m_sfnSf = ((0x3FF & frameNo) << 4) | (0xF & subframeNo);
      ulMacReq.m_macCeList.insert (ulMacReq.m_macCeList.begin (), m_ulCeReceived.begin (), m_ulCeReceived.end ());
      m_ulCeReceived.erase (m_ulCeReceived.begin (), m_ulCeReceived.end ());
      m_schedSapProvider->SchedUlMacCtrlInfoReq (ulMacReq);
    }


  // Get uplink transmission opportunities
  uint32_t ulSchedFrameNo = m_frameNo;
  uint32_t ulSchedSubframeNo = m_subframeNo;
  //   NS_LOG_DEBUG (this << " sfn " << frameNo << " sbfn " << subframeNo);
  if (ulSchedSubframeNo + (m_macChTtiDelay+UL_PUSCH_TTIS_DELAY) > 10)
    {
      ulSchedFrameNo++;
      ulSchedSubframeNo = (ulSchedSubframeNo + (m_macChTtiDelay+UL_PUSCH_TTIS_DELAY)) % 10;
    }
  else
    {
      ulSchedSubframeNo = ulSchedSubframeNo + (m_macChTtiDelay+UL_PUSCH_TTIS_DELAY);
    }
  FfMacSchedSapProvider::SchedUlTriggerReqParameters ulparams;
  ulparams.m_sfnSf = ((0x3FF & ulSchedFrameNo) << 4) | (0xF & ulSchedSubframeNo);

  // Forward DL HARQ feebacks collected during last TTI
  if (m_ulInfoListReceived.size () > 0)
    {
     ulparams.m_ulInfoList = m_ulInfoListReceived;
      // empty local buffer
      m_ulInfoListReceived.clear ();
    }

  m_schedSapProvider->SchedUlTriggerReq (ulparams);

}


void
LteEnbMac::DoReceiveLteControlMessage  (Ptr<LteControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);
  if (msg->GetMessageType () == LteControlMessage::DL_CQI)
    {
	  std::cout<<"Control Message : DL_CQI"<<std::endl;
      Ptr<DlCqiLteControlMessage> dlcqi = DynamicCast<DlCqiLteControlMessage> (msg);
      ReceiveDlCqiLteControlMessage (dlcqi);
    }
  else if (msg->GetMessageType () == LteControlMessage::BSR)
    {
	  std::cout<<"Control Message : Buffer Status Report"<<std::endl;
      Ptr<BsrLteControlMessage> bsr = DynamicCast<BsrLteControlMessage> (msg);
      ReceiveBsrMessage (bsr->GetBsr ());
    }
  else if (msg->GetMessageType () == LteControlMessage::DL_HARQ)
    {
	  std::cout<<"Control Message : HARQ"<<std::endl;
      Ptr<DlHarqFeedbackLteControlMessage> dlharq = DynamicCast<DlHarqFeedbackLteControlMessage> (msg);
      DoDlInfoListElementHarqFeeback (dlharq->GetDlHarqFeedback ());
    }
  else
    {
      NS_LOG_LOGIC (this << " LteControlMessage not recognized");
    }
}


void
LteEnbMac::DoUlCqiReport (FfMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi)
{ 
  if (ulcqi.m_ulCqi.m_type == UlCqi_s::PUSCH)
    {
      NS_LOG_DEBUG (this << " eNB rxed an PUSCH UL-CQI");
    }
  m_ulCqiReceived.push_back (ulcqi);
}
// dlSchedtrace.txt

void
LteEnbMac::ReceiveDlCqiLteControlMessage  (Ptr<DlCqiLteControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);

  static std::ofstream dlcqimsg;
  static int cqiltectr = 0;
  if(cqiltectr==0)
	  dlcqimsg.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//dlcqiltectrl.txt");
  cqiltectr++;
  dlcqimsg<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Pushing back CQI received : "<<msg->GetDlCqi().m_rnti<<"\n";
  CqiListElement_s dlcqi = msg->GetDlCqi ();
  NS_LOG_LOGIC (this << "Enb Received DL-CQI rnti" << dlcqi.m_rnti);
  m_dlCqiReceived.push_back (dlcqi);



}


void
LteEnbMac::ReceiveBsrMessage  (MacCeListElement_s bsr)
{
  NS_LOG_FUNCTION (this);

  m_ulCeReceived.push_back (bsr);
}



void
LteEnbMac::DoReceivePhyPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  LteRadioBearerTag tag;
  p->RemovePacketTag (tag);

  // store info of the packet received

//   std::map <uint16_t,UlInfoListElement_s>::iterator it;
//   u_int rnti = tag.GetRnti ();
//  u_int lcid = tag.GetLcid ();
//   it = m_ulInfoListElements.find (tag.GetRnti ());
//   if (it == m_ulInfoListElements.end ())
//     {
//       // new RNTI
//       UlInfoListElement_s ulinfonew;
//       ulinfonew.m_rnti = tag.GetRnti ();
//       // always allocate full size of ulReception vector, initializing all elements to 0
//       ulinfonew.m_ulReception.assign (MAX_LC_LIST+1, 0);
//       // set the element for the current LCID
//       ulinfonew.m_ulReception.at (tag.GetLcid ()) = p->GetSize ();
//       ulinfonew.m_receptionStatus = UlInfoListElement_s::Ok;
//       ulinfonew.m_tpc = 0; // Tx power control not implemented at this stage
//       m_ulInfoListElements.insert (std::pair<uint16_t, UlInfoListElement_s > (tag.GetRnti (), ulinfonew));
// 
//     }
//   else
//     {
//       // existing RNTI: we just set the value for the current
//       // LCID. Note that the corresponding element had already been
//       // allocated previously.
//       NS_ASSERT_MSG ((*it).second.m_ulReception.at (tag.GetLcid ()) == 0, "would overwrite previously written ulReception element");
//       (*it).second.m_ulReception.at (tag.GetLcid ()) = p->GetSize ();
//       (*it).second.m_receptionStatus = UlInfoListElement_s::Ok;
//     }


 // std::cout<<" eNB MAC - ReceivePhyPdu "<<std::endl;
 // sleep (3);
  // forward the packet to the correspondent RLC
  LteFlowId_t flow ( tag.GetRnti (), tag.GetLcid () );
  std::map <LteFlowId_t, LteMacSapUser* >::iterator it2;
  it2 = m_rlcAttached.find (flow);
  NS_ASSERT_MSG (it2 != m_rlcAttached.end (), "UE not attached rnti=" << flow.m_rnti << " lcid=" << (uint32_t) flow.m_lcId);
  (*it2).second->ReceivePdu (p);

}




// ////////////////////////////////////////////
// CMAC SAP
// ////////////////////////////////////////////

// mutable

void
LteEnbMac::DoSetCellId(uint16_t cellId)
{
	m_cellId = cellId;
	/* std::cout<<" DoSetCellId : Cell ID = "<<m_cellId<<" RHS assignment = "<<cellId<<std::endl;
	sleep(2); */
}

void
LteEnbMac::DoConfigureMac (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  NS_LOG_FUNCTION (this << " ulBandwidth=" << (uint16_t) ulBandwidth << " dlBandwidth=" << (uint16_t) dlBandwidth);
  FfMacCschedSapProvider::CschedCellConfigReqParameters params;
  // Configure the subset of parameters used by FfMacScheduler
  params.m_ulBandwidth = ulBandwidth;
  params.m_dlBandwidth = dlBandwidth;
  params.m_cellId = m_cellId;
  std::cout<<" ConfigureMac : Cell ID = "<<params.m_cellId<<" RHS assignment = "<<m_cellId<<std::endl;
  //sleep(2);
  m_macChTtiDelay = m_enbPhySapProvider->GetMacChTtiDelay ();
  // ...more parameters can be configured
  m_cschedSapProvider->CschedCellConfigReq (params);
}




void
LteEnbMac::DoAddUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << " rnti=" << rnti);
  FfMacCschedSapProvider::CschedUeConfigReqParameters params;
  params.m_rnti = rnti;
  params.m_transmissionMode = 0; // set to default value (SISO) for avoiding random initialization (valgrind error)
  m_cschedSapProvider->CschedUeConfigReq (params);

  // Create DL trasmission HARQ buffers
  std::vector < Ptr<Packet> > dlHarqLayer0pkt;
  dlHarqLayer0pkt.resize (8);
  std::vector < Ptr<Packet> > dlHarqLayer1pkt;
  dlHarqLayer1pkt.resize (8);
  DlHarqProcessesBuffer_t buf;
  buf.push_back (dlHarqLayer0pkt);
  buf.push_back (dlHarqLayer1pkt);
  m_miDlHarqProcessesPackets.insert (std::pair <uint16_t, DlHarqProcessesBuffer_t> (rnti, buf));
}

void
LteEnbMac::DoRemoveUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << " rnti=" << rnti);
  FfMacCschedSapProvider::CschedUeReleaseReqParameters params;
  params.m_rnti = rnti;
  m_cschedSapProvider->CschedUeReleaseReq (params);
}

void
LteEnbMac::DoAddLc (LteEnbCmacSapProvider::LcInfo lcinfo, LteMacSapUser* msu)
{
  NS_LOG_FUNCTION (this);
  std::map <LteFlowId_t, LteMacSapUser* >::iterator it;

  LteFlowId_t flow (lcinfo.rnti, lcinfo.lcId);

  it = m_rlcAttached.find (flow);
  if (it == m_rlcAttached.end ())
    {
      m_rlcAttached.insert (std::pair<LteFlowId_t, LteMacSapUser* > (flow, msu));
    }
  else
    {
      NS_LOG_ERROR ("LC already exists");
    }


  struct FfMacCschedSapProvider::CschedLcConfigReqParameters params;
  params.m_rnti = lcinfo.rnti;
  params.m_reconfigureFlag = false;

  struct LogicalChannelConfigListElement_s lccle;
  lccle.m_logicalChannelIdentity = lcinfo.lcId;
  lccle.m_logicalChannelGroup = lcinfo.lcGroup;
  lccle.m_direction = LogicalChannelConfigListElement_s::DIR_BOTH;
  lccle.m_qosBearerType = lcinfo.isGbr ? LogicalChannelConfigListElement_s::QBT_GBR : LogicalChannelConfigListElement_s::QBT_NON_GBR;
  lccle.m_qci = lcinfo.qci;
  lccle.m_eRabMaximulBitrateUl = lcinfo.mbrUl;
  lccle.m_eRabMaximulBitrateDl = lcinfo.mbrDl;
  lccle.m_eRabGuaranteedBitrateUl = lcinfo.gbrUl;
  lccle.m_eRabGuaranteedBitrateDl = lcinfo.gbrDl;
  params.m_logicalChannelConfigList.push_back (lccle);

  m_cschedSapProvider->CschedLcConfigReq (params);
}

void
LteEnbMac::DoReconfigureLc (LteEnbCmacSapProvider::LcInfo lcinfo)
{
  NS_FATAL_ERROR ("not implemented");
}

void
LteEnbMac::DoReleaseLc (uint16_t rnti, uint8_t lcid)
{
  NS_FATAL_ERROR ("not implemented");
}



// ////////////////////////////////////////////
// MAC SAP
// ////////////////////////////////////////////


void
LteEnbMac::DoTransmitPdu (LteMacSapProvider::TransmitPduParameters params)
{
  NS_LOG_FUNCTION (this);
  LteRadioBearerTag tag (params.rnti, params.lcid, params.layer);
  params.pdu->AddPacketTag (tag);
  // Store pkt in HARQ buffer
  std::map <uint16_t, DlHarqProcessesBuffer_t>::iterator it =  m_miDlHarqProcessesPackets.find (params.rnti);
  NS_ASSERT (it!=m_miDlHarqProcessesPackets.end ());
  NS_LOG_DEBUG (this << " LAYER " <<(uint16_t)tag.GetLayer () << " HARQ ID " << (uint16_t)params.harqProcessId);
//   NS_ASSERT ((*it).second.at (params.layer).at (params.harqProcessId) == 0);
  (*it).second.at (params.layer).at (params.harqProcessId) = params.pdu;//->Copy ();
  m_enbPhySapProvider->SendMacPdu (params.pdu);
}

void
LteEnbMac::DoReportBufferStatus (LteMacSapProvider::ReportBufferStatusParameters params)
{
  NS_LOG_FUNCTION (this);
  FfMacSchedSapProvider::SchedDlRlcBufferReqParameters req;
  req.m_rnti = params.rnti;
  req.m_logicalChannelIdentity = params.lcid;
  req.m_rlcTransmissionQueueSize = params.txQueueSize;
  req.m_rlcTransmissionQueueHolDelay = params.txQueueHolDelay;
  req.m_rlcRetransmissionQueueSize = params.retxQueueSize;
  req.m_rlcRetransmissionHolDelay = params.retxQueueHolDelay;
  req.m_rlcStatusPduSize = params.statusPduSize;

  std::ofstream myBuffer;
  myBuffer.open("//home//rajarajan//Documents//att_workspace//ltea//ReportBuffer.txt");
  myBuffer << "Queue size : "<<req.m_rlcTransmissionQueueSize<<" Retrans queue size : "<<req.m_rlcRetransmissionQueueSize<<"\n";
  myBuffer.close();


  m_schedSapProvider->SchedDlRlcBufferReq (req);
}



// ////////////////////////////////////////////
// SCHED SAP
// ////////////////////////////////////////////



void
LteEnbMac::DoSchedDlConfigInd (FfMacSchedSapUser::SchedDlConfigIndParameters ind)
{
  NS_LOG_FUNCTION (this);
  std::cout<<"DO SCHED DL CONFIG IND : LTE ENB MAC "<<std::endl;
  // Create DL PHY PDU
  Ptr<PacketBurst> pb = CreateObject<PacketBurst> ();
  std::map <LteFlowId_t, LteMacSapUser* >::iterator it;

  static std::ofstream myFileA;
  static int counter = 0;
  if(counter==0)
  {
	myFileA.open("//home//rajarajan//Documents//att_workspace//ltea//BuildDataList.txt");

  }
  counter++;
  myFileA<<"\n\n\n";
  myFileA << this<<" Time = "<<Simulator::Now().GetMilliSeconds()<<" DataList Value : "<<(uint16_t)ind.m_buildDataList.size()<<" \n";


  for (unsigned int i = 0; i < ind.m_buildDataList.size (); i++)
    {
	  std::ofstream myFileB;

	         myFileA << "RLC PDU Size : "<<(uint16_t)ind.m_buildDataList.at(i).m_rlcPduList.size()<<" \n";
        for (unsigned int j = 0; j < ind.m_buildDataList.at (i).m_rlcPduList.size (); j++)
        {
   	         myFileA << "Value : "<<(uint16_t)ind.m_buildDataList.at(i).m_rlcPduList.at(j).size()<<" \n";

          for (uint16_t k = 0; k < ind.m_buildDataList.at (i).m_rlcPduList.at (j).size (); k++)
            {
        	  myFileA << "Value : "<<(uint16_t)ind.m_buildDataList.at(i).m_dci.m_ndi.at(k)<<" \n";

              if (ind.m_buildDataList.at (i).m_dci.m_ndi.at (k) == 1)
                {

                  // New Data -> retrieve it from RLC
                  LteFlowId_t flow (ind.m_buildDataList.at (i).m_rnti,
                                    ind.m_buildDataList.at (i).m_rlcPduList.at (j).at (k).m_logicalChannelIdentity);
                  it = m_rlcAttached.find (flow);
                  NS_ASSERT_MSG (it != m_rlcAttached.end (), "rnti=" << flow.m_rnti << " lcid=" << (uint32_t) flow.m_lcId);
                  NS_LOG_DEBUG (this << " rnti= " << flow.m_rnti << " lcid= " << (uint32_t) flow.m_lcId << " layer= " << k);
                  //myFileA << " RNTI = "<<(uint16_t)flow.m_rnti<<" LCID = "<<(uint32_t) flow.m_lcId <<" Layer = "<<(uint16_t)k<<" RLC PDU List = "<<(uint16_t)ind.m_buildDataList.at (i).m_rlcPduList.at (j).at (k).m_size<<" DCI HARQ = "<<(uint16_t)ind.m_buildDataList.at (i).m_dci.m_harqProcess<<"\n";
                  //myFileE.close();
                  //if((*it).second)
                  (*it).second->NotifyTxOpportunity (ind.m_buildDataList.at (i).m_rlcPduList.at (j).at (k).m_size, k, ind.m_buildDataList.at (i).m_dci.m_harqProcess);
                }
              else
                {

                if (ind.m_buildDataList.at (i).m_dci.m_tbsSize.at (k)>0)
                    {
        	      	//  	myFileA << "DCI TBS Size = "<<(uint16_t)ind.m_buildDataList.at(i).m_dci.m_tbsSize.at(k)<<"\n";

                      // HARQ retransmission -> retrieve TB from HARQ buffer
                      std::map <uint16_t, DlHarqProcessesBuffer_t>::iterator it = m_miDlHarqProcessesPackets.find (ind.m_buildDataList.at (i).m_rnti);
                      NS_ASSERT(it!=m_miDlHarqProcessesPackets.end());
                      Ptr<Packet> pkt = (*it).second.at (k).at ( ind.m_buildDataList.at (i).m_dci.m_harqProcess)->Copy ();
                      m_enbPhySapProvider->SendMacPdu (pkt);
                    }
                }
            }
        }
      // send the relative DCI
      Ptr<DlDciLteControlMessage> msg = Create<DlDciLteControlMessage> ();
      myFileA << " DCI-RNTI = "<<(uint16_t)ind.m_buildDataList.at(i).m_dci.m_rnti<<"\n";
      msg->SetDci (ind.m_buildDataList.at (i).m_dci);
      m_enbPhySapProvider->SendLteControlMessage (msg);
    }
  std::cout<<"m_buildDataList Size = "<<ind.m_buildDataList.size()<<std::endl;
  // Fire the trace with the DL information
  for (  uint32_t i  = 0; i < ind.m_buildDataList.size (); i++ )
    {
	  myFileA<<"TB Size = "<<ind.m_buildDataList.at (i).m_dci.m_tbsSize.size ()<<"\n";
      // Only one TB used
      if (ind.m_buildDataList.at (i).m_dci.m_tbsSize.size () == 1)
        {
          m_dlScheduling (m_frameNo, m_subframeNo, ind.m_buildDataList.at (i).m_dci.m_rnti,
                          ind.m_buildDataList.at (i).m_dci.m_mcs.at (0),
                          ind.m_buildDataList.at (i).m_dci.m_tbsSize.at (0),
                          0, 0
                          );

        }
      // Two TBs used
      else if (ind.m_buildDataList.at (i).m_dci.m_tbsSize.size () == 2)
        {
          m_dlScheduling (m_frameNo, m_subframeNo, ind.m_buildDataList.at (i).m_dci.m_rnti,
                          ind.m_buildDataList.at (i).m_dci.m_mcs.at (0),
                          ind.m_buildDataList.at (i).m_dci.m_tbsSize.at (0),
                          ind.m_buildDataList.at (i).m_dci.m_mcs.at (1),
                          ind.m_buildDataList.at (i).m_dci.m_tbsSize.at (1)
                          );
        }
      else
        {
          NS_FATAL_ERROR ("Found element with more than two transport blocks");
        }
    }
}

//UL_DCI
void
LteEnbMac::DoSchedUlConfigInd (FfMacSchedSapUser::SchedUlConfigIndParameters ind)
{
  NS_LOG_FUNCTION (this);

  for (unsigned int i = 0; i < ind.m_dciList.size (); i++)
    {
      // send the correspondent ul dci
      Ptr<UlDciLteControlMessage> msg = Create<UlDciLteControlMessage> ();
      msg->SetDci (ind.m_dciList.at (i));
      m_enbPhySapProvider->SendLteControlMessage (msg);
    }

  // Fire the trace with the UL information
  for (  uint32_t i  = 0; i < ind.m_dciList.size (); i++ )
    {
      m_ulScheduling (m_frameNo, m_subframeNo, ind.m_dciList.at (i).m_rnti,
                      ind.m_dciList.at (i).m_mcs, ind.m_dciList.at (i).m_tbSize);
    }



}




// ////////////////////////////////////////////
// CSCHED SAP
// ////////////////////////////////////////////


void
LteEnbMac::DoCschedCellConfigCnf (FfMacCschedSapUser::CschedCellConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedUeConfigCnf (FfMacCschedSapUser::CschedUeConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedLcConfigCnf (FfMacCschedSapUser::CschedLcConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
  // Call the CSCHED primitive
  // m_cschedSap->LcConfigCompleted();
}

void
LteEnbMac::DoCschedLcReleaseCnf (FfMacCschedSapUser::CschedLcReleaseCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedUeReleaseCnf (FfMacCschedSapUser::CschedUeReleaseCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoCschedUeConfigUpdateInd (FfMacCschedSapUser::CschedUeConfigUpdateIndParameters params)
{
  NS_LOG_FUNCTION (this);
  // propagates to RRC
  LteEnbCmacSapUser::UeConfig ueConfigUpdate;
  ueConfigUpdate.m_rnti = params.m_rnti;
  ueConfigUpdate.m_transmissionMode = params.m_transmissionMode;
  m_cmacSapUser->RrcConfigurationUpdateInd (ueConfigUpdate);
}

void
LteEnbMac::DoUeUpdateConfigurationReq (LteEnbCmacSapProvider::UeConfig params)
{
  NS_LOG_FUNCTION (this);

  // propagates to scheduler
  FfMacCschedSapProvider::CschedUeConfigReqParameters req;
  req.m_rnti = params.m_rnti;
  req.m_transmissionMode = params.m_transmissionMode;
  m_cschedSapProvider->CschedUeConfigReq (req);
}

void
LteEnbMac::DoCschedCellConfigUpdateInd (FfMacCschedSapUser::CschedCellConfigUpdateIndParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
LteEnbMac::DoUlInfoListElementHarqFeeback (UlInfoListElement_s params)
{
  NS_LOG_FUNCTION (this);
  m_ulInfoListReceived.push_back (params);
  std::ofstream myFile;
    myFile.open("//home//rajarajan//Documents//att_workspace//ltea//DoUlInfoListElementHarqFeedback.txt");
    myFile << "DOULINFOLISTELEMENTHARQFEEDBACK - IS THIS AT ALL BEING CALLED ? \n";
    myFile.close();
}

void
LteEnbMac::DoDlInfoListElementHarqFeeback (DlInfoListElement_s params)
{
  NS_LOG_FUNCTION (this);
  // Update HARQ buffer
  std::map <uint16_t, DlHarqProcessesBuffer_t>::iterator it =  m_miDlHarqProcessesPackets.find (params.m_rnti);
  NS_ASSERT (it!=m_miDlHarqProcessesPackets.end ());
  for (uint8_t layer = 0; layer < params.m_harqStatus.size (); layer++)
    {
      if (params.m_harqStatus.at (layer)==DlInfoListElement_s::ACK)
        {
          // discard buffer
          (*it).second.at (layer).at (params.m_harqProcessId) = 0;
          NS_LOG_DEBUG (this << " HARQ-ACK UE " << params.m_rnti << " harqId " << (uint16_t)params.m_harqProcessId << " layer " << (uint16_t)layer);
        }
      else if (params.m_harqStatus.at (layer)==DlInfoListElement_s::NACK)
        {
          NS_LOG_DEBUG (this << " HARQ-NACK UE " << params.m_rnti << " harqId " << (uint16_t)params.m_harqProcessId << " layer " << (uint16_t)layer);
        }
      else
        {
          NS_FATAL_ERROR (" HARQ functionality not implemented");
        }
    }
  std::cout<<"DODLINFOLISTELEMENTHARQFEEDBACK - IS THIS AT ALL BEING CALLED ? "<<std::endl;

  std::ofstream myFile;
  myFile.open("/home/rajarajan/Documents/att_workspace/ltea/DoDlInfoListElementHarqFeedback.txt");
  myFile << "DODLINFOLISTELEMENTHARQFEEDBACK - IS THIS AT ALL BEING CALLED ? \n";
  myFile.close();


  m_dlInfoListReceived.push_back (params);
}


} // namespace ns3
