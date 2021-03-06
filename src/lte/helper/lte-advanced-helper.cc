/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nicola Baldo <nbaldo@cttc.es> (re-wrote from scratch this helper)
 *         Giuseppe Piro <g.piro@poliba.it> (parts of the PHY & channel  creation & configuration copied from the GSoC 2011 code)
 */


//#include "lte-helper.h"
#include "lte-advanced-helper.h"
#include <ns3/string.h>
#include <ns3/log.h>
#include <ns3/abort.h>
#include <ns3/pointer.h>
#include <ns3/lte-enb-rrc.h>
#include <ns3/epc-ue-nas.h>
#include <ns3/epc-enb-application.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/lte-ue-mac.h>
#include <ns3/lte-enb-mac.h>
#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-enb-phy.h>
#include <ns3/lte-ue-phy.h>
#include <ns3/lte-spectrum-phy.h>
#include <ns3/lte-sinr-chunk-processor.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/friis-spectrum-propagation-loss.h>
//#include <ns3/log-distance-loss-model.h>
#include <ns3/isotropic-antenna-model.h>
#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/ff-mac-scheduler.h>
#include <ns3/lte-rlc.h>
#include <ns3/lte-rlc-um.h>
#include <ns3/lte-rlc-am.h>
#include <ns3/epc-enb-s1-sap.h>
// RAJARAJAN
#include <ns3/propagation-loss-model.h>
// -- RAJARAJAN --
#include <ns3/epc-helper.h>
#include <iostream>
#include <ns3/buildings-propagation-loss-model.h>
#include <ns3/lte-spectrum-value-helper.h>
#include <ns3/epc-x2.h>

NS_LOG_COMPONENT_DEFINE ("LteAdvancedHelper");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LteAdvancedHelper);

LteAdvancedHelper::LteAdvancedHelper (void)
{
  NS_LOG_FUNCTION (this);
  m_enbNetDeviceFactory.SetTypeId (LteEnbNetDevice::GetTypeId ());
  m_enbAntennaModelFactory.SetTypeId (IsotropicAntennaModel::GetTypeId ());
  m_ueAntennaModelFactory.SetTypeId (IsotropicAntennaModel::GetTypeId ());
  m_channelFactory.SetTypeId (SingleModelSpectrumChannel::GetTypeId ());
}

void
LteAdvancedHelper::DoStart (void)
{
  NS_LOG_FUNCTION (this);

  for(int i = 0 ; i < maxCCs ; i++ )
  {
	  Ptr<SpectrumChannel> m_downlinkChannel,m_uplinkChannel;
	  m_downlinkChannel = m_channelFactory.Create<SpectrumChannel> ();
	  m_uplinkChannel = m_channelFactory.Create<SpectrumChannel> ();

	  m_downlinkPathlossModel = m_dlPathlossModelFactory.Create ();
	  Ptr<SpectrumPropagationLossModel> dlSplm = m_downlinkPathlossModel->GetObject<SpectrumPropagationLossModel> ();
	  if (dlSplm != 0)
	  {
		  NS_LOG_LOGIC (this << " using a SpectrumPropagationLossModel in DL");
		  m_downlinkChannel->AddSpectrumPropagationLossModel (dlSplm);
	  }

	  else
	  {
		  NS_LOG_LOGIC (this << " using a PropagationLossModel in DL");
		  Ptr<PropagationLossModel> dlPlm = m_downlinkPathlossModel->GetObject<PropagationLossModel> ();
		  NS_ASSERT_MSG (dlPlm != 0, " " << m_downlinkPathlossModel << " is neither PropagationLossModel nor SpectrumPropagationLossModel");
		  m_downlinkChannel->AddPropagationLossModel (dlPlm);
	  }

	  m_uplinkPathlossModel = m_ulPathlossModelFactory.Create ();
	  Ptr<SpectrumPropagationLossModel> ulSplm = m_uplinkPathlossModel->GetObject<SpectrumPropagationLossModel> ();
	  if (ulSplm != 0)
	  {
		  NS_LOG_LOGIC (this << " using a SpectrumPropagationLossModel in UL");
		  m_uplinkChannel->AddSpectrumPropagationLossModel (ulSplm);
	  }

	  else
	  {
		  NS_LOG_LOGIC (this << " using a PropagationLossModel in UL");
		  Ptr<PropagationLossModel> ulPlm = m_uplinkPathlossModel->GetObject<PropagationLossModel> ();
		  NS_ASSERT_MSG (ulPlm != 0, " " << m_uplinkPathlossModel << " is neither PropagationLossModel nor SpectrumPropagationLossModel");
		  m_uplinkChannel->AddPropagationLossModel (ulPlm);
	  }

	  if (!m_fadingModelType.empty ())
	  {
		  Ptr<SpectrumPropagationLossModel> m_fadingModule;
		  m_fadingModule = m_fadingModelFactory.Create<SpectrumPropagationLossModel> ();
		  m_fadingModule->Start ();
		  m_downlinkChannel->AddSpectrumPropagationLossModel (m_fadingModule);
		  m_uplinkChannel->AddSpectrumPropagationLossModel (m_fadingModule);
	  }

	  m_downlinkChannelCA.push_back(m_downlinkChannel);
	  m_uplinkChannelCA.push_back(m_uplinkChannel);
  }

  m_phyStats = CreateObject<PhyStatsCalculator> ();
  m_macStats = CreateObject<MacStatsCalculator> ();
  m_rlcStats = CreateObject<RadioBearerStatsCalculator> ("RLC");
  m_pdcpStats = CreateObject<RadioBearerStatsCalculator> ("PDCP");

  Object::DoStart ();
}

LteAdvancedHelper::~LteAdvancedHelper (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId LteAdvancedHelper::GetTypeId (void)
{
  static TypeId
    tid =
    TypeId ("ns3::LteAdvancedHelper")
    .SetParent<Object> ()
    .AddConstructor<LteAdvancedHelper> ()
    .AddAttribute ("Scheduler",
                   "The type of scheduler to be used for eNBs",
                   StringValue ("ns3::PfFfMacScheduler"),
                   MakeStringAccessor (&LteAdvancedHelper::SetSchedulerType),
                   MakeStringChecker ())
    .AddAttribute ("PathlossModel",
                   "The type of pathloss model to be used",
                   StringValue ("ns3::LogDistancePropagationLossModel"),
                   MakeStringAccessor (&LteAdvancedHelper::SetPathlossModelType),
                   MakeStringChecker ())
    .AddAttribute ("FadingModel",
                   "The type of fading model to be used",
                   StringValue (""), // fake module -> no fading
                   MakeStringAccessor (&LteAdvancedHelper::SetFadingModel),
                   MakeStringChecker ())
  ;
  return tid;
}

void
LteAdvancedHelper::DoDispose ()
{
	NS_LOG_FUNCTION (this);
	for(unsigned int i = 0 ; i < m_downlinkChannelCA.size() ; i++)
	{
		m_downlinkChannelCA.at(i) = 0;
		m_uplinkChannelCA.at(i) = 0;
	}
	Object::DoDispose ();
}


void
LteAdvancedHelper::SetEpcHelper (Ptr<EpcHelper> h)
{
  NS_LOG_FUNCTION (this << h);
  m_epcHelperCA = h;
}

void
LteAdvancedHelper::SetSchedulerType (std::string type)
{
	NS_LOG_FUNCTION (this << type);
	m_schedulerFactory = ObjectFactory ();
	m_schedulerFactory.SetTypeId (type);
}

void
LteAdvancedHelper::SetSchedulerAttribute (std::string n, const AttributeValue &v)
{
	NS_LOG_FUNCTION (this << n);
	m_schedulerFactory.Set (n, v);
}


void
LteAdvancedHelper::SetPathlossModelType (std::string type)
{
	NS_LOG_FUNCTION (this << type);
	m_dlPathlossModelFactory = ObjectFactory ();
	m_dlPathlossModelFactory.SetTypeId (type);
	m_ulPathlossModelFactory = ObjectFactory ();
	m_ulPathlossModelFactory.SetTypeId (type);
}

void
LteAdvancedHelper::SetPathlossModelAttribute (std::string n, const AttributeValue &v)
{
	  NS_LOG_FUNCTION (this << n);
	  m_dlPathlossModelFactory.Set (n, v);
	  m_ulPathlossModelFactory.Set (n, v);

}

void
LteAdvancedHelper::SetEnbDeviceAttribute (std::string n, const AttributeValue &v)
{
	NS_LOG_FUNCTION (this);
	m_enbNetDeviceFactory.Set (n, v);
}


void
LteAdvancedHelper::SetEnbAntennaModelType (std::string type)
{
	  NS_LOG_FUNCTION (this);
	  m_enbAntennaModelFactory.SetTypeId (type);
}

void
LteAdvancedHelper::SetEnbAntennaModelAttribute (std::string n, const AttributeValue &v)
{
	  NS_LOG_FUNCTION (this);
	  m_enbAntennaModelFactory.Set (n, v);
}

void
LteAdvancedHelper::SetUeAntennaModelType (std::string type)
{
	  NS_LOG_FUNCTION (this);
	  m_ueAntennaModelFactory.SetTypeId (type);
}

void
LteAdvancedHelper::SetUeAntennaModelAttribute (std::string n, const AttributeValue &v)
{
	  NS_LOG_FUNCTION (this);
	  m_ueAntennaModelFactory.Set (n, v);
}

void
LteAdvancedHelper::SetFadingModel (std::string type)
{
	  NS_LOG_FUNCTION (this << type);
	  m_fadingModelType = type;
	  if (!type.empty ())
	  {
	      m_fadingModelFactory = ObjectFactory ();
	      m_fadingModelFactory.SetTypeId (type);
	  }
}

void
LteAdvancedHelper::SetFadingModelAttribute (std::string n, const AttributeValue &v)
{
	m_fadingModelFactory.Set (n, v);
}

void
LteAdvancedHelper::SetSpectrumChannelType (std::string type)
{
	  NS_LOG_FUNCTION (this << type);
	  m_channelFactory.SetTypeId (type);
}

void
LteAdvancedHelper::SetSpectrumChannelAttribute (std::string n, const AttributeValue &v)
{
	m_channelFactory.Set (n, v);
}


std::vector<NetDeviceContainer>
LteAdvancedHelper::InstallEnbDeviceCA (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  Start ();  // will run DoStart () if necessary
  std::vector<NetDeviceContainer> devices;
  unsigned int k = 0;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      devices.push_back(NetDeviceContainer());
      Ptr<Node> node = *i;
      std::vector<Ptr<NetDevice> > device = InstallMultipleEnbDevice (node);
      for(unsigned int j = 0 ; j < maxCCs ; j++)
      	  devices.at(k).Add(device.at(j));
      k++;
    }
  return devices;
}

std::vector<NetDeviceContainer>
LteAdvancedHelper::InstallUeDeviceCA (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  std::vector<NetDeviceContainer> devices;
  unsigned int k = 0;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
	  devices.push_back(NetDeviceContainer());
      Ptr<Node> node = *i;
      std::vector<Ptr<NetDevice> > device = InstallMultipleUeDevice (node);
      for(unsigned int j = 0 ; j < maxCCs ; j++)
       	  devices.at(k).Add(device.at(j));
      k++;
    }
  return devices;
}

std::vector<Ptr<NetDevice> >
LteAdvancedHelper::InstallMultipleEnbDevice (Ptr<Node> n)
{
//  std::vector<Ptr<LteEnbNetDevice> > devvtr;
//	Ptr<LteEnbRrc> rrc = CreateObject<LteEnbRrc> ();
  std::vector<Ptr<NetDevice> > devvtr;
  for(unsigned int i = 0 ; i < maxCCs ; i++)
  {
	  Ptr<LteEnbRrc> rrc = CreateObject<LteEnbRrc> ();
	  std::cout<<"Creating UE Net Interface Device "<<std::endl;
	  Ptr<LteSpectrumPhy> dlPhy = CreateObject<LteSpectrumPhy> ();
	  Ptr<LteSpectrumPhy> ulPhy = CreateObject<LteSpectrumPhy> ();

	  Ptr<LteEnbPhy> phy = CreateObject<LteEnbPhy> (dlPhy, ulPhy);

	  Ptr<LteHarqPhy> harq = Create<LteHarqPhy> ();
	  dlPhy->SetHarqPhyModule (harq);
	  ulPhy->SetHarqPhyModule (harq);
	  phy->SetHarqPhyModule (harq);

	  Ptr<LteCtrlSinrChunkProcessor> pCtrl = Create<LteCtrlSinrChunkProcessor> (phy->GetObject<LtePhy> ());
	  ulPhy->AddCtrlSinrChunkProcessor (pCtrl); // for evaluating SRS UL-CQI

	  Ptr<LteDataSinrChunkProcessor> pData = Create<LteDataSinrChunkProcessor> (ulPhy, phy);
	  ulPhy->AddDataSinrChunkProcessor (pData); // for evaluating PUSCH UL-CQI

	  Ptr<LteInterferencePowerChunkProcessor> pInterf = Create<LteInterferencePowerChunkProcessor> (phy);
	  ulPhy->AddInterferenceChunkProcessor (pInterf); // for interference power tracing

	  dlPhy->SetChannel (m_downlinkChannelCA.at(i));
	  ulPhy->SetChannel (m_uplinkChannelCA.at(i));

	  Ptr<MobilityModel> mm = n->GetObject<MobilityModel> ();
	  NS_ASSERT_MSG (mm, "MobilityModel needs to be set on node before calling LteHelper::InstallUeDevice ()");
	  dlPhy->SetMobility (mm);
	  ulPhy->SetMobility (mm);

	  Ptr<AntennaModel> antenna = (m_enbAntennaModelFactory.Create())->GetObject<AntennaModel>();

	  NS_ASSERT_MSG (antenna, "error in creating the AntennaModel object");
	  dlPhy->SetAntenna (antenna);
	  ulPhy->SetAntenna (antenna);

	  Ptr<LteEnbMac> mac = CreateObject<LteEnbMac> ();
	  Ptr<FfMacScheduler> sched = m_schedulerFactory.Create<FfMacScheduler>();

	  if (m_epcHelperCA != 0)
	  {
		  EnumValue epsBearerToRlcMapping;
		  rrc->GetAttribute ("EpsBearerToRlcMapping", epsBearerToRlcMapping);
      // it does not make sense to use RLC/SM when also using the EPC
		  if (epsBearerToRlcMapping.Get () == LteEnbRrc::RLC_SM_ALWAYS)
		  {
			  rrc->SetAttribute ("EpsBearerToRlcMapping", EnumValue (LteEnbRrc::RLC_UM_ALWAYS));
		  }
	  }

  // connect SAPs
	  rrc->SetLteEnbCmacSapProvider (mac->GetLteEnbCmacSapProvider ());
	  mac->SetLteEnbCmacSapUser (rrc->GetLteEnbCmacSapUser ());
	  rrc->SetLteMacSapProvider (mac->GetLteMacSapProvider ());

	  mac->SetFfMacSchedSapProvider (sched->GetFfMacSchedSapProvider ());
	  mac->SetFfMacCschedSapProvider (sched->GetFfMacCschedSapProvider ());

	  sched->SetFfMacSchedSapUser (mac->GetFfMacSchedSapUser ());
	  sched->SetFfMacCschedSapUser (mac->GetFfMacCschedSapUser ());

	  phy->SetLteEnbPhySapUser (mac->GetLteEnbPhySapUser ());
	  mac->SetLteEnbPhySapProvider (phy->GetLteEnbPhySapProvider ());

	  phy->SetLteEnbCphySapUser (rrc->GetLteEnbCphySapUser ());
	  rrc->SetLteEnbCphySapProvider (phy->GetLteEnbCphySapProvider ());

	  //Ptr<LteEnbNetDevice> dev = m_enbNetDeviceFactory.Create<LteEnbNetDevice> ();
	  Ptr<LteEnbNetDevice> dev = m_enbNetDeviceFactory.Create<LteEnbNetDevice> ();
	  dev->SetNode (n);
	  dev->SetAttribute ("LteEnbPhy", PointerValue (phy));
	  dev->SetAttribute ("LteEnbMac", PointerValue (mac));
	  dev->SetAttribute ("FfMacScheduler", PointerValue (sched));
	  dev->SetAttribute ("LteEnbRrc", PointerValue (rrc));

	  phy->SetDevice (dev);
	  dlPhy->SetDevice (dev);
	  ulPhy->SetDevice (dev);

	  n->AddDevice (dev);
	  ulPhy->SetLtePhyRxDataEndOkCallback (MakeCallback (&LteEnbPhy::PhyPduReceived, phy));
	  ulPhy->SetLtePhyRxCtrlEndOkCallback (MakeCallback (&LteEnbPhy::ReceiveLteControlMessageList, phy));
	  ulPhy->SetLtePhyUlHarqFeedbackCallback (MakeCallback (&LteEnbPhy::ReceiveLteUlHarqFeedback, phy));
	  rrc->SetForwardUpCallback (MakeCallback (&LteEnbNetDevice::Receive, dev));

	  NS_LOG_LOGIC ("set the propagation model frequencies");
	  double dlFreq = LteSpectrumValueHelper::GetCarrierFrequency (dev->GetDlEarfcn ());
	  NS_LOG_LOGIC ("DL freq: " << dlFreq);
	  bool dlFreqOk = m_downlinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (dlFreq));
	  if (!dlFreqOk)
	  {
		  NS_LOG_WARN ("DL propagation model does not have a Frequency attribute");
	  }
	  double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency (dev->GetUlEarfcn ());
	  NS_LOG_LOGIC ("UL freq: " << ulFreq);
	  bool ulFreqOk = m_uplinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq));
	  if (!ulFreqOk)
	  {
		  NS_LOG_WARN ("UL propagation model does not have a Frequency attribute");
	  }
	  std::cout<<"Jus checking device's address: "<<dev<<std::endl;
	  std::cout<<"LTE-A is gonna call StartCA now "<<std::endl;
	  dev->Start();
	  //dev->StartCA (maxCCs);
	  //dev->StartCA (maxCCs);
	  m_uplinkChannelCA.at(i)->AddRx(ulPhy);
	  NS_LOG_INFO ("adding this eNB to the EPC");
	  devvtr.push_back(dev);

	  if(m_epcHelperCA != 0)
	  {
		  m_epcHelperCA->AddEnbCA(n,dev,maxCCs);
		  Ptr<EpcEnbApplication> enbApp = n->GetApplication (i)->GetObject<EpcEnbApplication> ();
		  NS_ASSERT_MSG (enbApp != 0, "cannot retrieve EpcEnbApplication");

      // S1 SAPs
		  rrc->SetS1SapProvider (enbApp->GetS1SapProvider ());
		  enbApp->SetS1SapUser (rrc->GetS1SapUser ());

	  }

      // X2 SAPs
	    Ptr<EpcX2> x2 = n->GetObject<EpcX2> ();
	    x2->SetEpcX2SapUser (rrc->GetEpcX2SapUser ());
	    rrc->SetEpcX2SapProvider (x2->GetEpcX2SapProvider ());

  }
  return devvtr;
}

std::vector<Ptr<NetDevice> >
LteAdvancedHelper::InstallMultipleUeDevice (Ptr<Node> n)
{
  std::vector<Ptr<NetDevice> > devvtr;
  NS_LOG_FUNCTION (this);
  for(unsigned int i = 0 ; i < maxCCs ; i++)
  {
	  Ptr<LteSpectrumPhy> dlPhy = CreateObject<LteSpectrumPhy> ();
	  Ptr<LteSpectrumPhy> ulPhy = CreateObject<LteSpectrumPhy> ();

	  Ptr<LteUePhy> phy = CreateObject<LteUePhy> (dlPhy, ulPhy);

	  Ptr<LteHarqPhy> harq = Create<LteHarqPhy> ();
	  dlPhy->SetHarqPhyModule (harq);
	  ulPhy->SetHarqPhyModule (harq);
	  phy->SetHarqPhyModule (harq);

	  Ptr<LteRsReceivedPowerChunkProcessor> pRs = Create<LteRsReceivedPowerChunkProcessor> (phy->GetObject<LtePhy> ());
	  dlPhy->AddRsPowerChunkProcessor (pRs);

	  Ptr<LteCtrlSinrChunkProcessor> pCtrl = Create<LteCtrlSinrChunkProcessor> (phy->GetObject<LtePhy> (), dlPhy);
	  dlPhy->AddCtrlSinrChunkProcessor (pCtrl);

	  Ptr<LteDataSinrChunkProcessor> pData = Create<LteDataSinrChunkProcessor> (dlPhy);
	  dlPhy->AddDataSinrChunkProcessor (pData);

	  dlPhy->SetChannel (m_downlinkChannelCA.at(i));
	  ulPhy->SetChannel (m_uplinkChannelCA.at(i));

	  Ptr<MobilityModel> mm = n->GetObject<MobilityModel> ();
	  NS_ASSERT_MSG (mm, "MobilityModel needs to be set on node before calling LteHelper::InstallUeDevice ()");
	  dlPhy->SetMobility (mm);
	  ulPhy->SetMobility (mm);

	  Ptr<AntennaModel> antenna = (m_ueAntennaModelFactory.Create ())->GetObject<AntennaModel> ();
	  NS_ASSERT_MSG (antenna, "error in creating the AntennaModel object");
	  dlPhy->SetAntenna (antenna);
	  ulPhy->SetAntenna (antenna);

	  Ptr<LteUeMac> mac = CreateObject<LteUeMac> ();
	  Ptr<LteUeRrc> rrc = CreateObject<LteUeRrc> ();
	  Ptr<EpcUeNas> nas = CreateObject<EpcUeNas> ();

	  nas->SetEpcHelper (m_epcHelperCA);

  // connect SAPs
	  nas->SetAsSapProvider (rrc->GetAsSapProvider ());
	  rrc->SetAsSapUser (nas->GetAsSapUser ());

	  rrc->SetLteUeCmacSapProvider (mac->GetLteUeCmacSapProvider ());
	  mac->SetLteUeCmacSapUser (rrc->GetLteUeCmacSapUser ());
	  rrc->SetLteMacSapProvider (mac->GetLteMacSapProvider ());

	  phy->SetLteUePhySapUser (mac->GetLteUePhySapUser ());
	  mac->SetLteUePhySapProvider (phy->GetLteUePhySapProvider ());

	  phy->SetLteUeCphySapUser (rrc->GetLteUeCphySapUser ());
	  rrc->SetLteUeCphySapProvider (phy->GetLteUeCphySapProvider ());

	  Ptr<LteUeNetDevice> dev = CreateObject<LteUeNetDevice> (n, phy, mac, rrc, nas, maxCCs);
	  phy->SetDevice (dev);
	  dlPhy->SetDevice (dev);
	  ulPhy->SetDevice (dev);
	  nas->SetDevice (dev);

	  n->AddDevice (dev);
	  dlPhy->SetLtePhyRxDataEndOkCallback (MakeCallback (&LteUePhy::PhyPduReceived, phy));
	  dlPhy->SetLtePhyRxCtrlEndOkCallback (MakeCallback (&LteUePhy::ReceiveLteControlMessageList, phy));
	  dlPhy->SetLtePhyDlHarqFeedbackCallback (MakeCallback (&LteUePhy::ReceiveLteDlHarqFeedback, phy));
	  nas->SetForwardUpCallback (MakeCallback (&LteUeNetDevice::Receive, dev));

	  dev->Start();
	  devvtr.push_back(dev);
  }

  return devvtr;
}

void
LteAdvancedHelper::AttachCA (std::vector<NetDeviceContainer> ueDevices, NetDeviceContainer enbDevices)
{
	NS_LOG_FUNCTION(this);
	for(unsigned int i = 0 ; i < ueDevices.size() ; i++)
	{
                unsigned int k = 0;
                std::cout<<"Iteration "<<(i+1)<<std::endl;
		for (NetDeviceContainer::Iterator iterue = ueDevices.at(i).Begin() , iterenb = enbDevices.Begin(); iterue != ueDevices.at(i).End(); ++iterue , ++iterenb )
                {
//                        Ptr<LteUeNetDevice> lteUe = DynamicCast<LteUeNetDevice>(*iterue);
//                        NetDeviceContainer::Iterator iterenb;
//                        unsigned int p = 0;
//                        for (iterenb = enbDevices.Begin(); p < k; ++iterenb,++p);
                        AttachCA(*iterue , *iterenb, k);

/*                        for(iterenb = enbDevices.Begin(); iterenb != enbDevices.End(); ++iterenb)
                        {
                                Ptr<LteEnbNetDevice> lteEnb = DynamicCast<LteEnbNetDevice>(*iterenb);
                                if(LteSpectrumValueHelper::GetCarrierFrequency (lteEnb->GetDlEarfcn ()) == LteSpectrumValueHelper::GetCarrierFrequency (lteUe->GetUlEarfcn ()))
                                {
        		               AttachCA(*iterue , *iterenb, p);
                                       break;
                                }
                                p++;
                        }
                        
                        if(iterenb == enbDevices.End())
                        {
                                unsigned int p = 0;
                                for (iterenb = enbDevices.Begin(); p < k; ++iterenb,++p);
                                AttachCA(*iterue , *iterenb, k);
                        } */

                        k++;
                }
	}
}

void
LteAdvancedHelper::AttachCA (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice, unsigned int index)
{
  NS_LOG_FUNCTION (this);
  //enbRrc->SetCellId (enbDevice->GetObject<LteEnbNetDevice> ()->GetCellId ());

  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  Ptr<EpcUeNas> ueNas = ueLteDevice->GetNas ();
  //ueNas->ConnectCA (enbDevice, maxCCs);
  ueNas->ConnectCA(enbDevice);
  
  m_downlinkChannelCA.at(index)->AddRx (ueLteDevice->GetPhy ()->GetDownlinkSpectrumPhy ());

  // tricks needed for the simplified LTE-only simulations 
  if (m_epcHelperCA == 0)
    {
      ueDevice->GetObject<LteUeNetDevice> ()->SetTargetEnb (enbDevice->GetObject<LteEnbNetDevice> ());
    }
}

/*void
LteAdvancedHelper::AttachCA (Ptr<NetDevice> ueDevice, NetDeviceContainer enbDevices)
{
  NS_LOG_FUNCTION (this);
  //enbRrc->SetCellId (enbDevice->GetObject<LteEnbNetDevice> ()->GetCellId ());

  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  Ptr<EpcUeNas> ueNas = ueLteDevice->GetNas ();
  uint8_t k = 0;
  for(NetDeviceContainer::Iterator iter = enbDevices.Begin() ; iter != enbDevices.End(); ++iter)
  {
	  ueNas->ConnectCA (*iter, maxCCs);
	  m_downlinkChannelCA.at(k)->AddRx (ueLteDevice->GetPhy ()->GetDownlinkSpectrumPhy ());

  // tricks needed for the simplified LTE-only simulations
	  if (m_epcHelperCA == 0)
	  {
		  ueDevice->GetObject<LteUeNetDevice> ()->SetTargetEnb ((*iter)->GetObject<LteEnbNetDevice> ());
	  }
          k++;
  }
}
*/

void
LteAdvancedHelper::ActivateDedicatedEpsBearerCA (std::vector<NetDeviceContainer> ueDevices, EpsBearer bearer, Ptr<EpcTft> tft)
{
  NS_LOG_FUNCTION (this);
  //for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
  for(unsigned int i = 0 ; i < ueDevices.size() ; i++)
    {
	  for(NetDeviceContainer::Iterator iter = ueDevices.at(i).Begin() ; iter != ueDevices.at(i).End(); ++iter)
      ActivateDedicatedEpsBearerCA (*iter, bearer, tft);
    }
}


void
LteAdvancedHelper::ActivateDedicatedEpsBearerCA (Ptr<NetDevice> ueDevice, EpsBearer bearer, Ptr<EpcTft> tft)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_epcHelperCA != 0, "dedicated EPS bearers cannot be set up when EPC is not used");

  ueDevice->GetObject<LteUeNetDevice> ()->ActivateDedicatedEpsBearer (bearer, tft);

}

void
LteAdvancedHelper::ActivateDataRadioBearerCA (std::vector<NetDeviceContainer> ueDevices, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);
  for(unsigned int i = 0 ; i < ueDevices.size() ; i++)
    ActivateDataRadioBearerCA (ueDevices.at(i), bearer);

}

void
LteAdvancedHelper::ActivateDataRadioBearerCA (NetDeviceContainer ueDeviceSet, EpsBearer bearer)
{
  //NS_LOG_FUNCTION (this << ueDeviceSet);
  NS_ASSERT_MSG (m_epcHelperCA != 0, "this method must not be used when EPC is being used");
  for (NetDeviceContainer::Iterator iter = ueDeviceSet.Begin (); iter != ueDeviceSet.End (); ++iter)
  {
	  Ptr<LteEnbNetDevice> enbDevice = (*iter)->GetObject<LteUeNetDevice> ()->GetTargetEnb ();
	  Ptr<LteEnbRrc> enbRrc = enbDevice->GetObject<LteEnbNetDevice> ()->GetRrc ();
	  EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters params;
	  params.rnti = (*iter)->GetObject<LteUeNetDevice> ()->GetRrc ()->GetRnti();
	  params.bearer = bearer;
	  params.teid = 0; // don't care
	  enbRrc->GetS1SapUser ()->DataRadioBearerSetupRequest (params);
  }
}

void
LteAdvancedHelper::EnableLogComponents (void)
{
  LogComponentEnable ("LteAdvancedHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUeRrc", LOG_LEVEL_ALL);
  LogComponentEnable ("LteEnbMac", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUeMac", LOG_LEVEL_ALL);
  LogComponentEnable ("LteRlc", LOG_LEVEL_ALL);
  LogComponentEnable ("LteRlcUm", LOG_LEVEL_ALL);
  LogComponentEnable ("LteRlcAm", LOG_LEVEL_ALL);
  LogComponentEnable ("RrFfMacScheduler", LOG_LEVEL_ALL);
  LogComponentEnable ("PfFfMacScheduler", LOG_LEVEL_ALL);

  LogComponentEnable ("LtePhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LteEnbPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUePhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LteSpectrumValueHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LteSpectrumPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LteInterference", LOG_LEVEL_ALL);
  LogComponentEnable ("LteSinrChunkProcessor", LOG_LEVEL_ALL);

  std::string propModelStr = m_dlPathlossModelFactory.GetTypeId ().GetName ().erase (0,5).c_str ();
  LogComponentEnable ("LteNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("LteUeNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("LteEnbNetDevice", LOG_LEVEL_ALL);

  LogComponentEnable ("RadioBearerStatsCalculator", LOG_LEVEL_ALL);
  LogComponentEnable ("MacStatsCalculator", LOG_LEVEL_ALL);
  LogComponentEnable ("PhyStatsCalculator", LOG_LEVEL_ALL);
  LogComponentEnable ("Config", LOG_LEVEL_ALL);


}

void
LteAdvancedHelper::EnableTracesAdvanced (void)
{
  EnablePhyTracesAdvanced ();
  EnableMacTracesAdvanced ();
  EnableRlcTracesAdvanced ();
  EnablePdcpTracesAdvanced ();
}

void
LteAdvancedHelper::EnableRlcTracesAdvanced (void)
{
  EnableDlRlcTracesAdvanced ();
  EnableUlRlcTracesAdvanced ();
}

int64_t
LteAdvancedHelper::AssignStreams (std::vector<NetDeviceContainer> netDevices, int64_t stream)
{
	int64_t currentStream = stream;
	Ptr<NetDevice> netDevice;
	for(unsigned int i = 0 ; i < netDevices.size() ; i++)
	{
		for(NetDeviceContainer::Iterator iter = netDevices.at(i).Begin(); iter != netDevices.at(i).End(); ++iter)
		{
			netDevice = (*iter);
			Ptr<LteEnbNetDevice> lteEnb = DynamicCast<LteEnbNetDevice> (netDevice);
			if(lteEnb)
			{
				Ptr<LteSpectrumPhy> dlPhy = lteEnb->GetPhy ()->GetDownlinkSpectrumPhy ();
				Ptr<LteSpectrumPhy> ulPhy = lteEnb->GetPhy ()->GetUplinkSpectrumPhy ();
				currentStream += dlPhy->AssignStreams (currentStream);
				currentStream += ulPhy->AssignStreams (currentStream);
			}

			Ptr<LteUeNetDevice> lteUe = DynamicCast<LteUeNetDevice> (netDevice);
			if (lteUe)
			{
			     Ptr<LteSpectrumPhy> dlPhy = lteUe->GetPhy ()->GetDownlinkSpectrumPhy ();
			     Ptr<LteSpectrumPhy> ulPhy = lteUe->GetPhy ()->GetUplinkSpectrumPhy ();
			     currentStream += dlPhy->AssignStreams (currentStream);
			     currentStream += ulPhy->AssignStreams (currentStream);
			}
		}
	}
	return (currentStream - stream);
}

uint64_t
FindImsiFromEnbRlcPathAdvanced (std::string path)
{
  NS_LOG_FUNCTION (path);
  // Sample path input:
  // /NodeList/#NodeId/DeviceList/#DeviceId/LteEnbRrc/UeMap/#C-RNTI/RadioBearerMap/#LCID/LteRlc/RxPDU

  // We retrieve the UeInfo associated to the C-RNTI and perform the IMSI lookup
  std::string ueMapPath = path.substr (0, path.find ("/RadioBearerMap"));
  Config::MatchContainer match = Config::LookupMatches (ueMapPath);

  if (match.GetN () != 0)
    {
      Ptr<Object> ueInfo = match.Get (0);
      NS_LOG_LOGIC ("FindImsiFromEnbRlcPath: " << path << ", " << ueInfo->GetObject<UeInfo> ()->GetImsi ());
      return ueInfo->GetObject<UeInfo> ()->GetImsi ();
    }
  else
    {
      NS_FATAL_ERROR ("Lookup " << ueMapPath << " got no matches");
    }
}

uint64_t
FindImsiFromUePhyAdvanced (std::string path)
{
  NS_LOG_FUNCTION (path);
  // Sample path input:
  // /NodeList/#NodeId/DeviceList/#DeviceId/LteUePhy

  // We retrieve the UeInfo associated to the C-RNTI and perform the IMSI lookup
  std::string ueRlcPath = path.substr (0, path.find ("/LteUePhy"));
  ueRlcPath += "/LteUeRrc";
  Config::MatchContainer match = Config::LookupMatches (ueRlcPath);

  if (match.GetN () != 0)
    {
      Ptr<Object> ueRrc = match.Get (0);
      return ueRrc->GetObject<LteUeRrc> ()->GetImsi ();
    }
  else
    {
      NS_FATAL_ERROR ("Lookup " << ueRlcPath << " got no matches");
    }
  return 0;
}

uint16_t
FindCellIdFromEnbRlcPathAdvanced (std::string path)
{
  NS_LOG_FUNCTION (path);
  // Sample path input:
  // /NodeList/#NodeId/DeviceList/#DeviceId/LteEnbRrc/UeMap/#C-RNTI/RadioBearerMap/#LCID/LteRlc/RxPDU

  // We retrieve the CellId associated to the Enb
  std::string enbNetDevicePath = path.substr (0, path.find ("/LteEnbRrc"));
  Config::MatchContainer match = Config::LookupMatches (enbNetDevicePath);

  if (match.GetN () != 0)
    {
      Ptr<Object> enbNetDevice = match.Get (0);
      NS_LOG_LOGIC ("FindCellIdFromEnbRlcPath: " << path << ", " << enbNetDevice->GetObject<LteEnbNetDevice> ()->GetCellId ());
      return enbNetDevice->GetObject<LteEnbNetDevice> ()->GetCellId ();
    }
  else
    {
      NS_FATAL_ERROR ("Lookup " << enbNetDevicePath << " got no matches");
    }
}

uint64_t
FindImsiFromUeRlcPathAdvanced (std::string path)
{
  NS_LOG_FUNCTION (path);
  // Sample path input:
  // /NodeList/#NodeId/DeviceList/#DeviceId/LteUeRrc/RadioBearer/#LCID/RxPDU

  // We retrieve the LteUeNetDevice path
  std::string lteUeNetDevicePath = path.substr (0, path.find ("/LteUeRrc"));
  Config::MatchContainer match = Config::LookupMatches (lteUeNetDevicePath);

  if (match.GetN () != 0)
    {
      Ptr<Object> ueNetDevice = match.Get (0);
      NS_LOG_LOGIC ("FindImsiFromUeRlcPath: " << path << ", " << ueNetDevice->GetObject<LteUeNetDevice> ()->GetImsi ());
      return ueNetDevice->GetObject<LteUeNetDevice> ()->GetImsi ();
    }
  else
    {
      NS_FATAL_ERROR ("Lookup " << lteUeNetDevicePath << " got no matches");
    }

}

uint16_t
FindCellIdFromUeRlcPathAdvanced (std::string path)
{
  NS_LOG_FUNCTION (path);
  // Sample path input:
  // /NodeList/#NodeId/DeviceList/#DeviceId/LteUeRrc/RadioBearer/#LCID/RxPDU

  // We retrieve the LteUeNetDevice path
  std::string lteUeNetDevicePath = path.substr (0, path.find ("/LteUeRrc"));
  Config::MatchContainer match = Config::LookupMatches (lteUeNetDevicePath);

  if (match.GetN () != 0)
    {
      Ptr<Object> ueNetDevice = match.Get (0);
      NS_LOG_LOGIC ("FindImsiFromUeRlcPath: " << path << ", " << ueNetDevice->GetObject<LteUeNetDevice> ()->GetImsi ());
      return ueNetDevice->GetObject<LteUeNetDevice> ()->GetRrc ()->GetCellId ();
    }
  else
    {
      NS_FATAL_ERROR ("Lookup " << lteUeNetDevicePath << " got no matches");
    }

}


uint64_t
FindImsiFromEnbMacAdvanced (std::string path, uint16_t rnti)
{
  NS_LOG_FUNCTION (path << rnti);
  // /NodeList/#NodeId/DeviceList/#DeviceId/LteEnbMac/DlScheduling
  std::ostringstream oss;
  std::string p = path.substr (0, path.find ("/LteEnbMac"));
  oss << rnti;
  p += "/LteEnbRrc/UeMap/" + oss.str ();
  uint64_t imsi = FindImsiFromEnbRlcPathAdvanced (p);
  NS_LOG_LOGIC ("FindImsiFromEnbMac: " << path << ", " << rnti << ", " << imsi);
  return imsi;
}

uint16_t
FindCellIdFromEnbMacAdvanced (std::string path, uint16_t rnti)
{
  NS_LOG_FUNCTION (path << rnti);
  // /NodeList/#NodeId/DeviceList/#DeviceId/LteEnbMac/DlScheduling
  std::ostringstream oss;
  std::string p = path.substr (0, path.find ("/LteEnbMac"));
  oss << rnti;
  p += "/LteEnbRrc/UeMap/" + oss.str ();
  uint16_t cellId = FindCellIdFromEnbRlcPathAdvanced (p);
  NS_LOG_LOGIC ("FindCellIdFromEnbMac: " << path << ", "<< rnti << ", " << cellId);
  return cellId;
}


void
DlTxPduCallbackAdvanced (Ptr<RadioBearerStatsCalculator> rlcStats, std::string path,
                 uint16_t rnti, uint8_t lcid, uint32_t packetSize)
{
  NS_LOG_FUNCTION (rlcStats << path << rnti << (uint16_t)lcid << packetSize);
  uint64_t imsi = 0;
  if (rlcStats->ExistsImsiPath (path) == true)
    {
      imsi = rlcStats->GetImsiPath (path);
    }
  else
    {
      imsi = FindImsiFromEnbRlcPathAdvanced (path);
      rlcStats->SetImsiPath (path, imsi);
    }
  uint16_t cellId = 0;
  if (rlcStats->ExistsCellIdPath (path) == true)
    {
      cellId = rlcStats->GetCellIdPath (path);
    }
  else
    {
      cellId = FindCellIdFromEnbRlcPathAdvanced (path);
      rlcStats->SetCellIdPath (path, cellId);
    }
  rlcStats->DlTxPdu (cellId, imsi, rnti, lcid, packetSize);
}

void
DlRxPduCallbackAdvanced (Ptr<RadioBearerStatsCalculator> rlcStats, std::string path,
                 uint16_t rnti, uint8_t lcid, uint32_t packetSize, uint64_t delay)
{
  NS_LOG_FUNCTION (rlcStats << path << rnti << (uint16_t)lcid << packetSize << delay);
  uint64_t imsi = 0;
  if (rlcStats->ExistsImsiPath (path) == true)
    {
      imsi = rlcStats->GetImsiPath (path);
    }
  else
    {
      imsi = FindImsiFromUeRlcPathAdvanced (path);
      rlcStats->SetImsiPath (path, imsi);
    }

  uint16_t cellId = 0;
  if (rlcStats->ExistsCellIdPath (path) == true)
    {
      cellId = rlcStats->GetCellIdPath (path);
    }
  else
    {
      cellId = FindCellIdFromUeRlcPathAdvanced (path);
      rlcStats->SetCellIdPath (path, cellId);
    }
  rlcStats->DlRxPdu (cellId, imsi, rnti, lcid, packetSize, delay);
}

void
LteAdvancedHelper::EnableDlRlcTracesAdvanced (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/RadioBearerMap/*/LteRlc/TxPDU",
                   MakeBoundCallback (&DlTxPduCallbackAdvanced, m_rlcStats));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/RadioBearerMap/*/LteRlc/RxPDU",
                   MakeBoundCallback (&DlRxPduCallbackAdvanced, m_rlcStats));
}

void
UlTxPduCallbackAdvanced (Ptr<RadioBearerStatsCalculator> rlcStats, std::string path,
                 uint16_t rnti, uint8_t lcid, uint32_t packetSize)
{
  NS_LOG_FUNCTION (rlcStats << path << rnti << (uint16_t)lcid << packetSize);
  uint64_t imsi = 0;
  if (rlcStats->ExistsImsiPath (path) == true)
    {
      imsi = rlcStats->GetImsiPath (path);
    }
  else
    {
      imsi = FindImsiFromUeRlcPathAdvanced (path);
      rlcStats->SetImsiPath (path, imsi);
    }

  uint16_t cellId = 0;
  if (rlcStats->ExistsCellIdPath (path) == true)
    {
      cellId = rlcStats->GetCellIdPath (path);
    }
  else
    {
      cellId = FindCellIdFromUeRlcPathAdvanced (path);
      rlcStats->SetCellIdPath (path, cellId);
    }
  rlcStats->UlTxPdu (cellId, imsi, rnti, lcid, packetSize);
}

void
UlRxPduCallbackAdvanced (Ptr<RadioBearerStatsCalculator> rlcStats, std::string path,
                 uint16_t rnti, uint8_t lcid, uint32_t packetSize, uint64_t delay)
{
  NS_LOG_FUNCTION (rlcStats << path << rnti << (uint16_t)lcid << packetSize << delay);
  uint64_t imsi = 0;
  if (rlcStats->ExistsImsiPath (path) == true)
    {
      imsi = rlcStats->GetImsiPath (path);
    }
  else
    {
      imsi = FindImsiFromEnbRlcPathAdvanced (path);
      rlcStats->SetImsiPath (path, imsi);
    }
  uint16_t cellId = 0;
  if (rlcStats->ExistsCellIdPath (path) == true)
    {
      cellId = rlcStats->GetCellIdPath (path);
    }
  else
    {
      cellId = FindCellIdFromEnbRlcPathAdvanced (path);
      rlcStats->SetCellIdPath (path, cellId);
    }
  rlcStats->UlRxPdu (cellId, imsi, rnti, lcid, packetSize, delay);
}


void
DlSchedulingCallbackAdvanced (Ptr<MacStatsCalculator> macStats,
                      std::string path, uint32_t frameNo, uint32_t subframeNo,
                      uint16_t rnti, uint8_t mcsTb1, uint16_t sizeTb1,
                      uint8_t mcsTb2, uint16_t sizeTb2)
{
  NS_LOG_FUNCTION (macStats << path);
  uint64_t imsi = 0;
  std::ostringstream pathAndRnti;
  pathAndRnti << path << "/" << rnti;
  if (macStats->ExistsImsiPath (pathAndRnti.str ()) == true)
    {
      imsi = macStats->GetImsiPath (pathAndRnti.str ());
    }
  else
    {
      imsi = FindImsiFromEnbMacAdvanced (path, rnti);
      macStats->SetImsiPath (pathAndRnti.str (), imsi);
    }

  uint16_t cellId = 0;
  if (macStats->ExistsCellIdPath (pathAndRnti.str ()) == true)
    {
      cellId = macStats->GetCellIdPath (pathAndRnti.str ());
    }
  else
    {
      cellId = FindCellIdFromEnbMacAdvanced (path, rnti);
      macStats->SetCellIdPath (pathAndRnti.str (), cellId);
    }

  macStats->DlScheduling (cellId, imsi, frameNo, subframeNo, rnti, mcsTb1, sizeTb1, mcsTb2, sizeTb2);
}

void
LteAdvancedHelper::EnableUlRlcTracesAdvanced (void)
{
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/RadioBearerMap/*/LteRlc/TxPDU",
                   MakeBoundCallback (&UlTxPduCallbackAdvanced, m_rlcStats));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/RadioBearerMap/*/LteRlc/RxPDU",
                   MakeBoundCallback (&UlRxPduCallbackAdvanced, m_rlcStats));
}

void
LteAdvancedHelper::EnableMacTracesAdvanced (void)
{
  EnableDlMacTracesAdvanced ();
  EnableUlMacTracesAdvanced ();
}


void
LteAdvancedHelper::EnableDlMacTracesAdvanced (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbMac/DlScheduling",
                   MakeBoundCallback (&DlSchedulingCallbackAdvanced, m_macStats));
}

void
UlSchedulingCallbackAdvanced (Ptr<MacStatsCalculator> macStats, std::string path,
                      uint32_t frameNo, uint32_t subframeNo, uint16_t rnti,
                      uint8_t mcs, uint16_t size)
{
  NS_LOG_FUNCTION (macStats << path);

  uint64_t imsi = 0;
  std::ostringstream pathAndRnti;
  pathAndRnti << path << "/" << rnti;
  if (macStats->ExistsImsiPath (pathAndRnti.str ()) == true)
    {
      imsi = macStats->GetImsiPath (pathAndRnti.str ());
    }
  else
    {
      imsi = FindImsiFromEnbMacAdvanced (path, rnti);
      macStats->SetImsiPath (pathAndRnti.str (), imsi);
    }
  uint16_t cellId = 0;
  if (macStats->ExistsCellIdPath (pathAndRnti.str ()) == true)
    {
      cellId = macStats->GetCellIdPath (pathAndRnti.str ());
    }
  else
    {
      cellId = FindCellIdFromEnbMacAdvanced (path, rnti);
      macStats->SetCellIdPath (pathAndRnti.str (), cellId);
    }

  macStats->UlScheduling (cellId, imsi, frameNo, subframeNo, rnti, mcs, size);
}

void
LteAdvancedHelper::EnableUlMacTracesAdvanced (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbMac/UlScheduling",
                   MakeBoundCallback (&UlSchedulingCallbackAdvanced, m_macStats));
}

void
LteAdvancedHelper::EnablePhyTracesAdvanced (void)
{
  EnableDlPhyTracesAdvanced ();
  EnableUlPhyTracesAdvanced ();
}


void
ReportCurrentCellRsrpSinrCallbackAdvanced (Ptr<PhyStatsCalculator> phyStats,
                      std::string path, uint16_t cellId, uint16_t rnti,
                      double rsrp, double sinr)
{
  NS_LOG_FUNCTION (phyStats << path);
  uint64_t imsi = 0;
  std::string pathUePhy  = path.substr (0, path.find ("/ReportCurrentCellRsrpSinr"));
  if (phyStats->ExistsImsiPath (pathUePhy) == true)
    {
      imsi = phyStats->GetImsiPath (pathUePhy);
    }
  else
    {
      imsi = FindImsiFromUePhyAdvanced (pathUePhy);
      phyStats->SetImsiPath (pathUePhy, imsi);
    }

  phyStats->ReportCurrentCellRsrpSinr (cellId, imsi, rnti, rsrp,sinr);
}

void
LteAdvancedHelper::EnableDlPhyTracesAdvanced (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Config::Connect ("/NodeList/*/DeviceList/*/LteUePhy/ReportCurrentCellRsrpSinr",
                   MakeBoundCallback (&ReportCurrentCellRsrpSinrCallbackAdvanced, m_phyStats));
}

void
ReportUeSinrAdvanced (Ptr<PhyStatsCalculator> phyStats, std::string path,
              uint16_t cellId, uint16_t rnti, double sinrLinear)
{
  NS_LOG_FUNCTION (phyStats << path);

  uint64_t imsi = 0;
  std::ostringstream pathAndRnti;
  pathAndRnti << path << "/" << rnti;
  std::string pathEnbMac  = path.substr (0, path.find ("LteEnbPhy/ReportUeSinr"));
  pathEnbMac += "LteEnbMac/DlScheduling";
  if (phyStats->ExistsImsiPath (pathAndRnti.str ()) == true)
    {
      imsi = phyStats->GetImsiPath (pathAndRnti.str ());
    }
  else
    {
      imsi = FindImsiFromEnbMacAdvanced (pathEnbMac, rnti);
      phyStats->SetImsiPath (pathAndRnti.str (), imsi);
    }

  phyStats->ReportUeSinr (cellId, imsi, rnti, sinrLinear);
}

void
ReportInterferenceAdvanced (Ptr<PhyStatsCalculator> phyStats, std::string path,
                    uint16_t cellId, Ptr<SpectrumValue> interference)
{
  NS_LOG_FUNCTION (phyStats << path);
  phyStats->ReportInterference (cellId, interference);
}

void
LteAdvancedHelper::EnableUlPhyTracesAdvanced (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbPhy/ReportUeSinr",
                   MakeBoundCallback (&ReportUeSinrAdvanced, m_phyStats));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbPhy/ReportInterference",
                   MakeBoundCallback (&ReportInterferenceAdvanced, m_phyStats));

}

Ptr<RadioBearerStatsCalculator>
LteAdvancedHelper::GetRlcStatsAdvanced (void)
{
  return m_rlcStats;
}

void
LteAdvancedHelper::EnablePdcpTracesAdvanced (void)
{
  EnableDlPdcpTracesAdvanced ();
  EnableUlPdcpTracesAdvanced ();
}

void
LteAdvancedHelper::EnableDlPdcpTracesAdvanced (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/RadioBearerMap/*/LtePdcp/TxPDU",
                   MakeBoundCallback (&DlTxPduCallbackAdvanced, m_pdcpStats));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/RadioBearerMap/*/LtePdcp/RxPDU",
                   MakeBoundCallback (&DlRxPduCallbackAdvanced, m_pdcpStats));
}

void
LteAdvancedHelper::EnableUlPdcpTracesAdvanced (void)
{
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/RadioBearerMap/*/LtePdcp/TxPDU",
                   MakeBoundCallback (&UlTxPduCallbackAdvanced, m_pdcpStats));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/RadioBearerMap/*/LtePdcp/RxPDU",
                   MakeBoundCallback (&UlRxPduCallbackAdvanced, m_pdcpStats));
}

Ptr<RadioBearerStatsCalculator>
LteAdvancedHelper::GetPdcpStatsAdvanced (void)
{
  return m_pdcpStats;
}

} // namespace ns3
