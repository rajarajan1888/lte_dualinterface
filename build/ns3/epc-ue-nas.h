/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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

#ifndef EPC_UE_NAS_H
#define EPC_UE_NAS_H


#include <ns3/object.h>
#include <ns3/lte-as-sap.h>
#include <ns3/epc-tft-classifier.h>

namespace ns3 {


class EpcHelper;

class EpcUeNas : public Object
{
  friend class MemberLteAsSapUser<EpcUeNas>;
public:

  /** 
   * Constructor
   */
  EpcUeNas ();

  /** 
   * Destructor
   */
  virtual ~EpcUeNas ();

  // inherited from Object
  virtual void DoDispose (void);
  static TypeId GetTypeId (void);

  /**
   * set the EpcHelper with which the NAS will interact as if it were the MME 
   * 
   * \param epcHelper 
   */
  void SetEpcHelper (Ptr<EpcHelper> epcHelper);


  /** 
   * 
   * \param dev the UE NetDevice
   */
  void SetDevice (Ptr<NetDevice> dev);

  /** 
   * 
   * 
   * \param imsi the unique UE identifier
   */
  void SetImsi (uint64_t imsi);

  /** 
   * Set the AS SAP provider to interact with the NAS entity
   * 
   * \param s the AS SAP provider
   */
  void SetAsSapProvider (LteAsSapProvider* s);

  /** 
   * 
   * 
   * \return the AS SAP user exported by this RRC
   */
  LteAsSapUser* GetAsSapUser ();

  /** 
   * set the callback used to forward data packets up the stack
   * 
   * \param cb the callback
   */
  void SetForwardUpCallback (Callback <void, Ptr<Packet> > cb);
 
  /** 
   * instruct the NAS to go to EMM Registered + ECM Connected
   * 
   * 
   * \param enbDevice the eNB through which to connect
   */
  void Connect (Ptr<NetDevice> enbDevice);

  // RAJARAJAN

  void ConnectCA (Ptr<NetDevice> enbDevice);

  // --RAJARAJAN--

  /** 
   * Activate an EPS bearer
   * 
   * \param bearer the characteristics of the bearer to be created
   * \param tft the TFT identifying the traffic that will go on this bearer
   */
  void ActivateEpsBearer (EpsBearer bearer, Ptr<EpcTft> tft);

  // RAJARAJAN

  void ActivateEpsBearerCA (EpsBearer bearer, Ptr<EpcTft> tft);

  // --RAJARAJAN--

  /** 
   * Enqueue an IP packet on the proper bearer for uplink transmission
   * 
   * \param p the packet
   * 
   * \return true if successful, false if an error occurred
   */
  bool Send (Ptr<Packet> p);
 
private:
  
  // LTE AS SAP methods
  void DoNotifyConnectionSuccessful ();
  void DoNotifyConnectionFailed ();
  void DoRecvData (Ptr<Packet> packet);

  Ptr<EpcHelper> m_epcHelper;
  Ptr<NetDevice> m_device;

  uint64_t m_imsi;
  
  LteAsSapProvider* m_asSapProvider;
  LteAsSapUser* m_asSapUser;

  uint8_t m_bidCounter;
  EpcTftClassifier m_tftClassifier;

  Callback <void, Ptr<Packet> > m_forwardUpCallback;
  bool bearerActivated;

};


} // namespace ns3

#endif // EPC_UE_NAS_H
