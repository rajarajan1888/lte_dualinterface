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
 *         Marco Miozzo <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <math.h>
#include <ns3/simulator.h>
#include <ns3/double.h>
#include "lte-ue-phy.h"
#include "lte-enb-phy.h"
#include "lte-net-device.h"
#include "lte-ue-net-device.h"
#include "lte-enb-net-device.h"
#include "lte-spectrum-value-helper.h"
#include "lte-amc.h"
#include "lte-ue-mac.h"
#include "ff-mac-common.h"
#include "lte-sinr-chunk-processor.h"
#include <ns3/lte-mi-error-model.h>
#include <ns3/lte-common.h>

#include <iostream>
#include <fstream>

NS_LOG_COMPONENT_DEFINE ("LteUePhy");

namespace ns3 {




// duration of data portion of UL subframe
// = TTI - 1 symbol for SRS - 1ns as margin to avoid overlapping simulator events
// (symbol duration in nanoseconds = TTI / 14 (rounded))
// in other words, duration of data portion of UL subframe = TTI*(13/14) -1ns
static const Time UL_DATA_DURATION = NanoSeconds (1e6 - 71429 - 1); 

// delay from subframe start to transmission of SRS 
// = TTI - 1 symbol for SRS 
static const Time UL_SRS_DELAY_FROM_SUBFRAME_START = NanoSeconds (1e6 - 71429); 




////////////////////////////////////////
// member SAP forwarders
////////////////////////////////////////


class UeMemberLteUePhySapProvider : public LteUePhySapProvider
{
public:
  UeMemberLteUePhySapProvider (LteUePhy* phy);

  // inherited from LtePhySapProvider
  virtual void SendMacPdu (Ptr<Packet> p);
  virtual void SendLteControlMessage (Ptr<LteControlMessage> msg);
  virtual void SetSrsConfigurationIndex (uint16_t srcCi);

private:
  LteUePhy* m_phy;
};

UeMemberLteUePhySapProvider::UeMemberLteUePhySapProvider (LteUePhy* phy) : m_phy (phy)
{

}


void
UeMemberLteUePhySapProvider::SendMacPdu (Ptr<Packet> p)
{
  m_phy->DoSendMacPdu (p);
}

void
UeMemberLteUePhySapProvider::SendLteControlMessage (Ptr<LteControlMessage> msg)
{
  m_phy->DoSendLteControlMessage (msg);
}

void
UeMemberLteUePhySapProvider::SetSrsConfigurationIndex (uint16_t   srcCi)
{
  m_phy->DoSetSrsConfigurationIndex (srcCi);
}


////////////////////////////////////////
// LteUePhy methods
////////////////////////////////////////


NS_OBJECT_ENSURE_REGISTERED (LteUePhy);


LteUePhy::LteUePhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

LteUePhy::LteUePhy (Ptr<LteSpectrumPhy> dlPhy, Ptr<LteSpectrumPhy> ulPhy)
  : LtePhy (dlPhy, ulPhy),
    m_p10CqiPeriocity (MilliSeconds (1)),
    // ideal behavior
    m_p10CqiLast (MilliSeconds (0)),
    m_a30CqiPeriocity (MilliSeconds (1)),
    // ideal behavior
    m_a30CqiLast (MilliSeconds (0)),
    m_uePhySapUser (0),
    m_ueCphySapUser (0),
    m_rnti (0),
    m_srsPeriodicity (0),
    m_rsReceivedPowerUpdated (false),
    m_rsrpSinrSampleCounter (0)
{
  m_amc = CreateObject <LteAmc> ();
  m_uePhySapProvider = new UeMemberLteUePhySapProvider (this);
  m_ueCphySapProvider = new MemberLteUeCphySapProvider<LteUePhy> (this);
  m_macChTtiDelay = UL_PUSCH_TTIS_DELAY;
  for (int i = 0; i < m_macChTtiDelay; i++)
    {
      Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
      m_packetBurstQueue.push_back (pb);
      std::list<Ptr<LteControlMessage> > l;
      m_controlMessagesQueue.push_back (l);
    }
  std::vector <int> ulRb;
  m_subChannelsForTransmissionQueue.resize (m_macChTtiDelay, ulRb);

  NS_ASSERT_MSG (Simulator::Now ().GetNanoSeconds () == 0,
                 "Cannot create UE devices after simulation started");
  Simulator::ScheduleNow (&LteUePhy::SubframeIndication, this, 1, 1);
}


LteUePhy::~LteUePhy ()
{
  m_txModeGain.clear ();
}

void
LteUePhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_uePhySapProvider;
  delete m_ueCphySapProvider;
  LtePhy::DoDispose ();
}

// sleep

TypeId
LteUePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteUePhy")
    .SetParent<LtePhy> ()
    .AddConstructor<LteUePhy> ()
    .AddAttribute ("TxPower",
                   "Transmission power in dBm",
                 //  DoubleValue (47.78),
                   DoubleValue (13.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxPower, 
                                       &LteUePhy::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0.\" "
                   "In this model, we consider T0 = 290K.",
                   DoubleValue (9.0),
                   MakeDoubleAccessor (&LteUePhy::SetNoiseFigure, 
                                       &LteUePhy::GetNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode1Gain",
                  "Transmission mode 1 gain in dB",
                  DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode1Gain                       ),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode2Gain",
                    "Transmission mode 2 gain in dB",
                    DoubleValue (4.2),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode2Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode3Gain",
                    "Transmission mode 3 gain in dB",
                    DoubleValue (-2.8),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode3Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode4Gain",
                    "Transmission mode 4 gain in dB",
                    DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode4Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode5Gain",
                  "Transmission mode 5 gain in dB",
                  DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode5Gain                       ),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode6Gain",
                    "Transmission mode 6 gain in dB",
                    DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode6Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode7Gain",
                  "Transmission mode 7 gain in dB",
                  DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode7Gain                       ),
                  MakeDoubleChecker<double> ())
    .AddTraceSource ("ReportCurrentCellRsrpSinr",
                     "RSRP and SINR statistics.",
                     MakeTraceSourceAccessor (&LteUePhy::m_reportCurrentCellRsrpSinrTrace))
    .AddTraceSource ("ReportCurrentCellTbs",
                     "TBS statistics.",
                     MakeTraceSourceAccessor (&LteUePhy::m_reportCurrentCellTbsTrace))
    .AddAttribute ("RsrpSinrSamplePeriod",
                   "The sampling period for reporting RSRP-SINR stats (default value 1)",
                   UintegerValue (1),
                   MakeUintegerAccessor (&LteUePhy::m_rsrpSinrSamplePeriod),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
LteUePhy::DoStart ()
{
  NS_LOG_FUNCTION (this);
  LtePhy::DoStart ();
}

void
LteUePhy::SetLteUePhySapUser (LteUePhySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_uePhySapUser = s;
}

LteUePhySapProvider*
LteUePhy::GetLteUePhySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_uePhySapProvider);
}


void
LteUePhy::SetLteUeCphySapUser (LteUeCphySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_ueCphySapUser = s;
}

LteUeCphySapProvider*
LteUePhy::GetLteUeCphySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_ueCphySapProvider);
}

void
LteUePhy::SetNoiseFigure (double nf)
{
  NS_LOG_FUNCTION (this << nf);
  m_noiseFigure = nf;
}

double
LteUePhy::GetNoiseFigure () const
{
  NS_LOG_FUNCTION (this);
  return m_noiseFigure;
}

void
LteUePhy::SetTxPower (double pow)
{
  NS_LOG_FUNCTION (this << pow);
  m_txPower = pow;
}

double
LteUePhy::GetTxPower () const
{
  NS_LOG_FUNCTION (this);
  return m_txPower;
}


uint8_t
LteUePhy::GetMacChDelay (void) const
{
  return (m_macChTtiDelay);
}

void
LteUePhy::DoSendMacPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  static std::ofstream doSendMac;
  	static int counter = 0;
  	if(counter==0)
  		doSendMac.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//doSendMacPdu.txt");
  	counter++;

  SetMacPdu (p);
}


void
LteUePhy::PhyPduReceived (Ptr<Packet> p)
{
  m_uePhySapUser->ReceivePhyPdu (p);
}

void
LteUePhy::SetSubChannelsForTransmission (std::vector <int> mask)
{
  NS_LOG_FUNCTION (this);

  m_subChannelsForTransmission = mask;

  Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensity ();
  m_uplinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}


void
LteUePhy::SetSubChannelsForReception (std::vector <int> mask)
{
  NS_LOG_FUNCTION (this);
  m_subChannelsForReception = mask;
}


std::vector <int>
LteUePhy::GetSubChannelsForTransmission ()
{
  NS_LOG_FUNCTION (this);
  return m_subChannelsForTransmission;
}


std::vector <int>
LteUePhy::GetSubChannelsForReception ()
{
  NS_LOG_FUNCTION (this);
  return m_subChannelsForReception;
}


Ptr<SpectrumValue>
LteUePhy::CreateTxPowerSpectralDensity ()
{
  NS_LOG_FUNCTION (this);
  LteSpectrumValueHelper psdHelper;
  Ptr<SpectrumValue> psd = psdHelper.CreateTxPowerSpectralDensity (m_ulEarfcn, m_ulBandwidth, m_txPower, GetSubChannelsForTransmission ());

  return psd;
}

void
LteUePhy::GenerateCtrlCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this);
  static std::ofstream cqireportchecker;
  static int cqireport = 0;

  if(cqireport == 0)
	  cqireportchecker.open("//home//rajarajan//Documents//att_workspace//ltea//ctrlgenerator.txt");
  cqireport++;

  // check periodic wideband CQI
  if (Simulator::Now () > m_p10CqiLast + m_p10CqiPeriocity)
    {
      Ptr<LteUeNetDevice> thisDevice = GetDevice ()->GetObject<LteUeNetDevice> ();
      Ptr<DlCqiLteControlMessage> msg = CreateDlCqiFeedbackMessage (sinr);
      for(uint32_t i = 0 ; i < msg->GetDlCqi().m_wbCqi.size() ; i++)
        cqireportchecker<<this<<" P10 Report checker : "<<(uint16_t)msg->GetDlCqi().m_wbCqi.at(i)<<"\n";
      Ptr<LteControlMessage> ctrlMsg = DynamicCast<LteControlMessage>(msg);
      DoSendLteControlMessage (ctrlMsg);
      m_p10CqiLast = Simulator::Now ();
    }
  // check aperiodic high-layer configured subband CQI
  if  (Simulator::Now () > m_a30CqiLast + m_a30CqiPeriocity)
    {
      Ptr<LteUeNetDevice> thisDevice = GetDevice ()->GetObject<LteUeNetDevice> ();
      Ptr<DlCqiLteControlMessage> msg = CreateDlCqiFeedbackMessage (sinr);
      for(uint32_t i = 0 ; i < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.size() ; i++)
    	 for(uint32_t j = 0 ; j < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.size() ; j++)
       cqireportchecker<<this<<" A30 Report checker : "<<(uint16_t)msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.at(j)<<"\n";
      Ptr<LteControlMessage> ctrlMsg = DynamicCast<LteControlMessage>(msg);
      DoSendLteControlMessage (ctrlMsg);
      m_a30CqiLast = Simulator::Now ();
    }
}

void
LteUePhy::GenerateDataCqiReport (const SpectrumValue& sinr)
{
	double SpectralEfficiencyForMcs[32] = {
	  0.15, 0.19, 0.23, 0.31, 0.38, 0.49, 0.6, 0.74, 0.88, 1.03, 1.18,
	  1.33, 1.48, 1.7, 1.91, 2.16, 2.41, 2.57,
	  2.73, 3.03, 3.32, 3.61, 3.9, 4.21, 4.52, 4.82, 5.12, 5.33, 5.55,
	  0, 0, 0
	};

	double SpectralEfficiencyForCqi[16] = {
	  0.0, // out of range
	  0.15, 0.23, 0.38, 0.6, 0.88, 1.18,
	  1.48, 1.91, 2.41,
	  2.73, 3.32, 3.9, 4.52, 5.12, 5.55
	};
  // Not used by UE, CQI are based only on RS
	static std::ofstream cqidatavals;
	  static int counter = 0;
	  if(counter==0)
		  cqidatavals.open("//home//rajarajan//Documents//att_workspace//ltea//cqidata.txt");
	  cqidatavals<<"CREATE CQI FEEDBACKS\n";
	  counter++;

	 std::vector <int> rbgMap;
	 Values::const_iterator it;
	 std::vector<int> cqi;
	      int rbId = 0;
	      for (it = sinr.ConstValuesBegin (); it != sinr.ConstValuesEnd (); it++)
	      {
	        rbgMap.push_back (rbId++);
	        if ((rbId % m_dlBandwidth == 0)||((it+1)==sinr.ConstValuesEnd ()))
	         {
	            uint8_t mcs = 0;
	            TbStats_t tbStats;
	            while (mcs < 28)
	              {
	                HarqProcessInfoList_t harqInfoList;
	                tbStats = LteMiErrorModel::GetTbDecodificationStats (sinr, rbgMap, (uint16_t)m_amc->GetTbSizeFromMcs (mcs, m_dlBandwidth) / 8, mcs, harqInfoList);
	                if (tbStats.tbler > 0.1)
	                  {
	                    break;
	                  }
	                //cqivals<<"MCS Take 1 : "<<mcs<<"\n";
	                mcs++;

	              }
	            NS_LOG_DEBUG (this << "\t RBG " << rbId << " MCS " << (uint16_t)mcs << " TBLER " << tbStats.tbler);
	            int rbgCqi = 0;
	            if ((tbStats.tbler > 0.1)&&(mcs==0))
	              {
	                rbgCqi = 0; // any MCS can guarantee the 10 % of BER
	              }
	            else if (mcs == 28)
	              {
	                rbgCqi = 15; // all MCSs can guarantee the 10 % of BER
	              }
	            else
	              {
	                double s = SpectralEfficiencyForMcs[mcs];
	                rbgCqi = 0;
	                while ((rbgCqi < 15) && (SpectralEfficiencyForCqi[rbgCqi + 1] < s))
	                {
	                  ++rbgCqi;
	                }
	              }
	            cqidatavals<< " Time : "<<Simulator::Now().GetMilliSeconds()<<" LTE AMC Module : "<<this<<" RB ID : "<<rbId<<" MCS : "<<(uint16_t)mcs<<" CQI : "<<rbgCqi<<" Spectral Efficiency : "<<SpectralEfficiencyForMcs[mcs]<<", TB Size = "<< (m_amc->GetTbSizeFromMcs (mcs, 48) / 8)<<" Throughput = "<< (m_amc->GetTbSizeFromMcs (mcs, 48) / 8) / 0.001<<"\n";
	            NS_LOG_DEBUG (this << "\t CQI " << rbgCqi);
	            // fill the cqi vector (per RB basis)
	            for (uint8_t j = 0; j < m_dlBandwidth; j++)
	              {
	                cqi.push_back (rbgCqi);
	              }
	            rbgMap.clear ();
	         }

	      }
}

void
LteUePhy::ReportInterference (const SpectrumValue& interf)
{
  // Currently not used by UE
}

void
LteUePhy::ReportRsReceivedPower (const SpectrumValue& power)
{
  NS_LOG_FUNCTION (this << power);
  m_rsReceivedPowerUpdated = true;
  m_rsReceivedPower = power;  
}



Ptr<DlCqiLteControlMessage>
LteUePhy::CreateDlCqiFeedbackMessage (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this);
  
  static std::ofstream rsrpMsg;
  static std::ofstream rsrpReport;
  static std::ofstream cqimcs;
  static int counter = 0;
  if (counter == 0)
  {
	  rsrpMsg.open("//home//rajarajan//Documents//att_workspace//ltea//rsrpVals.txt");
	  cqimcs.open("//home//rajarajan//Documents//att_workspace//ltea//cqimcs.txt");
  }
  counter++;

  // apply transmission mode gain
  //std::cout<<"Just for INFO: m_transmissionMode = "<<(uint16_t)m_transmissionMode<<" and m_txModeGain.size() = "<<m_txModeGain.size()<<" SINR = "<<sinr<<std::endl;
  NS_ASSERT (m_transmissionMode < m_txModeGain.size ());
  SpectrumValue newSinr = sinr;
  newSinr *= m_txModeGain.at (m_transmissionMode);

  m_rsrpSinrSampleCounter++;
  if (m_rsrpSinrSampleCounter==m_rsrpSinrSamplePeriod)
    {
      NS_ASSERT_MSG (m_rsReceivedPowerUpdated, " RS received power info obsolete");
      // RSRP evaluated as averaged received power among RBs
      double sum = 0.0;
      uint8_t rbNum = 0;
      Values::const_iterator it;
      for (it = m_rsReceivedPower.ConstValuesBegin (); it != m_rsReceivedPower.ConstValuesEnd (); it++)
        {
          sum += (*it);
          rbNum++;
        }
      double rsrp = sum / (double)rbNum;
      // averaged SINR among RBs
      for (it = sinr.ConstValuesBegin (); it != sinr.ConstValuesEnd (); it++)
        {
          sum += (*it);
          rbNum++;
        }
      double avSinr = sum / (double)rbNum;
      NS_LOG_INFO (this << " cellId " << m_cellId << " rnti " << m_rnti << " RSRP " << rsrp << " SINR " << avSinr);
 
      std::cout<<"RSRP for cell ID : "<<m_cellId<<" eNB cell ID = "<<m_enbCellId<<" with RNTI "<<m_rnti<<" is "<<rsrp<<" and in dB, it is "<<10*log10(rsrp)<<std::endl;
      rsrpMsg<<" UE PHY : "<<this<<", AMC : "<<m_amc<<" RSRP for cell ID : "<<m_cellId<<" SINR for cell ID : "<<avSinr<<" eNB cell ID = "<<m_enbCellId<<" with RNTI "<<m_rnti<<" is "<<rsrp<<" and in dB, it is "<<10*log10(rsrp)<<"\n";

     // m_reportCurrentCellRsrpSinrTrace (m_cellId, m_rnti, rsrp, avSinr);
      m_reportCurrentCellRsrpSinrTrace (m_enbCellId, m_rnti, rsrp, avSinr);
      m_rsrpSinrSampleCounter = 0;
    }
  // CREATE DlCqiLteControlMessage
  Ptr<DlCqiLteControlMessage> msg = Create<DlCqiLteControlMessage> ();
  CqiListElement_s dlcqi;
  std::vector<int> cqi;
  if (Simulator::Now () > m_p10CqiLast + m_p10CqiPeriocity)
    {
      cqi = m_amc->CreateCqiFeedbacks (newSinr, m_dlBandwidth);

      int nLayer = TransmissionModesLayers::TxMode2LayerNum (m_transmissionMode);

      uint8_t mcs = 0;
    	  uint16_t tbs;
    	  if (cqi.size () > (uint8_t)nLayer)
    	  {
    	      mcs = m_amc->GetMcsFromCqi (cqi.at ((uint8_t)nLayer));
    	  }

    	  else
    	  {
    	                                // no info on this subband -> worst MCS writeControlMsg.txt
    	      mcs = 0;
    	  }

    	  tbs = (m_amc->GetTbSizeFromMcs (mcs, 48) / 8);   // = TB size / TTI


      /*for(unsigned int i = 0 ; i < cqi.size() ; i++)
          	  cqimcs<<this<<" P10 periodicity, PRB "<<(i+1)<<" CQI : "<<cqi.at(i)<<"\n";*/
      cqimcs<<" P10 Cell ID = "<<m_enbCellId<<" = RNTI = "<<m_rnti<<" = MCS = "<<(uint16_t)mcs<<" = TB Size = "<<tbs<<"\n";

      int nbSubChannels = cqi.size ();
      double cqiSum = 0.0;
      int activeSubChannels = 0;
      // average the CQIs of the different RBs
      for (int i = 0; i < nbSubChannels; i++)
        {
          if (cqi.at (i) != -1)
            {
              cqiSum += cqi.at (i);
              activeSubChannels++;
            }
          NS_LOG_DEBUG (this << " subch " << i << " cqi " <<  cqi.at (i));
        }
      rsrpMsg<<this<<" Layers : "<<nLayer<<" CQI Sum : "<<(uint16_t)cqiSum<<" Active Sub Channels : "<<activeSubChannels<<"\n";
      dlcqi.m_rnti = m_rnti;
      dlcqi.m_ri = 1; // not yet used
      dlcqi.m_cqiType = CqiListElement_s::P10; // Peridic CQI using PUCCH wideband
      NS_ASSERT_MSG (nLayer > 0, " nLayer negative");
      NS_ASSERT_MSG (nLayer < 3, " nLayer limit is 2s");
      for (int i = 0; i < nLayer; i++)
        {
          if (activeSubChannels > 0)
            {
              dlcqi.m_wbCqi.push_back ((uint16_t) cqiSum / activeSubChannels);
      //        cqimcs<<" P10 : Layer : " << (i+1)<<" Pushing back : "<<(uint16_t) cqiSum / activeSubChannels<<"\n";
              rsrpMsg<<"Active Sub Channels - greater than 0\n";
            }
          else
            {
              // approximate with the worst case -> CQI = 1
              dlcqi.m_wbCqi.push_back (1);
            }
        }

      /*for(uint32_t i = 0 ; i < dlcqi.m_wbCqi.size(); i++)
         	  cqimcs<<" P10 Pushed : "<<dlcqi.m_wbCqi.at(i)<<"\n";
       */
      //NS_LOG_DEBUG (this << " Generate P10 CQI feedback " << (uint16_t) cqiSum / activeSubChannels);
      dlcqi.m_wbPmi = 0; // not yet used
      // dl.cqi.m_sbMeasResult others CQI report modes: not yet implemented
    }
  else if (Simulator::Now () > m_a30CqiLast + m_a30CqiPeriocity)
    {
      cqi = m_amc->CreateCqiFeedbacks (newSinr, GetRbgSize ());

      int nLayer = TransmissionModesLayers::TxMode2LayerNum (m_transmissionMode);

      uint8_t mcs = 0;
   	  uint16_t tbs;

          	  if (cqi.size () > (uint8_t)nLayer)
          	  {
          	      mcs = m_amc->GetMcsFromCqi (cqi.at ((uint8_t)nLayer));
          	  }

          	  else
          	  {
          	                                // no info on this subband -> worst MCS writeControlMsg.txt
          	      mcs = 0;
          	  }

          	  tbs = (m_amc->GetTbSizeFromMcs (mcs, 48) / 8);   // = TB size / TTI

      cqimcs<<"A30 Cell ID = "<<m_enbCellId<<" = RNTI = "<<m_rnti<<" = MCS = "<<(uint16_t)mcs<<" = TB Size = "<<tbs<<"\n";
      m_reportCurrentCellTbsTrace (m_enbCellId , m_rnti , tbs );
      int nbSubChannels = cqi.size ();
      int rbgSize = GetRbgSize ();



      double cqiSum = 0.0;
      int cqiNum = 0;
      SbMeasResult_s rbgMeas;
      //NS_LOG_DEBUG (this << " Create A30 CQI feedback, RBG " << rbgSize << " cqiNum " << nbSubChannels << " band "  << (uint16_t)m_dlBandwidth);
      for (int i = 0; i < nbSubChannels; i++)
        {
          if (cqi.at (i) != -1)
            {
              cqiSum += cqi.at (i);
            }
          // else "nothing" no CQI is treated as CQI = 0 (worst case scenario)
          cqiNum++;
          if (cqiNum == rbgSize)
            {
              // average the CQIs of the different RBGs
              //NS_LOG_DEBUG (this << " RBG CQI "  << (uint16_t) cqiSum / rbgSize);
              HigherLayerSelected_s hlCqi;
              hlCqi.m_sbPmi = 0; // not yet used
              for (int i = 0; i < nLayer; i++)
                {


                  hlCqi.m_sbCqi.push_back ((uint16_t) cqiSum / rbgSize);
                  //cqimcs<<" A30 CQI Pushing back : "<<(uint16_t)hlCqi.m_sbCqi.at(hlCqi.m_sbCqi.size()-1)<<"\n";

                }
              rbgMeas.m_higherLayerSelected.push_back (hlCqi);

              cqiSum = 0.0;
              cqiNum = 0;
            }
        }
      dlcqi.m_rnti = m_rnti;
      dlcqi.m_ri = 1; // not yet used
      dlcqi.m_cqiType = CqiListElement_s::A30; // Aperidic CQI using PUSCH
      //dlcqi.m_wbCqi.push_back ((uint16_t) cqiSum / nbSubChannels);
      dlcqi.m_wbPmi = 0; // not yet used
      dlcqi.m_sbMeasResult = rbgMeas;



// vector<char



   }
  msg->SetDlCqi (dlcqi);
  return msg;
}



void
LteUePhy::DoSendLteControlMessage (Ptr<LteControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);

  SetControlMessages (msg);
}

//EstimateUlSinr

void
LteUePhy::ReceiveLteControlMessageList (std::list<Ptr<LteControlMessage> > msgList)
{
  NS_LOG_FUNCTION (this);
  static std::ofstream myOutput;
    static int counter = 0;
    if ( counter == 0)
  	  myOutput.open("//home//rajarajan//Documents//att_workspace//ltea//writeUEControlMsg.txt");
    counter++;
    myOutput<<"MSG_LIST (UE PHY): "<<(this)<<" time : "<<Simulator::Now().GetMilliSeconds()<<"\n";
  std::list<Ptr<LteControlMessage> >::iterator it;
  for (it = msgList.begin (); it != msgList.end(); it++)
  {
    Ptr<LteControlMessage> msg = (*it);
  
    if (msg->GetMessageType () == LteControlMessage::DL_DCI)
    {
      Ptr<DlDciLteControlMessage> msg2 = DynamicCast<DlDciLteControlMessage> (msg);
      myOutput<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" DL DCI "<<"\n";
      DlDciListElement_s dci = msg2->GetDci ();
      if (dci.m_rnti != m_rnti)
        {
          // DCI not for me
          continue;
        }
      
      if (dci.m_resAlloc != 0)
      {
        NS_FATAL_ERROR ("Resource Allocation type not implemented");
      }
      
      std::vector <int> dlRb;
      
      // translate the DCI to Spectrum framework
      uint32_t mask = 0x1;
      for (int i = 0; i < 32; i++)
      {
        if (((dci.m_rbBitmap & mask) >> i) == 1)
        {
          for (int k = 0; k < GetRbgSize (); k++)
          {
            dlRb.push_back ((i * GetRbgSize ()) + k);
//             NS_LOG_DEBUG(this << " RNTI " << m_rnti << " RBG " << i << " DL-DCI allocated PRB " << (i*GetRbgSize()) + k);
          }
        }
        mask = (mask << 1);
      }
      
      // send TB info to LteSpectrumPhy
      NS_LOG_DEBUG (this << " UE " << m_rnti << " DL-DCI " << dci.m_rnti << " bitmap "  << dci.m_rbBitmap);
      for (uint8_t i = 0; i < dci.m_tbsSize.size (); i++)
      {
    	  myOutput<<" Time = "<<Simulator::Now().GetMilliSeconds()<<" Index = "<<(i+1)<<" RNTI = "<<dci.m_rnti<<" NDI = "<<dci.m_ndi.at(i)<<" TB Size = "<<dci.m_tbsSize.at(i)<<" MCS = "<<dci.m_mcs.at(i)<<"\n";
    	  m_downlinkSpectrumPhy->AddExpectedTb (dci.m_rnti, dci.m_ndi.at (i), dci.m_tbsSize.at (i), dci.m_mcs.at (i), dlRb, i, dci.m_harqProcess, true /* DL */);
      }
      
      SetSubChannelsForReception (dlRb);
      
      
    }
    else if (msg->GetMessageType () == LteControlMessage::UL_DCI) 
    {
     // myOutput<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" UL DCI "<<"\n";
      //std::cout<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" UL DCI "<<"\n";
      //sleep(3); // Size TB 1 :
      // set the uplink bandwidht according to the UL-CQI
      NS_LOG_DEBUG (this << " UL DCI");
      Ptr<UlDciLteControlMessage> msg2 = DynamicCast<UlDciLteControlMessage> (msg);
      UlDciListElement_s dci = msg2->GetDci ();
      if (dci.m_rnti != m_rnti)
        {
          // DCI not for me
          continue;
        }
      std::vector <int> ulRb;
      for (int i = 0; i < dci.m_rbLen; i++)
      {
        ulRb.push_back (i + dci.m_rbStart);
        //NS_LOG_DEBUG (this << " UE RB " << i + dci.m_rbStart);
      }
      
      QueueSubChannelsForTransmission (ulRb);
      // pass the info to the MAC
      m_uePhySapUser->ReceiveLteControlMessage (msg);
    }
    else
    {
      // pass the message to UE-MAC
      m_uePhySapUser->ReceiveLteControlMessage (msg);
    }
    
  }
  
  
}


void
LteUePhy::QueueSubChannelsForTransmission (std::vector <int> rbMap)
{
  m_subChannelsForTransmissionQueue.at (m_macChTtiDelay - 1) = rbMap;
}


void
LteUePhy::SubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
  NS_LOG_FUNCTION (this << frameNo << subframeNo);

  // refresh internal variables
  m_rsReceivedPowerUpdated = false;
  // update uplink transmission mask according to previous UL-CQIs
  SetSubChannelsForTransmission (m_subChannelsForTransmissionQueue.at (0));
  // shift the queue
  for (uint8_t i = 1; i < m_macChTtiDelay; i++)
    {
      m_subChannelsForTransmissionQueue.at (i-1) = m_subChannelsForTransmissionQueue.at (i);
    }
  m_subChannelsForTransmissionQueue.at (m_macChTtiDelay-1).clear ();
  
  bool srs = false;
  // check SRS periodicity
  //if (m_srsCounter==1)
  //WILD HACK
  //if (m_srsCounter==1)
    if (m_srsCounter==1)
    {
	  //std::cout<<this<<", SRS IS TRUE : COUNTER value is "<<m_srsCounter<<std::endl;
      srs = true;
      m_srsCounter = m_srsPeriodicity;
      //std::cout<<this<<", (NEW VALUES) SRS IS TRUE : COUNTER value is "<<m_srsCounter<<std::endl;
    }
  else
    {
	  //std::cout<<this<<", SRS IS FALSE : COUNTER value is "<<m_srsCounter<<std::endl;
      m_srsCounter--;
      //std::cout<<this<<", (NEW VALUES) SRS IS TRUE : COUNTER value is "<<m_srsCounter<<std::endl;
    }

  if (srs)
    {
      Simulator::Schedule (UL_SRS_DELAY_FROM_SUBFRAME_START, 
                           &LteUePhy::SendSrs,
                           this);
    }


  std::list<Ptr<LteControlMessage> > ctrlMsg = GetControlMessages ();

  static std::ofstream uectrlmsg;
   static int uephycounter = 0;

   if(uephycounter==0)
 	  uectrlmsg.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//lteuephyctrl.txt");

   uephycounter++;
   uectrlmsg<<"UE PHY : "<<this<<" and Spectrum PHY is "<<m_uplinkSpectrumPhy<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Ctrl Msg Queue size : "<<ctrlMsg.size()<<"\n";

   std::list<Ptr<LteControlMessage> >::iterator it = ctrlMsg.begin();


       	  for(unsigned int i = 0 ; i < ctrlMsg.size() ; i++)
       	  {

       		  std::advance(it,i);
       		  std::cout<<"Trying to type cast : "<<(*it)<<std::endl;
       		  uectrlmsg<<"Trying to type cast : "<<(*it)<<"\n";
       		  if((*it)->GetMessageType()==LteControlMessage::DL_CQI)
       		  {
       		  Ptr<DlCqiLteControlMessage> msg = DynamicCast<DlCqiLteControlMessage>(*it);

       		  if(msg)
       		  {

       			  uectrlmsg<<" Message type : "<<msg->GetDlCqi().m_cqiType<<"\n";
       			  if(msg->GetDlCqi().m_cqiType==CqiListElement_s::A30)
       			  {
       				  for(unsigned int i = 0 ; i < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.size() ; i++)
       					  for(unsigned int j = 0 ; j < msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.size() ; j++)
       						  uectrlmsg<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Confirmation CTRL : "<<(uint16_t)msg->GetDlCqi().m_sbMeasResult.m_higherLayerSelected.at(i).m_sbCqi.at(j)<<"\n";
       			  }
       			  else if(msg->GetDlCqi().m_cqiType==CqiListElement_s::P10)
       			  {
       				  for(unsigned int i = 0 ; i < msg->GetDlCqi().m_wbCqi.size() ; i++)
       					  uectrlmsg<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Confirmation CTRL : "<<(uint16_t)msg->GetDlCqi().m_wbCqi.at(i)<<"\n";
       			  }
       		  }
       		  }

       	  }


  // send packets in queue
  NS_LOG_LOGIC (this << " UE - start TX PUSCH + PUCCH");
  // send the current burts of packets
  Ptr<PacketBurst> pb = GetPacketBurst ();
  m_uplinkSpectrumPhy->SetDirection('U');
  if (pb)
    {
      m_uplinkSpectrumPhy->StartTxDataFrame (pb, ctrlMsg, UL_DATA_DURATION);
    }
  else
    {
      // send only PUCCH (ideal: fake null bandwidth signal)
      if (ctrlMsg.size ()>0)
        {
          NS_LOG_LOGIC (this << " UE - start TX PUCCH (NO PUSCH)");
          std::vector <int> dlRb;
          SetSubChannelsForTransmission (dlRb);
          m_uplinkSpectrumPhy->StartTxDataFrame (pb, ctrlMsg, UL_DATA_DURATION);
        }
    }
  
    
  // trigger the MAC
  m_uePhySapUser->SubframeIndication (frameNo, subframeNo);


  ++subframeNo;
  if (subframeNo > 10)
    {
      ++frameNo;
      subframeNo = 1;
    }

  // schedule next subframe indication VTR PRINTS with size
  Simulator::Schedule (Seconds (GetTti ()), &LteUePhy::SubframeIndication, this, frameNo, subframeNo);  
}

void
LteUePhy::SendSrs ()
{
  NS_LOG_FUNCTION (this << " UE " << m_rnti << " start tx SRS, cell Id " << m_cellId);
  // set the current tx power spectral density (full bandwidth)
  std::vector <int> dlRb;
  for (uint8_t i = 0; i < m_ulBandwidth; i++)
    {
      dlRb.push_back (i);
    }
  SetSubChannelsForTransmission (dlRb);
  m_uplinkSpectrumPhy->SetDirection('U');
  m_uplinkSpectrumPhy->StartTxUlSrsFrame ();
}



void
LteUePhy::DoSetRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  m_rnti = rnti;
}

// m_rxSignal
void
LteUePhy::DoSetBandwidth (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  m_ulBandwidth = ulBandwidth;
  m_dlBandwidth = dlBandwidth;

  int Type0AllocationRbg[4] = {
    10,     // RGB size 1
    26,     // RGB size 2
    63,     // RGB size 3
    110     // RGB size 4
  };  // see table 7.1.6.1-1 of 36.213
  for (int i = 0; i < 4; i++)
    {
      if (dlBandwidth < Type0AllocationRbg[i])
        {
          m_rbgSize = i + 1;
          break;
        }
    }
  UpdateNoisePsd ();
}

void 
LteUePhy::DoSetEarfcn (uint16_t dlEarfcn, uint16_t ulEarfcn)
{
  m_dlEarfcn = dlEarfcn;
  m_ulEarfcn = ulEarfcn;
  UpdateNoisePsd ();
}


void
LteUePhy::DoSyncronizeWithEnb (Ptr<LteEnbNetDevice> enbDevice, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enbDevice << cellId);
  m_enbCellId = cellId;
  m_enbDevice = enbDevice;
  
  m_downlinkSpectrumPhy->SetCellId (cellId);
  m_uplinkSpectrumPhy->SetCellId (cellId);
}

void
LteUePhy::DoSetTransmissionMode (uint8_t txMode)
{
  std::cout<<"TX MODE SET = "<<(uint16_t)txMode<<std::endl;
  NS_LOG_FUNCTION (this << (uint16_t)txMode);
  m_transmissionMode = txMode;
  m_downlinkSpectrumPhy->SetTransmissionMode (txMode);
}

void
LteUePhy::DoSetSrsConfigurationIndex (uint16_t srcCi)
{
  NS_LOG_FUNCTION (this << srcCi);
  m_srsPeriodicity = GetSrsPeriodicity (srcCi);
  m_srsCounter = GetSrsSubframeOffset (srcCi) + 1;
  std::cout<< " M_SRSCOUNTER = "<<m_srsCounter<<" UE SRS P " << m_srsPeriodicity << " RNTI " << m_rnti << " offset " << GetSrsSubframeOffset (srcCi) << " cellId " << m_cellId << " CI " << srcCi <<std::endl;
  NS_LOG_DEBUG (this << " UE SRS P " << m_srsPeriodicity << " RNTI " << m_rnti << " offset " << GetSrsSubframeOffset (srcCi) << " cellId " << m_cellId << " CI " << srcCi);
}


void 
LteUePhy::SetTxMode1Gain (double gain)
{
  SetTxModeGain (1, gain);
}

void 
LteUePhy::SetTxMode2Gain (double gain)
{
  SetTxModeGain (2, gain);
}

void 
LteUePhy::SetTxMode3Gain (double gain)
{
  SetTxModeGain (3, gain);
}

void 
LteUePhy::SetTxMode4Gain (double gain)
{
  SetTxModeGain (4, gain);
}

void 
LteUePhy::SetTxMode5Gain (double gain)
{
  SetTxModeGain (5, gain);
}

void 
LteUePhy::SetTxMode6Gain (double gain)
{
  SetTxModeGain (6, gain);
}

void 
LteUePhy::SetTxMode7Gain (double gain)
{
  SetTxModeGain (7, gain);
}


void
LteUePhy::SetTxModeGain (uint8_t txMode, double gain)
{
  NS_LOG_FUNCTION (this << gain);
  // convert to linear
  std::cout<<"Tx Mode here = "<<txMode<<std::endl;
  double gainLin = pow (10.0, (gain / 10.0));
  if (m_txModeGain.size () < txMode)
    {
      m_txModeGain.resize (txMode);
    }
  std::vector <double> temp;
  temp = m_txModeGain;
  m_txModeGain.clear ();
  for (uint8_t i = 0; i < temp.size (); i++)
    {
      if (i==txMode-1)
        {
          m_txModeGain.push_back (gainLin);
        }
      else
        {
          m_txModeGain.push_back (temp.at (i));
        }
    }
  // forward the info to DL LteSpectrumPhy
  m_downlinkSpectrumPhy->SetTxModeGain (txMode, gain);
}

void 
LteUePhy::UpdateNoisePsd ()
{
  Ptr<SpectrumValue> noisePsd = LteSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_dlEarfcn, m_dlBandwidth, m_noiseFigure);
  m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
}


void
LteUePhy::ReceiveLteDlHarqFeedback (DlInfoListElement_s m)
{
  NS_LOG_FUNCTION (this);
  // generate feedback to eNB and send it through ideal PUCCH
  //DL Info List Received Size
  std::cout<<" SETTING DL INFO LIST ELEMENT HARQ FEEDBACK (MES)"<<std::endl;
  	std::ofstream myFile;
  		      myFile.open("//home//rajarajan//Documents//att_workspace//ltea//DlHarqCallBacktesting.txt");
  		      myFile << "SETTING DL INFO LIST ELEMENT HARQ FEEDBACK (MES)\n";
  		      myFile.close();

  Ptr<DlHarqFeedbackLteControlMessage> msg = Create<DlHarqFeedbackLteControlMessage> ();
  msg->SetDlHarqFeedback (m);
  SetControlMessages (msg);
  m_enbDevice->GetPhy()->ReceiveLteDlHarqFeedback(m);

}

void
LteUePhy::SetHarqPhyModule (Ptr<LteHarqPhy> harq)
{
  m_harqPhyModule = harq;
}



} // namespace ns3
