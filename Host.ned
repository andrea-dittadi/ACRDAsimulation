//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 1992-2008 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//


//
// A computer in the ALOHAnet network.
//
simple Host
{
    parameters:
        @signal[state](type="long");
        @statistic[radioState](source="state";title="Radio state";enum="IDLE=0,TRANSMIT=1";record=vector);
        //double txRate @unit(bps);         // transmission rate
        double radioDelay @unit(s);       // propagation delay of radio link
        int nRep;
        int nSlots;
        double tPkt @unit(s);
        double defaultAvgSnrLinear;
        string arrivalType;
        double meanInterarr @unit(s);  // packet mean interarrival time
        volatile double randExpUnity @unit(s);
        volatile double randSnrDistrib;
        string randSnrDistribStr;
        int defaultSpreadingFactor;
        @display("i=device/pc_s");
}

