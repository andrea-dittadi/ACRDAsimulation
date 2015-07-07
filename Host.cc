//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 1992-2008 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//


#include "Host.h"
#include "acrdaPkt.h"

namespace acrda {

Define_Module(Host);


Host::Host()
{
    startFrameEvent = NULL;
    server = NULL;
}


Host::~Host()
{
    std::cout << "Destructor called - host\n";
    std::cout.flush();
    cancelAndDelete(startFrameEvent);
    for (int i=0; i<N_REP; i++) {
        if (startTxEvent[i] != NULL)
            cancelAndDelete(startTxEvent[i]);
        if (endTxEvent[i] != NULL)
            cancelAndDelete(endTxEvent[i]);
    }
    delete [] framePkts;
    delete [] startTxEvent;
    delete [] endTxEvent;
}


void Host::initialize()
{
    thisHostsId = this->idx;

    std::ostringstream strStream;
    strStream << "inputfiles/host" << thisHostsId << ".txt"; // TODO: this should be a parameter (.ini file)
    filename = strStream.str();
    dataFile = std::ifstream(filename);

    if (dataFile.is_open()) {
        haveDataFile = true;
        std::string line;
        getline(dataFile, line);
        radioDelay = std::stod(line, nullptr);
        while (getline(dataFile, line))
            arrivalTimes.push_back(std::stod(line, nullptr));
        dataFile.close();
        arrivalTimes.shrink_to_fit();
        arrTimesIter = arrivalTimes.begin();
        haveExternalArrivalTimes = (arrivalTimes.size() > 0);
    }
    else {
        std::cout << "Unable to open file " << filename << endl;
        haveDataFile = false;
        haveExternalArrivalTimes = false;
    }

    // Display input file status
    std::cout << "   hostId=" << thisHostsId;
    std::cout << "   haveDataFile=" << haveDataFile;
    std::cout << "   haveExternalInterarrivals=" << haveExternalArrivalTimes;
    std::cout << endl;

    stateSignal = registerSignal("state");
    server = simulation.getModuleByPath("server");
    if (!server) error("server not found");

    if (!haveDataFile)
        radioDelay = par("radioDelay");
//    iaTime = &par("iaTime");
    N_REP = par("nRep");
    N_SLOTS = par("nSlots");
    T_FRAME = par("tFrame");
    T_PKT_MAX = T_FRAME / N_SLOTS;
    PKDURATION = 0.9999 * T_PKT_MAX;  // Guard interval to avoid disaster

    replicaCounter = 0;
    pkCounter = 0;

    framePkts = new AcrdaPkt* [N_REP];
    startTxEvent = new cMessage *[N_REP];
    endTxEvent   = new cMessage *[N_REP];

    startFrameEvent = new cMessage("startFrameEvent", MSG_STARTFRAME); // always the same object
    for (int i=0; i<N_REP; i++) {
        startTxEvent[i] = new cMessage("startTxEvent", MSG_STARTTX);
        endTxEvent[i]   = new cMessage("endTxEvent",   MSG_ENDTX);
    }


    state = IDLE;
    emit(stateSignal, state);
    WATCH((int&)state);
    WATCH(pkCounter);

    if (ev.isGUI())
        getDisplayString().setTagArg("t",2,"#808000");

    simtime_t firstArrival = 0;
    if (haveExternalArrivalTimes) {
        firstArrival = *arrTimesIter;
        arrTimesIter++;
    }
    scheduleAt(firstArrival, startFrameEvent);
}


void Host::handleMessage(cMessage *msg)
{
    ASSERT(msg->getKind()==MSG_STARTFRAME || msg->getKind() == MSG_STARTTX || msg->getKind() == MSG_ENDTX);

    if (msg->getKind()==MSG_STARTFRAME)
    {
        ASSERT (state==IDLE);

        simtime_t nextStartFrame = simTime() + T_FRAME;
        if (haveExternalArrivalTimes) {
            nextStartFrame = std::max(nextStartFrame.dbl(), *arrTimesIter);
            arrTimesIter++;
        }
        scheduleAt(nextStartFrame, startFrameEvent);

        // Choose replica locations
        int replicaLocs[N_REP];
        bool decidedReplicas = false;
        while (!decidedReplicas) {
            for (int i=0; i<N_REP; i++)
                replicaLocs[i] = floor((double)rand() / ((unsigned long)RAND_MAX + 1) * N_SLOTS);


            bool allUnique = true;
            for (int i=0; i<N_REP && allUnique; i++)
                for (int j=0; j<N_REP && allUnique; j++)
                    if (i!=j && replicaLocs[i] == replicaLocs[j])
                        allUnique = false;

            if (allUnique)
                decidedReplicas = true;
        }

        // Display replica locations
        for (int i=0; i<N_REP; i++)
            EV << replicaLocs[i] << " ";
        EV << endl;

        // Generate packets and schedule start times
        char pkname[40];
        sprintf(pkname,"pk-%d-#%d", thisHostsId, pkCounter);
        for (int i=0; i<N_REP; i++) {   // for each packet replica in this frame
            double replicaFrameOffset = replicaLocs[i] * T_PKT_MAX;
            scheduleAt(simTime() + replicaFrameOffset, startTxEvent[i]);

            // Take all replica offsets except the current one, center them around the current one
            // TODO: We don't need this
            std::vector<double> replicaRelativeOffsets(N_REP-1);
            for (int j=0; j<N_REP-1; j++) {
                int shiftIdx = (j>=i) ? 1 : 0;
                replicaRelativeOffsets[j] = (replicaLocs[j + shiftIdx] - replicaLocs[i]) * T_PKT_MAX;
            }

            // Create the current packet
            framePkts[i] = new AcrdaPkt(thisHostsId, pkCounter, pkname, replicaRelativeOffsets);
        }

        pkCounter++;        // Increment packet counter
        replicaCounter = 0; // Initialize replica counter for the current packet

    } // start-frame event


    else if (msg->getKind() == MSG_STARTTX)
    {
        ASSERT (state==IDLE);
        ASSERT (replicaCounter >= 0 && replicaCounter < N_REP);
        state = TRANSMIT;
        emit(stateSignal, state);

        // update network graphics
        if (ev.isGUI()) {
            getDisplayString().setTagArg("i",1,"yellow");
            getDisplayString().setTagArg("t",0,"TRANSMIT");
        }


        simtime_t duration = PKDURATION; // TODO handle duration as required!
        sendDirect(framePkts[replicaCounter]->dup(), radioDelay, duration, server->gate("in"));
        delete framePkts[replicaCounter];
        scheduleAt(simTime()+duration, endTxEvent[replicaCounter]);

        replicaCounter++; // number of replicas sent in this frame so far

    }

    else if (msg->getKind() == MSG_ENDTX)  // TODO We don't need this, actually we don't need the state.
    {
        ASSERT (state==TRANSMIT);
        state = IDLE;
        emit(stateSignal, state);

        // update network graphics
        if (ev.isGUI()) {
            getDisplayString().setTagArg("i",1,"");
            getDisplayString().setTagArg("t",0,"");
        }
    }

}

}; //namespace
