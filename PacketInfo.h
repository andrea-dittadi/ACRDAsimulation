/*
 * PacketInfo.h
 *
 *  Created on: Jun 25, 2015
 *      Author: andrea
 */


#ifndef PACKETINFO_H_
#define PACKETINFO_H_

#include <acrdaPkt.h>

class PacketInfo {


private:
    int hostIdx;
    int pkIdx;
    double snr; // Linear SNR
    int sf;     // Spreading Factor
    bool resolved;
    simtime_t generationTime; // arrival time at the sender's buffer
    simtime_t startTime; // start of reception
    simtime_t endTime;   // end of reception


public:
    PacketInfo() {};
    PacketInfo(AcrdaPkt *pkt, simtime_t startTime, simtime_t endTime);
    PacketInfo(int hostIdx, int pkIdx, double snr, int sf,
            bool resolved, simtime_t arrivalTime, simtime_t startTime, simtime_t endTime);
    virtual ~PacketInfo();
    bool operator==(const PacketInfo &other) const;
    bool operator!=(const PacketInfo &other) const;

    int getHostIdx() const          { return hostIdx; }
    int getPkIdx() const            { return pkIdx; }
    int getSpreadingFactor() const  { return sf;}
    double getSnr() const           { return snr; }
    simtime_t getGenerationTime() const {return generationTime; }
    simtime_t getStartTime() const  { return startTime; }
    simtime_t getEndTime() const    { return endTime; }
    bool isResolved() const         { return resolved;}
    void setResolved(bool resolved=true);
    bool isReplicaOf(PacketInfo *p);


};

#endif /* PACKETINFO_H_ */
