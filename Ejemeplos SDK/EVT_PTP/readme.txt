You must have PTP time being served to the two cameras.
Can use software PTP server as below or feed by some other means
such as switch/grandmaster.

Test configuration is:
PC w/ Myricom Dual NIC and this connects direct to the two cameras
We use timekeeper from fsmlabs with timekeeper.conf edited to only this
SOURCE0() { PPSDEV=self; }
SERVEPTP1() { PTPSERVERVERSION=2; IFACE="Ethernet 1"; }
SERVEPTP2() { PTPSERVERVERSION=2; IFACE="Ethernet 2"; }

You can get eval version from FSM labs that will work for limited (enough) time.

In SERVEPTP1, the 1 is arbitrary but we match it up with the interface name for fun.
Ethernet 1 and Ethernet 2 are the interface names seen in Network Connections.

With PTP now in the cameras and presumably the IP addresses are set for cam
and host as usual we run the example as follows......


Run in DOS window 1
EVT_PTP 0 BayerRG8
this will spit out the current time in seconds.
ie. 1537640250
Now just run with future trigger time (ie. 50s later - add 50 to current time and run)
EVT_PTP 0 BayerRG8 1537640300
Run in DOS window 2 (same except using other camera)
EVT_PTP 1 BayerRG8 1537640300

Both instances will count down to trigger start and when countdown
reaches 0 then both start streaming and grab 1001 frames in sync.

Three methods used to calculate frame rate at the end:
1. OS timer
2. GevTimestampValue calculation
3. GVSP timestamp

Relevant parameters used in the attached example.
<pFeature>PtpMode</pFeature> 
Off, OneStep, TwoStep
<pFeature>PtpStatus</pFeature> 
Disabled, Listening, Calibrating, Slave
<pFeature>PtpAcquisitionGateTimeHigh</pFeature> 
<pFeature>PtpAcquisitionGateTimeLow</pFeature> 
Time to start acquiring in the future
<pFeature>PtpOffset</pFeature>
Offset in ns.

<pFeature>GevTimestampTickFrequencyHigh</pFeature> 
<pFeature>GevTimestampTickFrequencyLow</pFeature> 
For PTP this is 1000000000 for 1ns period(we hard code in this example for now).
<pFeature>GevTimestampControlLatch</pFeature> 
Latch PTP time
<pFeature>GevTimestampControlReset</pFeature> 
Not valid for PTP
<pFeature>GevTimestampValueHigh</pFeature> 
<pFeature>GevTimestampValueLow</pFeature>
Read latched PTP time.

GVSP leader also carries PTP time and our api and example has 
evtFrameRecv.timestamp parameter to read when frame comes in.