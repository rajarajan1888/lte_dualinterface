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
 */

#ifndef LTE_ADVANCED_HELPER_H
#define LTE_ADVANCED_HELPER_H

#include <ns3/config.h>
#include <ns3/simulator.h>
#include <ns3/names.h>
#include <ns3/net-device.h>
#include <ns3/net-device-container.h>
#include <ns3/node.h>
#include <ns3/node-container.h>
#include <ns3/eps-bearer.h>
#include <ns3/phy-stats-calculator.h>
#include <ns3/mac-stats-calculator.h>
#include <ns3/radio-bearer-stats-calculator.h>
#include <ns3/epc-tft.h>
#include <ns3/mobility-model.h>

namespace ns3 {


class LteUePhy;
class LteEnbPhy;
class SpectrumChannel;
class EpcHelper;
class PropagationLossModel;

/**
 * Creation and configuration of LTE entities
 *
 */
class LteAdvancedHelper : public Object
{
public:
  LteAdvancedHelper (void);
  virtual ~LteAdvancedHelper (void);

  static TypeId GetTypeId (void);
  virtual void DoDispose (void);

  /**
     * Set the EpcHelper to be used to setup the EPC network in
     * conjunction with the setup of the LTE radio access network
     *
     * \note if no EpcHelper is ever set, then LteHelper will default
     * to creating an LTE-only simulation with no EPC, using LteRlcSm as
     * the RLC model, and without supporting any IP networking. In other
     * words, it will be a radio-level simulation involving only LTE PHY
     * and MAC and the FF Scheduler, with a saturation traffic model for
     * the RLC.
     *
     * \param h a pointer to the EpcHelper to be used
     */
    void SetEpcHelper (Ptr<EpcHelper> h);

    /**
     *
     *
     * \param type the type of pathloss model to be used for the eNBs
     */
    void SetPathlossModelType (std::string type);

    /**
     * set an attribute for the pathloss model to be created
     *
     * \param n the name of the attribute
     * \param v the value of the attribute
     */
    void SetPathlossModelAttribute (std::string n, const AttributeValue &v);

    /**
     *
     * \param type the type of scheduler to be used for the eNBs
     */
    void SetSchedulerType (std::string type);

    /**
     * set an attribute for the scheduler to be created
     *
     * \param n the name of the attribute
     * \param v the value of the attribute
     */
    void SetSchedulerAttribute (std::string n, const AttributeValue &v);

    /**
     * set an attribute for the LteEnbNetDevice to be created
     *
     * \param n the name of the attribute
     * \param v the value of the attribute
     */
    void SetEnbDeviceAttribute (std::string n, const AttributeValue &v);

    /**
     *
     * \param type the type of AntennaModel to be used for the eNBs
     */
    void SetEnbAntennaModelType (std::string type);

    /**
     * set an attribute for the AntennaModel to be used for the eNBs
     *
     * \param n the name of the attribute
     * \param v the value of the attribute
     */
    void SetEnbAntennaModelAttribute (std::string n, const AttributeValue &v);

    /**
     *
     * \param type the type of AntennaModel to be used for the UEs
     */
    void SetUeAntennaModelType (std::string type);

    /**
     * set an attribute for the AntennaModel to be used for the UEs
     *
     * \param n the name of the attribute
     * \param v the value of the attribute
     */
    void SetUeAntennaModelAttribute (std::string n, const AttributeValue &v);

    /**
     *
     * \param type the type of SpectrumChannel to be used for the UEs
     */
    void SetSpectrumChannelType (std::string type);

    /**
     * set an attribute for the SpectrumChannel to be used for the UEs
     *
     * \param n the name of the attribute
     * \param v the value of the attribute
     */
    void SetSpectrumChannelAttribute (std::string n, const AttributeValue &v);
    /**
     * create a set of eNB devices
     *
     * \param c the node container where the devices are to be installed
     *
     * \return the NetDeviceContainer with the newly created devices
     */

  // RAJARAJAN

  /**
     * create a vector of network interfaces per eNB for inter-band CA
     *
     * \param c the node container where the devices are to be installed
     *
     * \return the NetDeviceContainer with the newly created devices
     */
  std::vector<NetDeviceContainer> InstallEnbDeviceCA (NodeContainer c);

  /**
     * create a vector of network interfaces per UE for inter-band CA
     *
     * \param c the node container where the devices are to be installed
     *
     * \return the NetDeviceContainer with the newly created devices
     */

  std::vector<NetDeviceContainer> InstallUeDeviceCA (NodeContainer c);

  /**
   * Attach the UE devices of all the UEs to all the network interfaces of a single eNB device
   *
   * \param ueDevices
   * \param enbDevice
   */
  void AttachCA (std::vector<NetDeviceContainer> ueDevices, NetDeviceContainer enbDevices);

  // -- RAJARAJAN --
  /**
     * Attach a UE to the network
     *
     * Attach a UE device to the network via a given eNB, and activate the default EPS bearer.
     *
     * \param ueDevice
     * \param enbDevice
     */
  void AttachCA (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice, unsigned int index);

  /**
     *
     *
     * \param type the fading model to be used
     */
  void SetFadingModel (std::string model);

    /**
     * set an attribute of the fading model
     */
  void SetFadingModelAttribute (std::string n, const AttributeValue &v);

  /**
   * Activate EPS for the devices connected to all the CCs of the eNB
   */

  void ActivateDedicatedEpsBearerCA (std::vector<NetDeviceContainer> ueDevices, EpsBearer bearer, Ptr<EpcTft> tft);

  void ActivateDedicatedEpsBearerCA (Ptr<NetDevice> ueDevice, EpsBearer bearer, Ptr<EpcTft> tft);

  void ActivateDataRadioBearerCA (std::vector<NetDeviceContainer> ueDevices, EpsBearer bearer);

  void ActivateDataRadioBearerCA (NetDeviceContainer ueDeviceSet, EpsBearer bearer);

  void AddX2Interface (NodeContainer enbNodes);

  void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);

  /**
    * Enables logging for all components of the LENA architecture
    *
    */
   void EnableLogComponents (void);

   /**
    * Enables trace sinks for PHY, MAC, RLC and PDCP. To make sure all nodes are
    * traced, traces should be enabled once all UEs and eNodeBs are in place and
    * connected, just before starting the simulation.
    */
   void EnableTracesAdvanced (void);

   /**
    * Enable trace sinks for PHY layer
    */
   void EnablePhyTracesAdvanced (void);

   /**
    * Enable trace sinks for DL PHY layer
    */
   void EnableDlPhyTracesAdvanced (void);

   /**
    * Enable trace sinks for UL PHY layer
    */
   void EnableUlPhyTracesAdvanced (void);

   /**
    * Enable trace sinks for MAC layer
    */
   void EnableMacTracesAdvanced (void);

   /**
    * Enable trace sinks for DL MAC layer
    */
   void EnableDlMacTracesAdvanced (void);

   /**
    * Enable trace sinks for UL MAC layer
    */
   void EnableUlMacTracesAdvanced (void);

   /**
    * Enable trace sinks for RLC layer
    */
   void EnableRlcTracesAdvanced (void);

   /**
    * Enable trace sinks for DL RLC layer
    */
   void EnableDlRlcTracesAdvanced (void);

   /**
    * Enable trace sinks for UL MAC layer
    */
   void EnableUlRlcTracesAdvanced (void);

   /**
    *
    * \return the RLC stats calculator object
    */
   Ptr<RadioBearerStatsCalculator> GetRlcStatsAdvanced (void);

   /**
    * Enable trace sinks for PDCP layer
    */
   void EnablePdcpTracesAdvanced (void);

   /**
    * Enable trace sinks for DL PDCP layer
    */
   void EnableDlPdcpTracesAdvanced (void);

   /**
    * Enable trace sinks for UL MAC layer
    */
   void EnableUlPdcpTracesAdvanced (void);

   /**
    *
    * \return the PDCP stats calculator object
    */
   Ptr<RadioBearerStatsCalculator> GetPdcpStatsAdvanced (void);

   enum LteEpsBearerToRlcMapping_t {RLC_SM_ALWAYS = 1,
                                    RLC_UM_ALWAYS = 2,
                                    RLC_AM_ALWAYS = 3,
                                    PER_BASED = 4};

   /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams (possibly zero) that
   * have been assigned. The Install() method should have previously been
   * called by the user.
   *
   * \param c NetDeviceContainer of the set of net devices for which the
   *          LteNetDevice should be modified to use a fixed stream
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this helper
   */

   int64_t AssignStreams (std::vector<NetDeviceContainer> netDevices, int64_t stream);

protected:
  // inherited from Object
  virtual void DoStart (void);

private:
  std::vector<Ptr<NetDevice> > InstallMultipleEnbDevice (Ptr<Node> n);
  std::vector<Ptr<NetDevice> > InstallMultipleUeDevice (Ptr<Node> n);

  std::vector<Ptr<SpectrumChannel> > m_downlinkChannelCA;
  std::vector<Ptr<SpectrumChannel> > m_uplinkChannelCA;

  Ptr<Object> m_downlinkPathlossModel;
  Ptr<Object> m_uplinkPathlossModel;

  ObjectFactory m_schedulerFactory;
  ObjectFactory m_propagationModelFactory;
  ObjectFactory m_enbNetDeviceFactory;
  ObjectFactory m_enbAntennaModelFactory;
  ObjectFactory m_ueAntennaModelFactory;
  ObjectFactory m_dlPathlossModelFactory;
  ObjectFactory m_ulPathlossModelFactory;
  ObjectFactory m_channelFactory;

  std::string m_fadingModelType;
  ObjectFactory m_fadingModelFactory;

  Ptr<PhyStatsCalculator> m_phyStats;
  Ptr<MacStatsCalculator> m_macStats;
  Ptr<RadioBearerStatsCalculator> m_rlcStats;
  Ptr<RadioBearerStatsCalculator> m_pdcpStats;

//  const int maxCCs = 5;
  //std::vector<LteHelper> m_lteHelperCA;
  Ptr<EpcHelper> m_epcHelperCA;

};


} // namespace ns3



#endif // LTE_ADVANCED_HELPER_H
