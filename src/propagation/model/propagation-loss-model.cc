/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Contributions: Timo Bingmann <timo.bingmann@student.kit.edu>
 * Contributions: Tom Hewer <tomhewer@mac.com> for Two Ray Ground Model
 *                Pavel Boyko <boyko@iitp.ru> for matrix
 */

#include "propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include <cmath>
#include "ns3/enum.h"
#include <iostream>
#include <fstream>

NS_LOG_COMPONENT_DEFINE ("PropagationLossModel");

namespace ns3 {

std::vector<std::vector<double> > shadowingvtr;
std::vector<std::vector<double> > LogDistancePropagationLossModel::m_shadowing = shadowingvtr;

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (PropagationLossModel);

TypeId 
PropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PropagationLossModel")
    .SetParent<Object> ()
  ;
  return tid;
}

PropagationLossModel::PropagationLossModel ()
  : m_next (0)
{
}

PropagationLossModel::~PropagationLossModel ()
{
}

void
PropagationLossModel::SetNext (Ptr<PropagationLossModel> next)
{
  m_next = next;
}

Ptr<PropagationLossModel>
PropagationLossModel::GetNext ()
{
  return m_next;
}

void
PropagationLossModel::SetDirection (uint8_t direction)
{
	m_direction = direction;
}

void
PropagationLossModel::AddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{
	DoAddShadowing (eNBpositionAllocMacro , nUEs);
}

double
PropagationLossModel::CalcRxPower (double txPowerDbm,
                                   Ptr<MobilityModel> a,
                                   Ptr<MobilityModel> b) const
{
  double self = DoCalcRxPower (txPowerDbm, a, b);
  if (m_next != 0)
    {
      self = m_next->CalcRxPower (self, a, b);
    }
  return self;
}


int64_t
PropagationLossModel::AssignStreams (int64_t stream)
{
  int64_t currentStream = stream;
  currentStream += DoAssignStreams (stream);
  if (m_next != 0)
    {
      currentStream += m_next->AssignStreams (currentStream);
    }
  return (currentStream - stream);
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (RandomPropagationLossModel);

TypeId 
RandomPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RandomPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<RandomPropagationLossModel> ()
    .AddAttribute ("Variable", "The random variable used to pick a loss everytime CalcRxPower is invoked.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&RandomPropagationLossModel::m_variable),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}
RandomPropagationLossModel::RandomPropagationLossModel ()
  : PropagationLossModel ()
{
}

RandomPropagationLossModel::~RandomPropagationLossModel ()
{
}

double
RandomPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                           Ptr<MobilityModel> a,
                                           Ptr<MobilityModel> b) const
{
  double rxc = -m_variable->GetValue ();
  NS_LOG_DEBUG ("attenuation coefficent="<<rxc<<"Db");
  return txPowerDbm + rxc;
}

void
RandomPropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

int64_t
RandomPropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_variable->SetStream (stream);
  return 1;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (FriisPropagationLossModel);

const double FriisPropagationLossModel::PI = 3.14159265358979323846;

TypeId 
FriisPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FriisPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<FriisPropagationLossModel> ()
    .AddAttribute ("Lambda", 
                   "The wavelength  (default is 5.15 GHz at 300 000 km/s).",
                   DoubleValue (300000000.0 / 5.150e9),
                   MakeDoubleAccessor (&FriisPropagationLossModel::m_lambda),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SystemLoss", "The system loss",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&FriisPropagationLossModel::m_systemLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinDistance", 
                   "The distance under which the propagation model refuses to give results (m)",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&FriisPropagationLossModel::SetMinDistance,
                                       &FriisPropagationLossModel::GetMinDistance),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

FriisPropagationLossModel::FriisPropagationLossModel ()
{
}
void
FriisPropagationLossModel::SetSystemLoss (double systemLoss)
{
  m_systemLoss = systemLoss;
}
double
FriisPropagationLossModel::GetSystemLoss (void) const
{
  return m_systemLoss;
}
void
FriisPropagationLossModel::SetMinDistance (double minDistance)
{
  m_minDistance = minDistance;
}
double
FriisPropagationLossModel::GetMinDistance (void) const
{
  return m_minDistance;
}
void
FriisPropagationLossModel::SetLambda (double frequency, double speed)
{
  m_lambda = speed / frequency;
}
void
FriisPropagationLossModel::SetLambda (double lambda)
{
  m_lambda = lambda;
}
double
FriisPropagationLossModel::GetLambda (void) const
{
  return m_lambda;
}

double
FriisPropagationLossModel::DbmToW (double dbm) const
{
  double mw = pow (10.0,dbm/10.0);
  return mw / 1000.0;
}

double
FriisPropagationLossModel::DbmFromW (double w) const
{
  double dbm = log10 (w * 1000.0) * 10.0;
  return dbm;
}

void
FriisPropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

double 
FriisPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
  /*
   * Friis free space equation:
   * where Pt, Gr, Gr and P are in Watt units
   * L is in meter units.
   *
   *    P     Gt * Gr * (lambda^2)
   *   --- = ---------------------
   *    Pt     (4 * pi * d)^2 * L
   *
   * Gt: tx gain (unit-less)
   * Gr: rx gain (unit-less)
   * Pt: tx power (W)
   * d: distance (m)
   * L: system loss
   * lambda: wavelength (m)
   *
   * Here, we ignore tx and rx gain and the input and output values 
   * are in dB or dBm:
   *
   *                           lambda^2
   * rx = tx +  10 log10 (-------------------)
   *                       (4 * pi * d)^2 * L
   *
   * rx: rx power (dB)
   * tx: tx power (dB)
   * d: distance (m)
   * L: system loss (unit-less)
   * lambda: wavelength (m)
   */

  double distance = a->GetDistanceFrom (b);
  if (distance <= m_minDistance)
    {
      return txPowerDbm;
    }
  double numerator = m_lambda * m_lambda;
  double denominator = 16 * PI * PI * distance * distance * m_systemLoss;
  double pr = 10 * log10 (numerator / denominator);
  NS_LOG_DEBUG ("distance="<<distance<<"m, attenuation coefficient="<<pr<<"dB");
  return txPowerDbm + pr;
}

int64_t
FriisPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //
// -- Two-Ray Ground Model ported from NS-2 -- tomhewer@mac.com -- Nov09 //

NS_OBJECT_ENSURE_REGISTERED (TwoRayGroundPropagationLossModel);

const double TwoRayGroundPropagationLossModel::PI = 3.14159265358979323846;

TypeId 
TwoRayGroundPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TwoRayGroundPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<TwoRayGroundPropagationLossModel> ()
    .AddAttribute ("Lambda",
                   "The wavelength  (default is 5.15 GHz at 300 000 km/s).",
                   DoubleValue (300000000.0 / 5.150e9),
                   MakeDoubleAccessor (&TwoRayGroundPropagationLossModel::m_lambda),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SystemLoss", "The system loss",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TwoRayGroundPropagationLossModel::m_systemLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinDistance",
                   "The distance under which the propagation model refuses to give results (m)",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&TwoRayGroundPropagationLossModel::SetMinDistance,
                                       &TwoRayGroundPropagationLossModel::GetMinDistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("HeightAboveZ",
                   "The height of the antenna (m) above the node's Z coordinate",
                   DoubleValue (0),
                   MakeDoubleAccessor (&TwoRayGroundPropagationLossModel::m_heightAboveZ),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

TwoRayGroundPropagationLossModel::TwoRayGroundPropagationLossModel ()
{
}
void
TwoRayGroundPropagationLossModel::SetSystemLoss (double systemLoss)
{
  m_systemLoss = systemLoss;
}
double
TwoRayGroundPropagationLossModel::GetSystemLoss (void) const
{
  return m_systemLoss;
}
void
TwoRayGroundPropagationLossModel::SetMinDistance (double minDistance)
{
  m_minDistance = minDistance;
}
double
TwoRayGroundPropagationLossModel::GetMinDistance (void) const
{
  return m_minDistance;
}
void
TwoRayGroundPropagationLossModel::SetHeightAboveZ (double heightAboveZ)
{
  m_heightAboveZ = heightAboveZ;
}
void 
TwoRayGroundPropagationLossModel::SetLambda (double frequency, double speed)
{
  m_lambda = speed / frequency;
}
void 
TwoRayGroundPropagationLossModel::SetLambda (double lambda)
{
  m_lambda = lambda;
}
double 
TwoRayGroundPropagationLossModel::GetLambda (void) const
{
  return m_lambda;
}

double 
TwoRayGroundPropagationLossModel::DbmToW (double dbm) const
{
  double mw = pow (10.0,dbm / 10.0);
  return mw / 1000.0;
}

double
TwoRayGroundPropagationLossModel::DbmFromW (double w) const
{
  double dbm = log10 (w * 1000.0) * 10.0;
  return dbm;
}

void
TwoRayGroundPropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

double 
TwoRayGroundPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                                 Ptr<MobilityModel> a,
                                                 Ptr<MobilityModel> b) const
{
  /*
   * Two-Ray Ground equation:
   *
   * where Pt, Gt and Gr are in dBm units
   * L, Ht and Hr are in meter units.
   *
   *   Pr      Gt * Gr * (Ht^2 * Hr^2)
   *   -- =  (-------------------------)
   *   Pt            d^4 * L
   *
   * Gt: tx gain (unit-less)
   * Gr: rx gain (unit-less)
   * Pt: tx power (dBm)
   * d: distance (m)
   * L: system loss
   * Ht: Tx antenna height (m)
   * Hr: Rx antenna height (m)
   * lambda: wavelength (m)
   *
   * As with the Friis model we ignore tx and rx gain and output values
   * are in dB or dBm
   *
   *                      (Ht * Ht) * (Hr * Hr)
   * rx = tx + 10 log10 (-----------------------)
   *                      (d * d * d * d) * L
   */
  double distance = a->GetDistanceFrom (b);
  if (distance <= m_minDistance)
    {
      return txPowerDbm;
    }

  // Set the height of the Tx and Rx antennae
  double txAntHeight = a->GetPosition ().z + m_heightAboveZ;
  double rxAntHeight = b->GetPosition ().z + m_heightAboveZ;

  // Calculate a crossover distance, under which we use Friis
  /*
   * 
   * dCross = (4 * pi * Ht * Hr) / lambda
   *
   */

  double dCross = (4 * PI * txAntHeight * rxAntHeight) / GetLambda ();
  double tmp = 0;
  if (distance <= dCross)
    {
      // We use Friis
      double numerator = m_lambda * m_lambda;
      tmp = PI * distance;
      double denominator = 16 * tmp * tmp * m_systemLoss;
      double pr = 10 * log10 (numerator / denominator);
      NS_LOG_DEBUG ("Receiver within crossover (" << dCross << "m) for Two_ray path; using Friis");
      NS_LOG_DEBUG ("distance=" << distance << "m, attenuation coefficient=" << pr << "dB");
      return txPowerDbm + pr;
    }
  else   // Use Two-Ray Pathloss
    {
      tmp = txAntHeight * rxAntHeight;
      double rayNumerator = tmp * tmp;
      tmp = distance * distance;
      double rayDenominator = tmp * tmp * m_systemLoss;
      double rayPr = 10 * log10 (rayNumerator / rayDenominator);
      NS_LOG_DEBUG ("distance=" << distance << "m, attenuation coefficient=" << rayPr << "dB");
      return txPowerDbm + rayPr;

    }
}

int64_t
TwoRayGroundPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (LogDistancePropagationLossModel);

TypeId
LogDistancePropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LogDistancePropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<LogDistancePropagationLossModel> ()
    .AddAttribute ("Exponent",
                   "The exponent of the Path Loss propagation model",
                   DoubleValue (3.52),
                   MakeDoubleAccessor (&LogDistancePropagationLossModel::m_exponent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("FreqExponent",
    				"The exponent of the freq-dependent Path Loss propagation model",
    				DoubleValue (2.16),
    				MakeDoubleAccessor (&LogDistancePropagationLossModel::m_freqexponent),
    				MakeDoubleChecker<double>())
    .AddAttribute ("ReferenceDistance",
                   "The distance at which the reference loss is calculated (m)",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&LogDistancePropagationLossModel::m_referenceDistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RefFreq",
                   "The distance at which the reference loss is calculated (m)",
                   DoubleValue (700),
                   MakeDoubleAccessor (&LogDistancePropagationLossModel::m_refFreq),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceLoss",
                   "The reference loss at reference distance (dB). (Default is Friis at 1m with 5.15 GHz)",
                   DoubleValue (46.6777),
                   MakeDoubleAccessor (&LogDistancePropagationLossModel::m_referenceLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("StandardDeviation",
    			   "The standard deviation for normally-distributed shadowing variable (dB)",
    			   DoubleValue (8.3),
    			   MakeDoubleAccessor (&LogDistancePropagationLossModel::m_shadowingstddev),
    			   MakeDoubleChecker<double> ())
    .AddAttribute ("Mean",
    			   "The mean for normally-distributed shadowing variable (dB)",
    			   DoubleValue (0.0),
    			   MakeDoubleAccessor (&LogDistancePropagationLossModel::m_shadowingmean),
    			   MakeDoubleChecker<double> ())
  ;
  return tid;

}

//BuildingsPropagation
LogDistancePropagationLossModel::LogDistancePropagationLossModel ()
{
//m_shadowing = randomSample(m_shadowingmean,m_shadowingstddev,NORMAL);
	m_randVariable = CreateObject<NormalRandomVariable> ();
	m_shadowingmean = 0.0;
//	m_shadowingstddev = 7.0;
	m_shadowingstddev = 8.3;

}

void
LogDistancePropagationLossModel::SetPathLossExponent (double n)
{
  m_exponent = n;
}

void
LogDistancePropagationLossModel::SetReference (double referenceDistance, double referenceLoss)
{
  m_referenceDistance = referenceDistance;
  m_referenceLoss = referenceLoss;
}
double
LogDistancePropagationLossModel::GetPathLossExponent (void) const
{
  return m_exponent;
}

void
LogDistancePropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{
  Vector3D m_next = eNBpositionAllocMacro->GetNext();
  for(uint16_t ueNo = 1 ; ueNo <= nUEs ; ueNo++)
  {
	  m_shadowing.push_back(std::vector<double>());

	  uint16_t m_cellId=1;
	  while(!eNBpositionAllocMacro->isEnd())
	  {
		  m_shadowing[ueNo-1].push_back(m_randVariable->GetValue(0.0, 8.3));
	//	  m_shadowing[ueNo-1].push_back(0);
	  //ShadowingMapOfeNBs.insert(std::make_pair(m_next, m_cellId));
		/*  if(ueNo==1)
		    std::cout<<"UE : "<<ueNo<<" eNB : "<<m_cellId<<" Shadowing : "<<m_shadowing.at(ueNo-1).at(m_cellId-1)<<std::endl;
	    */
		  m_next = eNBpositionAllocMacro->GetNext ();
		  m_cellId++;
	  }
	  m_shadowing[ueNo-1].push_back(m_randVariable->GetValue(0.0, 8.3));
	//  	  m_shadowing[ueNo-1].push_back(0);
	  /*
	  if(ueNo==1)
		  std::cout<<"UE : "<<ueNo<<" eNB : "<<m_cellId<<" Shadowing : "<<m_shadowing.at(ueNo-1).at(m_cellId-1)<<std::endl;
	   */
	  m_next = eNBpositionAllocMacro->GetNext ();
	  m_cellId++;

	  m_shadowing[ueNo-1].push_back(m_randVariable->GetValue(0.0, 8.3));
//	  m_shadowing[ueNo-1].push_back (0);
	  /*
	  if(ueNo==1)
	  {
	  		  std::cout<<"UE : "<<ueNo<<" eNB : "<<m_cellId<<std::endl;
	  		  std::cout<<" Shadowing : "<<m_shadowing.at(ueNo-1).at(m_cellId-1)<<std::endl;
	  		  sleep(5);
	  }
	  */
	  m_next = eNBpositionAllocMacro->GetNext ();
	  m_cellId++;
  }

}
//-174
// EstimateUlSinr
double
LogDistancePropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                                Ptr<MobilityModel> a,
                                                Ptr<MobilityModel> b) const
{

	static std::ofstream pathlosslogs;

	// cqimcs.txt
	static int counter = 0;

	if(counter==0)
	{
	  pathlosslogs.open("//home//rajarajan//Documents//att_workspace//ltea//hetnet//pathlossvaluenec.txt",std::ios_base::out);
	  counter++;
	}

  //double shadowing;
  double distance = a->GetDistanceFrom (b);
  if (distance <= m_referenceDistance)
    {
      return txPowerDbm;
    }
  //std::ofstream
  //std::cout<<" UE : "<<b->GetNodeId()<<" eNB : "<<a->GetNodeId()<<" Shadowing vector size = "<<m_shadowing.size()<<std::endl;
//  std::cout<<" UE : "<<b->GetNodeId()<<" eNB : "<<a->GetNodeId()<<std::endl;

  double shadowing = 0.0;
  double freq;

   // TOWER_FAILURE - IMPORTANT

   if(m_direction=='D')
   {
	  shadowing = m_shadowing.at(b->GetNodeId()-1).at(a->GetNodeId()-1);
	  freq = a->GetFreq();
   }
   else if(m_direction=='U')
   {
	  shadowing = m_shadowing.at(a->GetNodeId()-1).at(b->GetNodeId()-1);
	  freq = a->GetFreq();
   }


/*  if(b->GetNodeId()==1)
  {
	  std::cout<<" UE : "<<b->GetNodeId()<<" eNB : "<<a->GetNodeId()<<std::endl;
	  std::cout<<" Shad. is "<<shadowing<<std::endl;
	  //sleep(1);
  }
  */
  //uint32_t index = 0;
  //uint8_t sectorCounter = 1;

  /*
  for(uint32_t i = 0 ; i < ShadowingVectorofeNBs.size(); i++)
  {
	if(ShadowingVectorofeNBs.at(i).x == a->GetPosition().x && ShadowingVectorofeNBs.at(i).y == a->GetPosition().y && ShadowingVectorofeNBs.at(i).z == a->GetPosition().z)
	{
		if(sectorCounter == a->GetSector())
		{
		//	shadowing = m_shadowing.at(index);
			break;
		}
		else
		{
			index++;
			sectorCounter++;
		}
	}
	else
		index++;
  }*/
  /**
   * The formula is:
   * rx = 10 * log (Pr0(tx)) - n * 10 * log (d/d0)
   *
   * Pr0: rx power at reference distance d0 (W)
   * d0: reference distance: 1.0 (m)
   * d: distance (m)
   * tx: tx power (dB)
   * rx: dB
   *
   * Which, in our case is:
   *
   * rx = rx0(tx) - 10 * n * log (d/d0)
   */
  //shadowing = 0.0;
  double pathLossDb = 10 * (m_exponent * log10 (distance / m_referenceDistance)) + 10 * (m_freqexponent * log10 (freq / m_refFreq)) + shadowing;
  pathlosslogs<<"UE : "<<b->GetNodeId()<<" eNB : "<<a->GetNodeId()<<" Path Loss DB = "<<pathLossDb<<"\n";


  double rxc = -m_referenceLoss - pathLossDb;
  NS_LOG_DEBUG ("distance="<<distance<<"m, reference-attenuation="<< -m_referenceLoss<<"dB, "<<
                "attenuation coefficient="<<rxc<<"db");
  return txPowerDbm + rxc;
}

int64_t
LogDistancePropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (ThreeLogDistancePropagationLossModel);

TypeId
ThreeLogDistancePropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeLogDistancePropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<ThreeLogDistancePropagationLossModel> ()
    .AddAttribute ("Distance0",
                   "Beginning of the first (near) distance field",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&ThreeLogDistancePropagationLossModel::m_distance0),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Distance1",
                   "Beginning of the second (middle) distance field.",
                   DoubleValue (200.0),
                   MakeDoubleAccessor (&ThreeLogDistancePropagationLossModel::m_distance1),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Distance2",
                   "Beginning of the third (far) distance field.",
                   DoubleValue (500.0),
                   MakeDoubleAccessor (&ThreeLogDistancePropagationLossModel::m_distance2),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Exponent0",
                   "The exponent for the first field.",
                   DoubleValue (1.9),
                   MakeDoubleAccessor (&ThreeLogDistancePropagationLossModel::m_exponent0),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Exponent1",
                   "The exponent for the second field.",
                   DoubleValue (3.8),
                   MakeDoubleAccessor (&ThreeLogDistancePropagationLossModel::m_exponent1),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Exponent2",
                   "The exponent for the third field.",
                   DoubleValue (3.8),
                   MakeDoubleAccessor (&ThreeLogDistancePropagationLossModel::m_exponent2),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceLoss",
                   "The reference loss at distance d0 (dB). (Default is Friis at 1m with 5.15 GHz)",
                   DoubleValue (46.6777),
                   MakeDoubleAccessor (&ThreeLogDistancePropagationLossModel::m_referenceLoss),
                   MakeDoubleChecker<double> ())
  ;
  return tid;

}

ThreeLogDistancePropagationLossModel::ThreeLogDistancePropagationLossModel ()
{
}

void
ThreeLogDistancePropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

double 
ThreeLogDistancePropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                                     Ptr<MobilityModel> a,
                                                     Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  NS_ASSERT (distance >= 0);

  // See doxygen comments for the formula and explanation

  double pathLossDb;

  if (distance < m_distance0)
    {
      pathLossDb = 0;
    }
  else if (distance < m_distance1)
    {
      pathLossDb = m_referenceLoss
        + 10 * m_exponent0 * log10 (distance / m_distance0);
    }
  else if (distance < m_distance2)
    {
      pathLossDb = m_referenceLoss
        + 10 * m_exponent0 * log10 (m_distance1 / m_distance0)
        + 10 * m_exponent1 * log10 (distance / m_distance1);
    }
  else
    {
      pathLossDb = m_referenceLoss
        + 10 * m_exponent0 * log10 (m_distance1 / m_distance0)
        + 10 * m_exponent1 * log10 (m_distance2 / m_distance1)
        + 10 * m_exponent2 * log10 (distance / m_distance2);
    }

  NS_LOG_DEBUG ("ThreeLogDistance distance=" << distance << "m, " <<
                "attenuation=" << pathLossDb << "dB");

  return txPowerDbm - pathLossDb;
}

int64_t
ThreeLogDistancePropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (NakagamiPropagationLossModel);

TypeId
NakagamiPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NakagamiPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<NakagamiPropagationLossModel> ()
    .AddAttribute ("Distance1",
                   "Beginning of the second distance field. Default is 80m.",
                   DoubleValue (80.0),
                   MakeDoubleAccessor (&NakagamiPropagationLossModel::m_distance1),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Distance2",
                   "Beginning of the third distance field. Default is 200m.",
                   DoubleValue (200.0),
                   MakeDoubleAccessor (&NakagamiPropagationLossModel::m_distance2),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("m0",
                   "m0 for distances smaller than Distance1. Default is 1.5.",
                   DoubleValue (1.5),
                   MakeDoubleAccessor (&NakagamiPropagationLossModel::m_m0),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("m1",
                   "m1 for distances smaller than Distance2. Default is 0.75.",
                   DoubleValue (0.75),
                   MakeDoubleAccessor (&NakagamiPropagationLossModel::m_m1),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("m2",
                   "m2 for distances greater than Distance2. Default is 0.75.",
                   DoubleValue (0.75),
                   MakeDoubleAccessor (&NakagamiPropagationLossModel::m_m2),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ErlangRv",
                   "Access to the underlying ErlangRandomVariable",
                   StringValue ("ns3::ErlangRandomVariable"),
                   MakePointerAccessor (&NakagamiPropagationLossModel::m_erlangRandomVariable),
                   MakePointerChecker<ErlangRandomVariable> ())
    .AddAttribute ("GammaRv",
                   "Access to the underlying GammaRandomVariable",
                   StringValue ("ns3::GammaRandomVariable"),
                   MakePointerAccessor (&NakagamiPropagationLossModel::m_gammaRandomVariable),
                   MakePointerChecker<GammaRandomVariable> ());
  ;
  return tid;

}

NakagamiPropagationLossModel::NakagamiPropagationLossModel ()
{
}

double
NakagamiPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                             Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{
  // select m parameter

  double distance = a->GetDistanceFrom (b);
  NS_ASSERT (distance >= 0);

  double m;
  if (distance < m_distance1)
    {
      m = m_m0;
    }
  else if (distance < m_distance2)
    {
      m = m_m1;
    }
  else
    {
      m = m_m2;
    }

  // the current power unit is dBm, but Watt is put into the Nakagami /
  // Rayleigh distribution.
  double powerW = pow (10, (txPowerDbm - 30) / 10);

  double resultPowerW;

  // switch between Erlang- and Gamma distributions: this is only for
  // speed. (Gamma is equal to Erlang for any positive integer m.)
  unsigned int int_m = static_cast<unsigned int>(floor (m));

  if (int_m == m)
    {
      resultPowerW = m_erlangRandomVariable->GetValue (int_m, powerW / m);
    }
  else
    {
      resultPowerW = m_gammaRandomVariable->GetValue (m, powerW / m);
    }

  double resultPowerDbm = 10 * log10 (resultPowerW) + 30;

  NS_LOG_DEBUG ("Nakagami distance=" << distance << "m, " <<
                "power=" << powerW <<"W, " <<
                "resultPower=" << resultPowerW << "W=" << resultPowerDbm << "dBm");

  return resultPowerDbm;
}

void
NakagamiPropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

int64_t
NakagamiPropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_erlangRandomVariable->SetStream (stream);
  m_gammaRandomVariable->SetStream (stream + 1);
  return 2;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (FixedRssLossModel);

TypeId 
FixedRssLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FixedRssLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<FixedRssLossModel> ()
    .AddAttribute ("Rss", "The fixed receiver Rss.",
                   DoubleValue (-150.0),
                   MakeDoubleAccessor (&FixedRssLossModel::m_rss),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}
FixedRssLossModel::FixedRssLossModel ()
  : PropagationLossModel ()
{
}

FixedRssLossModel::~FixedRssLossModel ()
{
}

void
FixedRssLossModel::SetRss (double rss)
{
  m_rss = rss;
}

double
FixedRssLossModel::DoCalcRxPower (double txPowerDbm,
                                  Ptr<MobilityModel> a,
                                  Ptr<MobilityModel> b) const
{
  return m_rss;
}

void
FixedRssLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

int64_t
FixedRssLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (MatrixPropagationLossModel);

TypeId 
MatrixPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MatrixPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<MatrixPropagationLossModel> ()
    .AddAttribute ("DefaultLoss", "The default value for propagation loss, dB.",
                   DoubleValue (std::numeric_limits<double>::max ()),
                   MakeDoubleAccessor (&MatrixPropagationLossModel::m_default),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

MatrixPropagationLossModel::MatrixPropagationLossModel ()
  : PropagationLossModel (), m_default (std::numeric_limits<double>::max ())
{
}

MatrixPropagationLossModel::~MatrixPropagationLossModel ()
{
}

void 
MatrixPropagationLossModel::SetDefaultLoss (double loss)
{
  m_default = loss;
}

void
MatrixPropagationLossModel::SetLoss (Ptr<MobilityModel> ma, Ptr<MobilityModel> mb, double loss, bool symmetric)
{
  NS_ASSERT (ma != 0 && mb != 0);

  MobilityPair p = std::make_pair (ma, mb);
  std::map<MobilityPair, double>::iterator i = m_loss.find (p);

  if (i == m_loss.end ())
    {
      m_loss.insert (std::make_pair (p, loss));
    }
  else
    {
      i->second = loss;
    }

  if (symmetric)
    {
      SetLoss (mb, ma, loss, false);
    }
}

double 
MatrixPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                           Ptr<MobilityModel> a,
                                           Ptr<MobilityModel> b) const
{
  std::map<MobilityPair, double>::const_iterator i = m_loss.find (std::make_pair (a, b));

  if (i != m_loss.end ())
    {
      return txPowerDbm - i->second;
    }
  else
    {
      return txPowerDbm - m_default;
    }
}

int64_t
MatrixPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

void
MatrixPropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (RangePropagationLossModel);

TypeId
RangePropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RangePropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<RangePropagationLossModel> ()
    .AddAttribute ("MaxRange",
                   "Maximum Transmission Range (meters)",
                   DoubleValue (250),
                   MakeDoubleAccessor (&RangePropagationLossModel::m_range),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

RangePropagationLossModel::RangePropagationLossModel ()
{
}

double
RangePropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  if (distance <= m_range)
    {
      return txPowerDbm;
    }
  else
    {
      return -1000;
    }
}

void
RangePropagationLossModel::DoAddShadowing (Ptr<ListPositionAllocator> eNBpositionAllocMacro , uint16_t nUEs)
{

}

int64_t
RangePropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

} // namespace ns3
