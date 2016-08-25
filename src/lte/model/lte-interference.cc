/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 * DynamicCast
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */


#include "lte-interference.h"
#include "lte-sinr-chunk-processor.h"

#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/lte-mi-error-model.h>
#include <iostream>
#include <fstream>
#include <ns3/lte-amc.h>
NS_LOG_COMPONENT_DEFINE ("LteInterference");

namespace ns3 {


LteInterference::LteInterference ()
  : m_receiving (false)
{
  NS_LOG_FUNCTION (this);
}

LteInterference::~LteInterference ()
{
  NS_LOG_FUNCTION (this);
}

void 
LteInterference::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_rsPowerChunkProcessorList.clear ();
  m_sinrChunkProcessorList.clear ();
  m_interfChunkProcessorList.clear ();
  m_rxSignal = 0;
  m_allSignals = 0;
  m_noise = 0;
  Object::DoDispose ();
} 


TypeId
LteInterference::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteInterference")
    .SetParent<Object> ()
  ;
  return tid;
}


void
LteInterference::StartRx (Ptr<const SpectrumValue> rxPsd)
{ 
  NS_LOG_FUNCTION (this << *rxPsd);
  static std::ofstream interfRx;
  static int counter = 0;
  if(counter==0)
	  interfRx.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//interfRxcheck.txt");
  counter++;
  if (m_receiving == false)
    {
	  //std::cout<<"FIRST SIGNAL"<<std::endl; Net device :
      NS_LOG_LOGIC ("first signal");
      interfRx<<this<<" First signal "<<Simulator::Now().GetMilliSeconds()<<"\n";
      m_rxSignal = rxPsd->Copy ();
      m_lastChangeTime = Now ();
      m_receiving = true;
      for (std::list<Ptr<LteSinrChunkProcessor> >::const_iterator it = m_sinrChunkProcessorList.begin (); it != m_sinrChunkProcessorList.end (); ++it)
        {
          (*it)->Start (); 
        }
    }
  else
    { //8008245895
	  //std::cout<<"ADDITIONAL SIGNAL = "<<(*m_rxSignal)<<std::endl;
      NS_LOG_LOGIC ("additional signal" << *m_rxSignal);
      interfRx<<this<<" Additional signal "<<Simulator::Now().GetMilliSeconds()<<"\n";

      // receiving multiple simultaneous signals, make sure they are synchronized
      NS_ASSERT (m_lastChangeTime == Now ());
      // make sure they use orthogonal resource blocks
      std::cout<<this<<"Time : "<<Simulator::Now().GetMilliSeconds()<<" RXPSD = "<<(*rxPsd)<<" RX Signal = "<<(*m_rxSignal)<<std::endl;
  //    sleep(2);
  // WILD HACK CHECK
      NS_ASSERT (Sum ((*rxPsd) * (*m_rxSignal)) == 0.0);
      // WILD HACK CHECK
      (*m_rxSignal) += (*rxPsd);
    }
}

// S
//MCS =
void
LteInterference::EndRx ()
{
  NS_LOG_FUNCTION (this);
  ConditionallyEvaluateChunk ();
  m_receiving = false;
  for (std::list<Ptr<LteSinrChunkProcessor> >::const_iterator it = m_rsPowerChunkProcessorList.begin (); it != m_rsPowerChunkProcessorList.end (); ++it)
    {
      (*it)->End ();
    }
  for (std::list<Ptr<LteSinrChunkProcessor> >::const_iterator it = m_sinrChunkProcessorList.begin (); it != m_sinrChunkProcessorList.end (); ++it)
    {
      (*it)->End (); 
    }
  for (std::list<Ptr<LteSinrChunkProcessor> >::const_iterator it = m_interfChunkProcessorList.begin (); it != m_interfChunkProcessorList.end (); ++it)
    {
      (*it)->End ();
    }
}


void
LteInterference::AddSignal (Ptr<const SpectrumValue> spd, const Time duration)
{
	static std::ofstream interfRx;
	static int counter = 0;
	  if (counter == 0)
		  interfRx.open("//home//rajarajan//Documents//att_workspace//ltea//interfRx.txt");
	  counter++;
  NS_LOG_FUNCTION (this << *spd << duration);
  // if this is the first signal that we see, we need to initialize the interference calculation
  if (m_allSignals == 0) 
    {
	  switch(interfType)
	  {
	  case 'd':
		  interfRx<<"DATA\n ";
		  break;

	  case 'c':
	  		  interfRx<<"CTRL\n ";
	  		  break;

	  default:
		     interfRx << "Others\n";
	  }
	 // interfRx<<"Time : "<<Simulator::Now().GetMilliSeconds()<<this<<" No Signal, thus far - Will start now"<<(*m_allSignals)<<" Power spectral density : "<<spd<<"\n";

      m_allSignals = Create<SpectrumValue> (spd->GetSpectrumModel ());
      if(m_receiving && interfType == 'd')
      		  interfRx<<"HEADING TO DATA AT TIME (IF): "<<Simulator::Now().GetMilliSeconds()<<"\n";
    }
  else
  {
	  switch(interfType)
	  	  {
	  	  case 'd':
	  		  interfRx<<"DATA\n ";
	  		AddType('d');
	  		  break;

	  	  case 'c':
	  	  		  interfRx<<"CTRL\n ";
	  	  		AddType('c');
	  	  		  break;

	  	  default:
	  		     interfRx << "Others\n";
	  	  }

	  interfRx<<"Time : "<<Simulator::Now().GetMilliSeconds()<<this<<" Signals present, thus far - Will continue now"<<(*m_allSignals)<<" Power spectral density : "<<*spd<<"\n";
	  if(m_receiving && interfType == 'd')
		  interfRx<<"HEADING TO DATA AT TIME (ELSE) : "<<Simulator::Now().GetMilliSeconds()<<"\n";
  }

  DoAddSignal (spd);

 // interfRx<<"Time : "<<Simulator::Now().GetMilliSeconds()<<this<<" Signals present, AFTER ADDITION"<<(*m_allSignals)<<" Noise : "<<(*m_noise)<<" Power spectral density : "<<*spd<<"\n";

  Simulator::Schedule (duration, &LteInterference::DoSubtractSignal, this, spd);
}


void
LteInterference::DoAddSignal  (Ptr<const SpectrumValue> spd)
{ 
  NS_LOG_FUNCTION (this << *spd);
  ConditionallyEvaluateChunk ();
  (*m_allSignals) += (*spd);
  m_lastChangeTime = Now ();
}

void
LteInterference::DoSubtractSignal  (Ptr<const SpectrumValue> spd)
{
  NS_LOG_FUNCTION (this << *spd);
  ConditionallyEvaluateChunk ();
  //std::cout<<"Are we subtracting signals here (BEFORE SUBTRACTION) ? ?SIGNAL SUBTRACTION"<<(*m_allSignals)<<std::endl;  Blank Indicator is equal to
  (*m_allSignals) -= (*spd);
  //std::cout<<"Are we subtracting signals here ? ?SIGNAL SUBTRACTION"<<(*m_allSignals)<<std::endl; Net device :
  m_lastChangeTime = Now ();
}

void
LteInterference::AddType( char type )
{
	interfType = type;
}

void
LteInterference::ConditionallyEvaluateChunk ()
{
  NS_LOG_FUNCTION (this);
  static std::ofstream mySinr;
  static int counter = 0;
  if(counter==0)
  mySinr.open("sinr.txt");
  counter++;
  if (m_receiving)
    {
	  switch(interfType)
	  {
	  case 'd':
		  mySinr<<"CASE A: DATA ";
		  break;

	  case 'c':
	      mySinr<<"CASE A: CTRL ";
	  	  break;

	  default:
		  mySinr<<"CASE A: Other signal ";

	  }
	  mySinr<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Signal = "<< (*m_rxSignal)<<" All Signals = "<< *m_allSignals<<" Interference without noise = "<<((*m_allSignals)-(*m_rxSignal))<<" Interference = "<<((*m_allSignals)-(*m_rxSignal)+(*m_noise))<<" Noise = "<< *m_noise<<"\n";
	  mySinr<<"End CASE A\n";
      NS_LOG_DEBUG (this << " Receiving");
    }
//data_ctrl.txt

  NS_LOG_DEBUG (this << " now "  << Now () << " last " << m_lastChangeTime);
  if (m_receiving && (Now () > m_lastChangeTime))
    {
      NS_LOG_LOGIC (this << " signal = " << *m_rxSignal << " allSignals = " << *m_allSignals << " noise = " << *m_noise);

      SpectrumValue interf =  (*m_allSignals) - (*m_rxSignal) + (*m_noise);

      SpectrumValue sinr = (*m_rxSignal) / ((*m_allSignals) - (*m_rxSignal) + (*m_noise));

      switch(interfType)
      	  {
      	  case 'd':
      		  mySinr<<"CASE B: DATA ";
      		  break;

      	  case 'c':
      	      mySinr<<"CASE B: CTRL ";
      	  	  break;

      	  default:
   			 mySinr<<"CASE B: Other signal ";

      	  }

      mySinr<<this<<" Time : "<<Simulator::Now().GetMilliSeconds()<<" Signal = "<< *m_rxSignal<<" All Signals = "<< *m_allSignals<<" Noise = "<< *m_noise<<" Interference = "<<interf<<" SINR = "<<sinr<<"\n";
      mySinr<<" End CASE B\n";

      Time duration = Now () - m_lastChangeTime;
      for (std::list<Ptr<LteSinrChunkProcessor> >::const_iterator it = m_sinrChunkProcessorList.begin (); it != m_sinrChunkProcessorList.end (); ++it)
        {
          (*it)->EvaluateSinrChunk (sinr, duration);
        }
      for (std::list<Ptr<LteSinrChunkProcessor> >::const_iterator it = m_rsPowerChunkProcessorList.begin (); it != m_rsPowerChunkProcessorList.end (); ++it)
        {
          (*it)->EvaluateSinrChunk (*m_rxSignal, duration);
        }

      for (std::list<Ptr<LteSinrChunkProcessor> >::const_iterator it = m_interfChunkProcessorList.begin (); it != m_interfChunkProcessorList.end (); ++it)
        {      NS_LOG_LOGIC (this << " signal = " << *m_rxSignal << " allSignals = " << *m_allSignals << " noise = " << *m_noise);

          NS_LOG_DEBUG (this << "ConditionallyEvaluateChunk INTERF ");
          (*it)->EvaluateSinrChunk (interf, duration);
        }
    }
  else
    {
      NS_LOG_DEBUG (this << " NO EV");
    }
}

void
LteInterference::SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd)
{
  NS_LOG_FUNCTION (this << *noisePsd);
  m_noise = noisePsd;
  // we can initialize m_allSignal only now, because earlier we
  // didn't know what spectrum model was going to be used.
  // we'll now create a zeroed SpectrumValue using the same
  // SpectrumModel which is being specified for the noise.
  m_allSignals = Create<SpectrumValue> (noisePsd->GetSpectrumModel ());
}

void
LteInterference::AddRsPowerChunkProcessor (Ptr<LteSinrChunkProcessor> p)
{
  NS_LOG_FUNCTION (this << p);
  m_rsPowerChunkProcessorList.push_back (p);
}

void
LteInterference::AddSinrChunkProcessor (Ptr<LteSinrChunkProcessor> p)
{
  NS_LOG_FUNCTION (this << p);
  m_sinrChunkProcessorList.push_back (p);
}

void
LteInterference::AddInterferenceChunkProcessor (Ptr<LteSinrChunkProcessor> p)
{
  NS_LOG_FUNCTION (this << p);
  m_interfChunkProcessorList.push_back (p);
}




} // namespace ns3


