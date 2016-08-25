/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Dan Broyles
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
 * Author: Dan Broyles <dbroyl01@ku.edu>
 * Thanks to Kevin Peters, faculty advisor James P.G. Sterbenz, and the ResiliNets 
 * initiative at The University of Kansas, https://wiki.ittc.ku.edu/resilinets
 */
#ifndef GAUSS_MARKOV_MOBILITY_MODEL_H
#define GAUSS_MARKOV_MOBILITY_MODEL_H

#include "constant-velocity-helper.h"
#include "mobility-model.h"
#include "position-allocator.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/box.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

/**
 * \ingroup mobility
 * \brief Gauss-Markov mobility model
 *
 * This is a 3D version of the Gauss-Markov mobility model described in [1]. 
 * Unlike the other mobility models in ns-3, which are memoryless, the Gauss
 * Markov model has both memory and variability. The tunable alpha parameter
 * determines the how much memory and randomness you want to model.
 * Each object starts with a specific velocity, direction (radians), and pitch 
 * angle (radians) equivalent to the mean velocity, direction, and pitch. 
 * At each timestep, a new velocity, direction, and pitch angle are generated 
 * based upon the previous value, the mean value, and a gaussian random variable. 
 * This version is suited for simple airplane flight, where direction, velocity,
 * and pitch are the key variables.
 * The motion field is limited by a 3D bounding box (called "box") which is a 3D
 * version of the "rectangle" field that is used in 2-dimensional ns-3 mobility models.
 * 
 * Here is an example of how to implement the model and set the initial node positions:
 * \code
    MobilityHelper mobility;

    mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
      "Bounds", BoxValue (Box (0, 150000, 0, 150000, 0, 10000)),
      "TimeStep", TimeValue (Seconds (0.5)),
      "Alpha", DoubleValue (0.85),
      "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=800|Max=1200]"),
      "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
      "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
      "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
      "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
      "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));

    mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
      "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
      "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
      "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=10000]"));
 
    mobility.Install (wifiStaNodes);
 * \endcode
 * [1] Tracy Camp, Jeff Boleng, Vanessa Davies, "A Survey of Mobility Models
 * for Ad Hoc Network Research", Wireless Communications and Mobile Computing,
 * Wiley, vol.2 iss.5, September 2002, pp.483-502
 */
class GaussMarkovMobilityModel : public MobilityModel
{
public:
  static TypeId GetTypeId (void);
  GaussMarkovMobilityModel ();
private:
  void Start (void);
  void DoWalk (Time timeLeft);
  virtual void DoDispose (void);
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;
  virtual void DoSetFreq (double cbFreq);
  virtual double DoGetFreq ();
  virtual void DoSetNodeId (uint16_t nodeId);
  virtual uint16_t DoGetNodeId ();
  virtual int64_t DoAssignStreams (int64_t);
  ConstantVelocityHelper m_helper;
  uint16_t m_nodeId;
  double m_cbFreq;
  Time m_timeStep;
  double m_alpha;
  double m_meanVelocity;
  double m_meanDirection;
  double m_meanPitch;
  double m_Velocity;
  double m_Direction;
  double m_Pitch;
  Ptr<RandomVariableStream> m_rndMeanVelocity;
  Ptr<NormalRandomVariable> m_normalVelocity;
  Ptr<RandomVariableStream> m_rndMeanDirection;
  Ptr<NormalRandomVariable> m_normalDirection;
  Ptr<RandomVariableStream> m_rndMeanPitch;
  Ptr<NormalRandomVariable> m_normalPitch;
  EventId m_event;
  Box m_bounds;
};

} // namespace ns3

#endif /* GAUSS_MARKOV_MOBILITY_MODEL_H */
