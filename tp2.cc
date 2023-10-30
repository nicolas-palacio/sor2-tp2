/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */
#include <string>
#include <fstream>
#include <stddef.h>
#include <iomanip>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SOR2-Grupo 3-TCP/UDP");
static bool firstCwnd = true;
static bool firstSshThr = true;
static bool firstRtt = true;
static bool firstRto = true;
static Ptr<OutputStreamWrapper> cWndStream;
static Ptr<OutputStreamWrapper> ssThreshStream;
static Ptr<OutputStreamWrapper> rttStream;
static Ptr<OutputStreamWrapper> rtoStream;
static Ptr<OutputStreamWrapper> nextTxStream;
static Ptr<OutputStreamWrapper> nextRxStream;
static Ptr<OutputStreamWrapper> inFlightStream;
static uint32_t cWndValue;
static uint32_t ssThreshValue;


static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue = newval;

  if (!firstSshThr)
    {
      *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << ssThreshValue << std::endl;
    }
}

static void
SsThreshTracer (uint32_t oldval, uint32_t newval)
{
  if (firstSshThr)
    {
      *ssThreshStream->GetStream () << "0.0 " << oldval << std::endl;
      firstSshThr = false;
    }
  *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  ssThreshValue = newval;

  if (!firstCwnd)
    {
      *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << cWndValue << std::endl;
    }
}

static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (Time oldval, Time newval)
{
  if (firstRto)
    {
      *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRto = false;
    }
  *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

/*static void
NextTxTracer (SequenceNumber32 old, SequenceNumber32 nextTx)
{
  NS_UNUSED (old);
  *nextTxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
}*/

static void
InFlightTracer (uint32_t old, uint32_t inFlight)
{
  NS_UNUSED (old);
  *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
}

/*static void
NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
{
  NS_UNUSED (old);
  *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
}*/

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
}

static void
TraceSsThresh (std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  ssThreshStream = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold", MakeCallback (&SsThreshTracer));
}

static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
}

/*static void
TraceNextTx (std::string &next_tx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextTxStream = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence", MakeCallback (&NextTxTracer));
}*/

static void
TraceInFlight (std::string &in_flight_file_name)
{
  AsciiTraceHelper ascii;
  inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight", MakeCallback (&InFlightTracer));
}


/*static void
TraceNextRx (std::string &next_rx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextRxStream = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence", MakeCallback (&NextRxTracer));
}
*/
int main (int argc, char *argv[])
{
  std::string prefix_file_name="gp3-tp2";
  std::string cwndName="cwnd";
  uint16_t port=400;
  bool tracing=true;
  uint64_t data_bytes=400;
  Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpNewReno::GetTypeId()));
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
   

//Creo los dispositivos finales 
  NodeContainer emisores;
  emisores.Create (3); 
  NodeContainer receptores;
  receptores.Create(3);
  NodeContainer routers;
  routers.Create(2);

  PointToPointHelper bottleneck;
  bottleneck.SetDeviceAttribute ("DataRate", StringValue ("50Kbps"));
  bottleneck.SetChannelAttribute ("Delay", StringValue ("100ms"));
  bottleneck.SetQueue("ns3::DropTailQueue","MaxSize", StringValue ("10p"));
//Genero la conexion punto a punto
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  pointToPoint.SetQueue("ns3::DropTailQueue","MaxSize", StringValue ("10p"));
  PointToPointDumbbellHelper dumbbellNetwork(3,pointToPoint,3,pointToPoint,bottleneck);

//Se instala la pila de protocolos internet
  InternetStackHelper stack;
  dumbbellNetwork.InstallStack(stack);


  Ipv4AddressHelper leftIP("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper rightIP("10.2.1.0", "255.255.255.0");
  Ipv4AddressHelper routersIP("10.3.1.0", "255.255.255.0");

  dumbbellNetwork.AssignIpv4Addresses(leftIP,rightIP,routersIP);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  uint32_t nodo0=0;
  uint32_t nodo1=1;
  uint32_t nodo2=2;

//-------------------------------------------
  OnOffHelper clientHelper("ns3::TcpSocketFactory",InetSocketAddress (dumbbellNetwork.GetRightIpv4Address (nodo1),port));
  clientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000.0]"));
  clientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper.SetAttribute("MaxBytes", UintegerValue(data_bytes*10000));
  //AddressValue remoteAddress(InetSocketAddress (dumbbellNetwork.GetRightIpv4Address (1),400));
  //clientHelper.SetAttribute("Remote",remoteAddress);

  clientApps.Add(clientHelper.Install(dumbbellNetwork.GetLeft(nodo0)));

  PacketSinkHelper server("ns3::TcpSocketFactory", InetSocketAddress(dumbbellNetwork.GetRightIpv4Address(nodo1),port));
  serverApps.Add(server.Install(dumbbellNetwork.GetRight(nodo1)));

//-------------------------------------------
  OnOffHelper clientHelper2("ns3::TcpSocketFactory",InetSocketAddress (dumbbellNetwork.GetRightIpv4Address (nodo2),port));
  clientHelper2.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000.0]"));
  clientHelper2.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper2.SetAttribute("MaxBytes", UintegerValue(data_bytes*10000));

  clientApps.Add(clientHelper2.Install(dumbbellNetwork.GetLeft(1)));

  PacketSinkHelper server2("ns3::TcpSocketFactory", InetSocketAddress(dumbbellNetwork.GetRightIpv4Address(nodo2),port));
  serverApps.Add(server2.Install(dumbbellNetwork.GetRight(nodo2)));

//-------------------------------------------
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (90.0));
  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (62.0));

  if(tracing){
    //TO-DO
    pointToPoint.EnablePcapAll ("gp3-tp2-tcp",true);

     std::ofstream ascii;
     Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
                                            std::ios::out);
      stack.EnableAsciiIpv4All (ascii_wrap);

      Simulator::Schedule (Seconds (0.00001), &TraceCwnd, cwndName + "-cwnd.data");
      Simulator::Schedule (Seconds (0.00001), &TraceSsThresh, prefix_file_name + "-ssth.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRtt, prefix_file_name + "-rtt.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRto, prefix_file_name + "-rto.data");
      Simulator::Schedule (Seconds (0.00001), &TraceInFlight, prefix_file_name + "-inflight.data");
  }

  NS_LOG_INFO("Simulation started.");
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
