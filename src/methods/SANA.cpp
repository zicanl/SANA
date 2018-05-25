#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <queue>
#include <iomanip>
#include <set>
#include <cmath>
#include <limits>
#include <cassert>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "SANA.hpp"
#include "../measures/SymmetricSubstructureScore.hpp"
#include "../measures/InducedConservedStructure.hpp"
#include "../measures/EdgeCorrectness.hpp"
#include "../measures/SquaredEdgeScore.hpp"
#include "../measures/WeightedEdgeConservation.hpp"
#include "../measures/TriangleCorrectness.hpp"
#include "../measures/NodeCorrectness.hpp"
#include "../measures/SymmetricEdgeCoverage.hpp"
#include "../measures/localMeasures/Sequence.hpp"
#include "../utils/NormalDistribution.hpp"
#include "../utils/LinearRegression.hpp"
#include "../utils/utils.hpp"

#define FinalPBad 1e-10

using namespace std;

void SANA::initTau(void) {
    /*
    tau = vector<double> {
    1.000, 0.985, 0.970, 0.960, 0.950, 0.942, 0.939, 0.934, 0.928, 0.920,
    0.918, 0.911, 0.906, 0.901, 0.896, 0.891, 0.885, 0.879, 0.873, 0.867,
    0.860, 0.853, 0.846, 0.838, 0.830, 0.822, 0.810, 0.804, 0.794, 0.784,
    0.774, 0.763, 0.752, 0.741, 0.728, 0.716, 0.703, 0.690, 0.676, 0.662,
    0.647, 0.632, 0.616, 0.600, 0.584, 0.567, 0.549, 0.531, 0.514, 0.495,
    0.477, 0.458, 0.438, 0.412, 0.400, 0.381, 0.361, 0.342, 0.322, 0.303,
    0.284, 0.264, 0.246, 0.228, 0.210, 0.193, 0.177, 0.161, 0.145, 0.131,
    0.117, 0.104, 0.092, 0.081, 0.070, 0.061, 0.052, 0.044, 0.0375, 0.031,
    0.026, 0.0212, 0.0172, 0.0138, 0.011, 0.008, 0.006, 0.005, 0.004, 0.003,
    0.002, 0.001, 0.0003, 0.0001, 3e-5, 1e-6, 0, 0, 0, 0,
    0};
     */
    tau = vector<double> {
        0.996738, 0.994914, 0.993865, 0.974899, 0.977274, 0.980926, 0.97399, 0.970583, 0.967492, 0.962373,
        0.953197, 0.954104, 0.951387, 0.953532, 0.948492, 0.939501, 0.939128, 0.932902, 0.912378, 0.896011,
        0.89535, 0.88642, 0.874628, 0.856721, 0.855782, 0.838483, 0.820407, 0.784303, 0.771297, 0.751457,
        0.735902, 0.676393, 0.633939, 0.604872, 0.53482, 0.456856, 0.446905, 0.377708, 0.337258, 3.04e-01,
        0.280585, 0.240093, 1.95e-01, 1.57e-01, 1.21e-01, 1.00e-01, 8.04e-02, 5.95e-02, 4.45e-02, 3.21e-02,
        1.81e-02, 1.82e-02, 1.12e-02, 7.95e-03, 4.82e-03, 3.73e-03, 2.11e-03, 1.41e-03, 9.69e-04, 6.96e-04,
        5.48e-04, 4.20e-04, 4.00e-04, 3.50e-04, 3.10e-04, 2.84e-04, 2.64e-04, 1.19e-04, 8.16e-05, 7.22e-05,
        6.16e-05, 4.46e-05, 3.36e-05, 2.66e-05, 1.01e-05, 9.11e-06, 4.09e-06, 3.96e-06, 3.43e-06, 3.12e-06,
        2.46e-06, 2.02e-06, 1.85e-06, 1.72e-06, 1.10e-06, 9.13e-07, 8.65e-07, 8.21e-07, 7.26e-07, 6.25e-07,
        5.99e-07, 5.42e-07, 8.12e-08, 4.16e-08, 6.56e-09, 9.124e-10, 6.1245e-10, 3.356e-10, 8.124e-11, 4.587e-11};
}

SANA::SANA(Graph* G1, Graph* G2,
        double TInitial, double TDecay, double t, bool usingIterations, bool addHillClimbing, MeasureCombination* MC, string& objectiveScore
#ifdef WEIGHTED
        ,string& startAligName
#endif
        ): Method(G1, G2, "SANA_"+MC->toString())
{
    //data structures for the networks
    n1              = G1->getNumNodes();
    n2              = G2->getNumNodes();
    g1Edges         = G1->getNumEdges();
#ifdef WEIGHTED
    g1WeightedEdges = G1->getWeightedNumEdges();
    g2WeightedEdges = G2->getWeightedNumEdges();
#endif
    g2Edges         = G2->getNumEdges();
    if (objectiveScore == "sum")
        score = Score::sum;
    else if (objectiveScore == "product")
        score = Score::product;
    else if (objectiveScore == "inverse")
        score = Score::inverse;
    else if (objectiveScore == "max")
        score = Score::max;
    else if (objectiveScore == "min")
        score = Score::min;
    else if (objectiveScore == "maxFactor")
        score = Score::maxFactor;
    else if (objectiveScore == "pareto")
        score = Score::pareto;

    paretoInitial   = MC->getParetoInitial();
    paretoCapacity  = MC->getParetoCapacity();

    G1->getAdjMatrix(G1AdjMatrix);
    G2->getAdjMatrix(G2AdjMatrix);
    G1->getAdjLists(G1AdjLists);
#ifdef WEIGHTED
    if (startAligName != "") {
        prune(startAligName);
        this->startAligName = startAligName;
    }
#endif
    G2->getAdjLists(G2AdjLists);

    //random number generation
    random_device rd;
    gen                       = mt19937(getRandomSeed());
    G1RandomNode              = uniform_int_distribution<>(0, n1-1);
    uint G1UnLockedCount      = n1 - G1->getLockedCount() -1;
    G1RandomUnlockedNodeDist  = uniform_int_distribution<>(0, G1UnLockedCount);
    G2RandomUnassignedNode    = uniform_int_distribution<>(0, n2-n1-1);
    G1RandomUnlockedGeneDist  = uniform_int_distribution<>(0, G1->unlockedGeneCount - 1);
    G1RandomUnlockedmiRNADist = uniform_int_distribution<>(0, G1->unlockedmiRNACount - 1);
    randomReal                = uniform_real_distribution<>(0, 1);

    G2RandomUnassignedGeneDist = uniform_int_distribution<>(0, G2->geneCount - G1->geneCount -1);
    G2RandomUnassignedmiRNADist = uniform_int_distribution<>(0, G2->miRNACount - G1->miRNACount -1);

    //temperature schedule
    this->TInitial        = TInitial;
    this->TDecay          = TDecay;
    this->usingIterations = usingIterations;

    if (this->usingIterations) {
        maxIterations = (uint)(t);
    } else {
        minutes = t;
    }
    initializedIterPerSecond = false;

    //data structures for the solution space search
    assert(G1->hasNodeTypes() == G2->hasNodeTypes());
    uint ramificationChange[2], ramificationSwap[2], totalRamification[2];
    if(G1->hasNodeTypes()) {
	assert(G1->geneCount <= G2->geneCount);
	assert(G1->miRNACount <= G2->miRNACount);
	ramificationChange[0] = G1->geneCount*(G2->geneCount - G1->geneCount);
	ramificationChange[1] = G1->miRNACount*(G2->miRNACount - G1->miRNACount);
	ramificationSwap[0]   = G1->geneCount*(G1->geneCount - 1)/2;
	ramificationSwap[1]   = G1->miRNACount*(G1->miRNACount - 1)/2;
	totalRamification[0]  = ramificationSwap[0] + ramificationChange[0];
	totalRamification[1]  = ramificationSwap[1] + ramificationChange[1];
	changeProbability[0]       = (double) ramificationChange[0]/totalRamification[0];
	changeProbability[1]       = (double) ramificationChange[1]/totalRamification[1];
    }
    else
    {
	ramificationChange[0] = n1*(n2-n1);
	ramificationSwap[0]   = n1*(n1-1)/2;
	totalRamification[0]  = ramificationSwap[0] + ramificationChange[0];
	changeProbability[0] = (double)ramificationChange[0]/totalRamification[0];
    }

    initTau();

    //objective function
    this->MC  = MC;
    ecWeight  = MC->getWeight("ec");
    s3Weight  = MC->getWeight("s3");
    icsWeight = MC->getWeight("ics");
    secWeight = MC->getWeight("sec");
    mecWeight = MC->getWeight("mec");
    sesWeight = MC->getWeight("ses");

    try {
        wecWeight = MC->getWeight("wec");
    } catch(...) {
        wecWeight = 0;
    }

    try {
        ewecWeight = MC->getWeight("ewec");
    } catch(...) {
        ewecWeight = 0;
    }

    try {
        needNC      = false;
        ncWeight    = MC->getWeight("nc");
        Measure* nc = MC->getMeasure("nc");
        trueA       = nc->getMappingforNC();
        needNC      = true;
    } catch(...) {
        ncWeight = 0;
        trueA    = {static_cast<ushort>(G2->getNumNodes()), 1};
    }

    try {
        TCWeight = MC->getWeight("tc");
    } catch (...) {
        TCWeight = 0;
    }

    localWeight = MC->getSumLocalWeight();


    restart              = false; //restart scheme
    dynamic_tdecay       = false; //temperature decay dynamically
    needAligEdges        = icsWeight > 0 || ecWeight > 0 || s3Weight > 0 || wecWeight > 0 || secWeight > 0 || mecWeight > 0; //to evaluate EC incrementally
    needSquaredAligEdges = sesWeight > 0; // to evaluate SES incrementally
    needInducedEdges     = s3Weight > 0 || icsWeight > 0; //to evaluate S3 & ICS incrementally
    needWec              = wecWeight > 0; //to evaluate WEC incrementally
    needEwec             = ewecWeight>0; //to evaluate EWEC incrementally
    needSec              = secWeight > 0; //to evaluate SEC incrementally
    needTC               = TCWeight > 0; //to evaluation TC incrementally
    needLocal            = localWeight > 0;

    if (needWec) {
        Measure* wec                     = MC->getMeasure("wec");
        LocalMeasure* m                  = ((WeightedEdgeConservation*) wec)->getNodeSimMeasure();
        vector<vector<float> >* wecSimsP = m->getSimMatrix();
        wecSims                          = (*wecSimsP);
    }
#ifdef CORES
    coreFreq        = vector<vector<ulong> > (n1, vector<ulong>(n2, 0));
    coreCount       = vector<ulong>(n1, 0);
    weightedCoreFreq= vector<vector<double> > (n1, vector<double>(n2, 0));
    totalCoreWeight = vector<double>(n1, 0);
#endif
    //to evaluate local measures incrementally
    if (needLocal) {
        sims              = MC->getAggregatedLocalSims();
        localSimMatrixMap = MC->getLocalSimMap();
        localWeight       = 1; //the values in the sim matrix 'sims' have already been scaled by the weight
    } else {
        localWeight = 0;
    }


    //other execution options
    constantTemp        = false;
    enableTrackProgress = true;
    iterationsPerStep   = 10000000;

    //Make sana think were running in (score == sum) to avoid crashing during linear regression.
    if(score == Score::pareto)
        score = Score::sum;

    /*
    this does not need to be initialized here,
    but the space has to be reserved in the
    stack. it is initialized properly before
    running the algorithm
    */
    assignedNodesG2       = new vector<bool> (n2);
    unassignedNodesG2     = new vector<ushort> (n2-n1);
    A                     = new vector<ushort> (n1);
    avgEnergyInc          = -0.00001; //to track progress
    this->addHillClimbing = addHillClimbing;
}

Alignment SANA::getStartingAlignment(){
    Alignment randomAlig;

    if(this->startAligName != "")
        randomAlig = Alignment::loadEdgeList(G1, G2, startAligName);
    else if (G1->hasNodeTypes()) 
        randomAlig = Alignment::randomAlignmentWithNodeType(G1,G2);
    else if (lockFileName != "")
        randomAlig = Alignment::randomAlignmentWithLocking(G1,G2);
    else
        randomAlig = Alignment::random(n1, n2);
    
    // Doing a Rexindexing if required
#ifdef REINDEX    
    if (G1->hasNodeTypes()) {    
    	randomAlig.reIndexBefore_Iterations(G1->getNodeTypes_ReIndexMap());
    } 
    else if (lockFileName != "") {
	    randomAlig.reIndexBefore_Iterations(G1->getLocking_ReIndexMap());
    }
#endif

    return randomAlig;
}

Alignment SANA::run() {
    if (restart)
        return runRestartPhases();
    else {
        long long int iter = 0;
        Alignment align;
        if(!usingIterations) {
          cout << "usingIterations = 0" << endl;
          align = simpleRun(getStartingAlignment(), minutes * 60, (long long int) (getIterPerSecond()*minutes*60), iter);
        }
        else {
          cout << "usingIterations = 1" << endl;
          align = simpleRun(getStartingAlignment(), ((long long int)(maxIterations))*10000000, iter);
        }

        if(addHillClimbing){
            Timer hill;
            hill.start();
            cout << "Adding HillClimbing at the end.. ";
            align = hillClimbingAlignment(align, (long long int)(10000000)); //arbitrarily chosen, probably too big.
            cout << hill.elapsedString() << endl;
        }
#define PRINT_CORES 0
#if PRINT_CORES
#ifndef CORES
#error must have CORES macro defined to print them
#endif
	unordered_map<ushort,string> G1Index2Name = G1->getIndexToNodeNameMap();
	unordered_map<ushort,string> G2Index2Name = G2->getIndexToNodeNameMap();
	printf("######## core frequencies#########\n");
	for(ushort i=0; i<n1; i++) for(ushort j=0; j<n2; j++) if(coreFreq[i][j]>0)
	    printf("%s %s  %lu / %lu = %.16f weighted %.16f / %.16f = %.16f\n", G1Index2Name[i].c_str(), G2Index2Name[j].c_str(),
		coreFreq[i][j], coreCount[i],
		coreFreq[i][j]/(double)coreCount[i],
		weightedCoreFreq[i][j], totalCoreWeight[i],
		weightedCoreFreq[i][j]/totalCoreWeight[i]
		);
#endif
        return align;
    }
}

unordered_set<vector<ushort>*>* SANA::paretoRun() {
    long long int iter = 0;
    if (!usingIterations) {
        return simpleParetoRun(getStartingAlignment(), (long long int) (getIterPerSecond()*minutes*60), iter);
    } else {
        return simpleParetoRun(getStartingAlignment(), ((long long int)(maxIterations))*100000000, iter);
    }
    return storedAlignments;
}

// Used for method #2 of locking
inline ushort SANA::G1RandomUnlockedNode_Fast() {
    return unLockedNodesG1[G1RandomUnlockedNodeDist(gen)];
}

inline ushort SANA::G1RandomUnlockedNode(){
    #ifdef REINDEX
        return G1RandomUnlockedNodeDist(gen);
    #else
        return G1RandomUnlockedNode_Fast();
    #endif
}

// Gives a random unlocked nodes with the same type as source1
// Only called from performswap
inline ushort SANA::G1RandomUnlockedNode(uint source1){
    if(!nodesHaveType){
        return SANA::G1RandomUnlockedNode();
    }
    else{
        bool reIndex = false;
        #ifdef REINDEX
            reIndex  = true;
        #endif

        // Checking node type and returning one with same type
        if(reIndex){
            bool isGene = source1 < (uint) G1->unlockedGeneCount;
            if(isGene)
                return G1RandomUnlockedGeneDist(gen);
            else
                return G1->unlockedGeneCount + G1RandomUnlockedmiRNADist(gen);    
        }
        else {
            bool isGene = G1->nodeTypes[source1] == "gene";
            if(isGene){
                int index = G1RandomUnlockedGeneDist(gen);
                return G1->geneIndexList[index]; 
            }
            else{
                int index =  G1RandomUnlockedmiRNADist(gen);
                return G1->miRNAIndexList[index]; 
            }
        }
        
    }
}

inline ushort SANA::G2RandomUnlockedNode(uint target1){
    if(!nodesHaveType){
        return G2RandomUnlockedNode_Fast();
    } else {
        ushort node;
        do {
            node = G2RandomUnlockedNode_Fast(); // gives back unlocked G2 node
        } while(G2->nodeTypes[target1] != G2->nodeTypes[(*unassignedNodesG2)[node]]);
        return node;
    }
}

inline ushort SANA::G2RandomUnlockedNode_Fast(){
    return G2RandomUnassignedNode(gen);
}

void SANA::describeParameters(ostream& sout) {
    sout << "Temperature schedule:" << endl;
    sout << "T_initial: " << TInitial << endl;
    sout << "T_decay: " << TDecay << endl;
    sout << "Optimize: " << endl;
    MC->printWeights(sout);

    if (!usingIterations){
        sout << "Execution time: ";
        if (restart) {sout << minutesNewAlignments + minutesPerCandidate*numCandidates + minutesFinalist;}
        else {sout << minutes;}
        sout << "m" << endl;
    } else {
        sout << "Iterations Run: " << maxIterations << "00,000,000" << endl; //it's in hundred millions
    }

    if (restart) {
        sout << "Restart scheme:" << endl;
        sout << "- new alignments: " << minutesNewAlignments << "m" << endl;
        sout << "- each candidate: " << minutesPerCandidate << "m" << endl;
        sout << "- finalist: " << minutesFinalist << "m" << endl;
        sout << "number candidates: " << numCandidates << endl;
        sout << "number new alignments: " << newAlignmentsCount << endl;
        sout << "iterations per new alignment: " << iterationsPerStep << endl;
    }
}

string SANA::fileNameSuffix(const Alignment& A) {
    return "_" + extractDecimals(eval(A),3);
}

void SANA::enableRestartScheme(double minutesNewAlignments, uint iterationsPerStep, uint numCandidates, double minutesPerCandidate, double minutesFinalist) {
    restart                    = true;
    this->minutesNewAlignments = minutesNewAlignments;
    this->iterationsPerStep    = iterationsPerStep;
    this->numCandidates        = numCandidates;
    this->minutesPerCandidate  = minutesPerCandidate;
    this->minutesFinalist      = minutesFinalist;
    candidates                 = vector<Alignment> (numCandidates, getStartingAlignment());
    candidatesScores           = vector<double> (numCandidates, 0);
    for (uint i = 0; i < numCandidates; i++) {
        candidatesScores[i] = eval(candidates[i]);
    }
}

double SANA::temperatureFunction(long long int iter, double TInitial, double TDecay) {
    double fraction;
    if(usingIterations)
        fraction = iter / ((double)(maxIterations)*100000000);
    else
        fraction = iter / (minutes * 60 * getIterPerSecond());
    return TInitial * (constantTemp ? 1 : exp(-TDecay* fraction ));
}

double SANA::acceptingProbability(double energyInc, double T) {
    return energyInc >= 0 ? 1 : exp(energyInc/T);
}

double SANA::trueAcceptingProbability(){
    return vectorMean(sampledProbability);
}

void SANA::initDataStructures(const Alignment& startA) {
    nodesHaveType = G1->hasNodeTypes();

    A = new vector<ushort>(startA.getMapping());
    storedAlignments->insert(A);

    assignedNodesG2 = new vector<bool> (n2, false);
    for (uint i = 0; i < n1; i++) {
        (*assignedNodesG2)[(*A)[i]] = true;
    }
    storedAssignedNodesG2[A] = assignedNodesG2;
    unassignedNodesG2        = new vector<ushort> (n2-n1);
    for (uint i = 0, j = 0; i < n2; i++) {
        if (not (*assignedNodesG2)[i]) {
            (*unassignedNodesG2)[j++] = i;
        }
    }
    storedUnassignedNodesG2[A] = unassignedNodesG2;
    //  Init unlockedNodesG1
    uint unlockedG1            = n1 - G1->getLockedCount();
    unLockedNodesG1            = vector<ushort> (unlockedG1);
    uint index                 = 0;
    for (uint i = 0; i < n1; i++){
        if(not G1->isLocked(i)){
            unLockedNodesG1[index++] = i;
        }
    }
    assert(index == unlockedG1);

    if (needAligEdges or needSec) {
        aligEdges          = startA.numAlignedEdges(*G1, *G2);
        storedAligEdges[A] = aligEdges;
    }

    if (needSquaredAligEdges) {
        squaredAligEdges          = startA.numSquaredAlignedEdges(*G1, *G2);
        storedSquaredAligEdges[A] = squaredAligEdges;
    }

    if (needInducedEdges) {
        inducedEdges          = G2->numNodeInducedSubgraphEdges(*A);
        storedInducedEdges[A] = inducedEdges;
    }

    if (needLocal) {
        localScoreSum = 0;
        for (uint i = 0; i < n1; i++) {
            localScoreSum += sims[i][(*A)[i]];
        }
        storedLocalScoreSum[A] = localScoreSum;
    }

    if (needWec) {
        Measure* wec    = MC->getMeasure("wec");
        double wecScore = wec->eval(*A);
        wecSum          = wecScore*2*g1Edges;
        storedWecSum[A] = wecSum;
    }

    if(needEwec){
        ewec             = (ExternalWeightedEdgeConservation*)(MC->getMeasure("ewec"));
        ewecSum          = ewec->eval(*A);
        storedEwecSum[A] = ewecSum;
    }

    if (needNC) {
        Measure* nc    = MC->getMeasure("nc");
        ncSum          = (nc->eval(*A))*trueA.back();
        storedNcSum[A] = ncSum;
    }

    if(needTC){
        Measure* tc = MC->getMeasure("tc");
        maxTriangles = ((TriangleCorrectness*)tc)->getMaxTriangles();
        TCSum = tc->eval(*A);
        storedTCSum[A] = TCSum;
    }

    iterationsPerformed = 0;
    sampledProbability.clear();

    currentScore = eval(startA);
    storedCurrentScore[A] = currentScore;
    timer.start();
}

double SANA::eval(const Alignment& Al) {
    return MC->eval(Al);
}

void SANA::setInterruptSignal() {
    interrupt = false;
    struct sigaction sigInt;
    sigInt.sa_handler = sigIntHandler;
    sigemptyset(&sigInt.sa_mask);
    sigInt.sa_flags = 0;
    sigaction(SIGINT, &sigInt, NULL);
}

Alignment SANA::simpleRun(const Alignment& startA, double maxExecutionSeconds,
        long long int& iter) {
    initDataStructures(startA);
    setInterruptSignal();

    for (; ; iter++) {
        T = temperatureFunction(iter, TInitial, TDecay);
        if (interrupt) {
            break;
        }
        if (iter%iterationsPerStep == 0) {
            trackProgress(iter);
            if (iter != 0 and timer.elapsed() > maxExecutionSeconds) {
                break;
            }
        }
        SANAIteration();
    }
    trackProgress(iter);
    return *A;
}

Alignment SANA::simpleRun(const Alignment& startA, double maxExecutionSeconds, long long int maxExecutionIterations,
		long long int& iter) {
    initDataStructures(startA);
    setInterruptSignal();
	for (; ; iter++) {
		T = temperatureFunction(iter, TInitial, TDecay);
		if (interrupt) {
			break; // return *A;
		}
		if (iter%iterationsPerStep == 0) {
			trackProgress(iter);
			if( iter != 0 and timer.elapsed() > maxExecutionSeconds and currentScore - previousScore < 0.005 ){
				break;
			}
			previousScore = currentScore;
		}
		if (iter != 0 and iter > maxExecutionIterations) {
			break;
		}
		SANAIteration();
	}
	trackProgress(iter);
	return *A; //dummy return to shut compiler warning
}

Alignment SANA::simpleRun(const Alignment& startA, long long int maxExecutionIterations,
                long long int& iter) {

        initDataStructures(startA);

        setInterruptSignal();

        for (; ; iter++) {
                T = temperatureFunction(iter, TInitial, TDecay);
                if (interrupt) {
                        break; // return *A;
                }
                if (iter%iterationsPerStep == 0) {
                        trackProgress(iter);

                }
                if (iter != 0 and iter > maxExecutionIterations) {
                        break; // return *A;
                }
                SANAIteration();
        }
        trackProgress(iter);
        return *A; //dummy return to shut compiler warning
}

unordered_set<vector<ushort>*>* SANA::simpleParetoRun(const Alignment& startA, double maxExecutionSeconds,
        long long int& iter) {

    initDataStructures(startA);
    setInterruptSignal();
    vector<string> measureNames; vector<double> scores;
    scoreNamesToIndexes = mapScoresToIndexes(measureNames);
    paretoFront = ParetoFront(paretoCapacity, numOfMeasures, measureNames);
    score = Score::pareto;
    for(auto iter = scoreNamesToIndexes.begin(); iter != scoreNamesToIndexes.end(); iter++)
        cout << iter->first << '\n';

    for (; ; iter++) {
	//T = temperatureFunction(iter, TInitial, TDecay);
	if (interrupt) {
	    return storedAlignments;
	}
	if (iter%iterationsPerStep == 0) {
	    trackProgress(iter);
	    if( iter != 0 and timer.elapsed() > maxExecutionSeconds){
		cout << "ending seconds " << timer.elapsed() << " " << maxExecutionSeconds << endl;
		paretoFront.printAlignmentScores(cout);
		return storedAlignments;
	    }
	}
	SANAIteration();
    }
    trackProgress(iter);

    return storedAlignments;
}
unordered_set<vector<ushort>*>* SANA::simpleParetoRun(const Alignment& startA, long long int maxExecutionIterations,
        long long int& iter) {

    initDataStructures(startA);
    setInterruptSignal();
    vector<string> measureNames; vector<double> scores;
    scoreNamesToIndexes = mapScoresToIndexes(measureNames);
    paretoFront = ParetoFront(paretoCapacity, numOfMeasures, measureNames);
    score = Score::pareto;
    for(auto iter = scoreNamesToIndexes.begin(); iter != scoreNamesToIndexes.end(); iter++)
        cout << iter->first << '\n';

    for (; ; iter++) {
    	//T = temperatureFunction(iter, TInitial, TDecay);
        if (interrupt) {
            return storedAlignments;
        }
        if (iter%iterationsPerStep == 0) {
            trackProgress(iter);
        }
        if (iter != 0 and iter > maxExecutionIterations) {
        	cout << "ending iterations " << iter << " " << maxExecutionIterations << endl;
        	paretoFront.printAlignmentScores(cout);
            return storedAlignments;
        }
        SANAIteration();
    }
    trackProgress(iter);
    return storedAlignments;
}

unordered_map<string, int> SANA::mapScoresToIndexes(vector<string> &measureNames) {
    numOfMeasures = MC->numMeasures();
    measureNames = vector<string>(numOfMeasures);
    for(int i = 0; i < numOfMeasures; i++)
        measureNames[i] = MC->getMeasure(i)->getName();
    sort(measureNames.begin(), measureNames.end());
    unordered_map<string, int> toReturn;
    for(int i = 0; i < numOfMeasures; i++)
    	toReturn[measureNames[i]] = i;
    return toReturn;
}

void SANA::prepareMeasureDataByAlignment() {
    aligEdges        = (needAligEdges or needSec) ?  storedAligEdges[A] : -1;
    squaredAligEdges = (needSquaredAligEdges) ?  storedSquaredAligEdges[A] : -1;
    inducedEdges     = (needInducedEdges) ?  storedInducedEdges[A] : -1;
    TCSum            = (needTC) ?  storedTCSum[A] : -1;
    localScoreSum    = (needLocal) ? storedLocalScoreSum[A] : -1;
    wecSum           = (needWec) ?  storedWecSum[A] : -1;
    ewecSum          = (needEwec) ?  storedEwecSum[A] : -1;
    ncSum            = (needNC) ? storedNcSum[A] : -1;
    //currentScore     = storedCurrentScore[A];
}

void SANA::insertCurrentAndPrepareNewMeasureDataByAlignment() {
	*newA = vector<ushort>(*A);
	storedAlignments->insert(newA);

    if(needAligEdges or needSec) storedAligEdges[A]        = aligEdges;
    if(needSquaredAligEdges)     storedSquaredAligEdges[A] = squaredAligEdges;
    if(needInducedEdges)         storedInducedEdges[A]     = inducedEdges;
    if(needTC)                   storedTCSum[A]            = TCSum;
    if(needLocal)                storedLocalScoreSum[A]    = localScoreSum;
    if(needWec)                  storedWecSum[A]           = wecSum;
    if(needEwec)                 storedEwecSum[A]          = ewecSum;
    if(needNC)                   storedNcSum[A]            = ncSum;
    ///*------------------------>*/storedCurrentScore[A]     = currentScore;

    A = paretoFront.procureRandomAlignment();
    prepareMeasureDataByAlignment();
}

void SANA::removeAlignmentData(vector<ushort>* toRemove) {
    storedAlignments->erase(toRemove);

    if(needAligEdges or needSec) storedAligEdges.erase(toRemove);
    if(needSquaredAligEdges)     storedSquaredAligEdges.erase(toRemove);
    if(needInducedEdges)         storedInducedEdges.erase(toRemove);
    if(needTC)                   storedTCSum.erase(toRemove);
    if(needLocal)                storedLocalScoreSum.erase(toRemove);
    if(needWec)                  storedWecSum.erase(toRemove);
    if(needEwec)                 storedEwecSum.erase(toRemove);
    if(needNC)                   storedNcSum.erase(toRemove);
    ///*------------------------>*/storedCurrentScore.erase(toRemove);
}

void SANA::SANAIteration() {
    iterationsPerformed++;
    if(G1->hasNodeTypes())
    {
	// Choose the type here based on counts (and locking...)
	// For example if no locking, then prob (gene) >> prob (miRNA)
	// And if locking, then arrange prob(gene) and prob(miRNA) appropriately
	int type = /* something clever */ 0;
	(randomReal(gen) < changeProbability[type]) ? performChange(type) : performSwap(type);
    }
    else
	(randomReal(gen) < changeProbability[0]) ? performChange(0) : performSwap(0);
}

void SANA::performChange(int type) {
    ushort source       = G1RandomUnlockedNode();
    ushort oldTarget    = (*A)[source];
    
    uint newTargetIndex = G2RandomUnlockedNode(oldTarget);
    ushort newTarget    = (*unassignedNodesG2)[newTargetIndex];

    int newAligEdges           = (needAligEdges or needSec) ?  aligEdges + aligEdgesIncChangeOp(source, oldTarget, newTarget) : -1;
    double newSquaredAligEdges = (needSquaredAligEdges) ?  squaredAligEdges + squaredAligEdgesIncChangeOp(source, oldTarget, newTarget) : -1;
    int newInducedEdges        = (needInducedEdges) ?  inducedEdges + inducedEdgesIncChangeOp(source, oldTarget, newTarget) : -1;
    double newTCSum            = (needTC) ?  TCSum + TCIncChangeOp(source, oldTarget, newTarget) : -1;
    double newLocalScoreSum    = (needLocal) ? localScoreSum + localScoreSumIncChangeOp(sims, source, oldTarget, newTarget) : -1;
    double newWecSum           = (needWec) ?  wecSum + WECIncChangeOp(source, oldTarget, newTarget) : -1;
    double newEwecSum          = (needEwec) ?  ewecSum + EWECIncChangeOp(source, oldTarget, newTarget) : -1;
    double newNcSum            = (needNC) ? ncSum + ncIncChangeOp(source, oldTarget, newTarget) : -1;

    map<string, double> newLocalScoreSumMap(*localScoreSumMap);
    if (needLocal) {
        for(auto &item : newLocalScoreSumMap)
            item.second += localScoreSumIncChangeOp(localSimMatrixMap[item.first], source, oldTarget, newTarget);
    }

    double newCurrentScore = 0;
    bool makeChange = scoreComparison(newAligEdges, newInducedEdges, newTCSum, newLocalScoreSum, newWecSum, newNcSum, newCurrentScore, newEwecSum, newSquaredAligEdges);
    if (makeChange) {
        (*A)[source]                         = newTarget;
        (*unassignedNodesG2)[newTargetIndex] = oldTarget;
        (*assignedNodesG2)[oldTarget]        = false;
        (*assignedNodesG2)[newTarget]        = true;
        aligEdges                            = newAligEdges;
        inducedEdges                         = newInducedEdges;
        TCSum                                = newTCSum;
        localScoreSum                        = newLocalScoreSum;
        wecSum                               = newWecSum;
        ewecSum                              = newEwecSum;
        ncSum                                = newNcSum;
        if (needLocal)
            (*localScoreSumMap) = newLocalScoreSumMap;
        if(score == Score::pareto and ((iterationsPerformed % 512) == 0)) //maybe create a boolean for pareto mode to avoid string comparison.
        	insertCurrentAndPrepareNewMeasureDataByAlignment();
#if 0
        if(randomReal(gen)<=1) {
        double foo = eval(*A);
        if(fabs(foo - newCurrentScore)>20){
            cout << "\nChange: nCS " << newCurrentScore << " (nSAE) " << newSquaredAligEdges << " eval " << foo << " nCS - eval " << newCurrentScore-foo;
            //cout << "source " << source << " oldTarget " << oldTarget << " newTarget " << newTarget << " adj? " << G2AdjMatrix[oldTarget][newTarget] << endl;
            newCurrentScore = newSquaredAligEdges = foo;
        } else cout << "c";
        }
#endif
        currentScore     = newCurrentScore;
        squaredAligEdges = newSquaredAligEdges;
    }
#ifdef CORES
    // Statistics on the emerging core alignment.
    // only update pBad if it's nonzero; re-use previous nonzero pBad if the current one is zero.
    static double pBad;
    if(trueAcceptingProbability()>0) pBad = trueAcceptingProbability();
    coreCount[source]++;
    coreFreq[source][(*A)[source]]++;
    weightedCoreFreq[source][(*A)[source]] += pBad;
    totalCoreWeight[source] += pBad;
#endif
}

void SANA::performSwap(int type) {
    ushort source1 = G1RandomUnlockedNode();
    ushort source2 = G1RandomUnlockedNode(source1);
    ushort target1 = (*A)[source1], target2 = (*A)[source2];

    int newAligEdges           = (needAligEdges or needSec) ?  aligEdges + aligEdgesIncSwapOp(source1, source2, target1, target2) : -1;
    int newTCSum               = (needTC) ?  TCSum + TCIncSwapOp(source1, source2, target1, target2) : -1;
    double newSquaredAligEdges = (needSquaredAligEdges) ? squaredAligEdges + squaredAligEdgesIncSwapOp(source1, source2, target1, target2) : -1;
    double newWecSum           = (needWec) ?  wecSum + WECIncSwapOp(source1, source2, target1, target2) : -1;
    double newEwecSum          = (needEwec) ?  ewecSum + EWECIncSwapOp(source1, source2, target1, target2) : -1;
    double newNcSum            = (needNC) ? ncSum + ncIncSwapOp(source1, source2, target1, target2) : -1;
    double newLocalScoreSum    = (needLocal) ? localScoreSum + localScoreSumIncSwapOp(sims, source1, source2, target1, target2) : -1;

    map<string, double> newLocalScoreSumMap(*localScoreSumMap);
    if (needLocal) {
        for (auto &item : newLocalScoreSumMap)
            item.second += localScoreSumIncSwapOp(localSimMatrixMap[item.first], source1, source2, target1, target2);
    }

    double newCurrentScore = 0;
    bool makeChange = scoreComparison(newAligEdges, inducedEdges, newTCSum, newLocalScoreSum, newWecSum, newNcSum, newCurrentScore, newEwecSum, newSquaredAligEdges);

    if (makeChange) {
        (*A)[source1]       = target2;
        (*A)[source2]       = target1;
        aligEdges           = newAligEdges;
        localScoreSum       = newLocalScoreSum;
        TCSum               = newTCSum;
        wecSum              = newWecSum;
        ewecSum             = newEwecSum;
        ncSum               = newNcSum;
        currentScore        = newCurrentScore;
        squaredAligEdges    = newSquaredAligEdges;
        if (needLocal)
            (*localScoreSumMap) = newLocalScoreSumMap;
        if (score == Score::pareto and (iterationsPerformed % 512 == 0)) //maybe create a boolean for pareto mode to avoid string comparison.
            insertCurrentAndPrepareNewMeasureDataByAlignment();

#if 0
        if (randomReal(gen) <= 1) {
            double foo = eval(*A);
            if (fabs(foo - newCurrentScore) > 20) {
                cout << "\nSwap: nCS " << newCurrentScore << " eval " << foo << " nCS - eval " << newCurrentScore - foo << " adj? " << (G1AdjMatrix[source1][source2] & G2AdjMatrix[target1][target2]);
                newCurrentScore = newSquaredAligEdges = foo;
            }
            else cout << "s";
        }
#endif
    }
#ifdef CORES
    // Statistics on the emerging core alignment.
    // only update pBad if it's nonzero; re-use previous nonzero pBad if the current one is zero.
    static double pBad;
    if(trueAcceptingProbability()>0) pBad = trueAcceptingProbability();

    coreCount[source1]++;
    coreFreq[source1][(*A)[source1]]++;
    weightedCoreFreq[source1][(*A)[source1]] += pBad;
    totalCoreWeight[source1] += pBad;

    coreCount[source2]++;
    coreFreq[source2][(*A)[source2]]++;
    weightedCoreFreq[source2][(*A)[source2]] += pBad;
    totalCoreWeight[source2] += pBad;
#endif
}

bool SANA::scoreComparison(double newAligEdges, double newInducedEdges, double newTCSum, double newLocalScoreSum, double newWecSum, double newNcSum, double& newCurrentScore, double newEwecSum, double newSquaredAligEdges) {
    bool makeChange = false;
    bool wasBadMove = false;
    double badProbability = 0;

    switch (score)
    {
    case Score::sum:
    {
        newCurrentScore += ecWeight * (newAligEdges / g1Edges);
        newCurrentScore += s3Weight * (newAligEdges / (g1Edges + newInducedEdges - newAligEdges));
        newCurrentScore += icsWeight * (newAligEdges / newInducedEdges);
        newCurrentScore += secWeight * (newAligEdges / g1Edges + newAligEdges / g2Edges)*0.5;
        newCurrentScore += TCWeight * (newTCSum);
        newCurrentScore += localWeight * (newLocalScoreSum / n1);
        newCurrentScore += wecWeight * (newWecSum / (2 * g1Edges));
        newCurrentScore += ewecWeight * (newEwecSum);
        newCurrentScore += ncWeight * (newNcSum / trueA.back());
#ifdef WEIGHTED
        newCurrentScore += mecWeight * (newAligEdges / (g1WeightedEdges + g2WeightedEdges));
        newCurrentScore += sesWeight * newSquaredAligEdges / SquaredEdgeScore::getDenom();
#endif
        energyInc = newCurrentScore - currentScore;
        wasBadMove = energyInc < 0;
        badProbability = exp(energyInc / T);
        makeChange = (energyInc >= 0 or randomReal(gen) <= badProbability);
        break;
    }
    case Score::product:
    {
        newCurrentScore = 1;
        newCurrentScore *= ecWeight * (newAligEdges / g1Edges);
        newCurrentScore *= s3Weight * (newAligEdges / (g1Edges + newInducedEdges - newAligEdges));
        newCurrentScore *= icsWeight * (newAligEdges / newInducedEdges);
        newCurrentScore *= TCWeight * (newTCSum);
        newCurrentScore *= localWeight * (newLocalScoreSum / n1);
        newCurrentScore *= secWeight * (newAligEdges / g1Edges + newAligEdges / g2Edges)*0.5;
        newCurrentScore *= wecWeight * (newWecSum / (2 * g1Edges));
        newCurrentScore *= ncWeight * (newNcSum / trueA.back());
        energyInc = newCurrentScore - currentScore;
        wasBadMove = energyInc < 0;
        badProbability = exp(energyInc / T);
        makeChange = (energyInc >= 0 or randomReal(gen) <= exp(energyInc / T));
        break;
    }
    case Score::max:
    {
	// this is a terrible way to compute the max; we should loop through all of them and figure out which is the biggest
	// and in fact we haven't yet integrated icsWeight here yet, so assert so
	assert(icsWeight == 0.0);
        double deltaEnergy = max(ncWeight* (newNcSum / trueA.back() - ncSum / trueA.back()), max(max(ecWeight*(newAligEdges / g1Edges - aligEdges / g1Edges), max(
            s3Weight*((newAligEdges / (g1Edges + newInducedEdges - newAligEdges) - (aligEdges / (g1Edges + inducedEdges - aligEdges)))),
            secWeight*0.5*(newAligEdges / g1Edges - aligEdges / g1Edges + newAligEdges / g2Edges - aligEdges / g2Edges))),
            max(localWeight*((newLocalScoreSum / n1) - (localScoreSum)),
                wecWeight*(newWecSum / (2 * g1Edges) - wecSum / (2 * g1Edges)))));

        newCurrentScore += ecWeight * (newAligEdges / g1Edges);
        newCurrentScore += secWeight * (newAligEdges / g1Edges + newAligEdges / g2Edges)*0.5;
        newCurrentScore += s3Weight * (newAligEdges / (g1Edges + newInducedEdges - newAligEdges));
        newCurrentScore += icsWeight * (newAligEdges / newInducedEdges);
        newCurrentScore += localWeight * (newLocalScoreSum / n1);
        newCurrentScore += wecWeight * (newWecSum / (2 * g1Edges));
        newCurrentScore += ncWeight * (newNcSum / trueA.back());

        energyInc = newCurrentScore - currentScore;
        wasBadMove = energyInc < 0;
        badProbability = exp(energyInc / T);
        makeChange = deltaEnergy >= 0 or randomReal(gen) <= exp(energyInc / T);
        break;
    }
    case Score::min:
    {
	// see comment above in max
	assert(icsWeight == 0.0);
        double deltaEnergy = min(ncWeight* (newNcSum / trueA.back() - ncSum / trueA.back()), min(min(ecWeight*(newAligEdges / g1Edges - aligEdges / g1Edges), min(
            s3Weight*((newAligEdges / (g1Edges + newInducedEdges - newAligEdges) - (aligEdges / (g1Edges + inducedEdges - aligEdges)))),
            secWeight*0.5*(newAligEdges / g1Edges - aligEdges / g1Edges + newAligEdges / g2Edges - aligEdges / g2Edges))),
            min(localWeight*((newLocalScoreSum / n1) - (localScoreSum)),
                wecWeight*(newWecSum / (2 * g1Edges) - wecSum / (2 * g1Edges)))));

        newCurrentScore += ecWeight * (newAligEdges / g1Edges);
        newCurrentScore += s3Weight * (newAligEdges / (g1Edges + newInducedEdges - newAligEdges));
        newCurrentScore += icsWeight * (newAligEdges / newInducedEdges);
        newCurrentScore += secWeight * (newAligEdges / g1Edges + newAligEdges / g2Edges)*0.5;
        newCurrentScore += localWeight * (newLocalScoreSum / n1);
        newCurrentScore += wecWeight * (newWecSum / (2 * g1Edges));
        newCurrentScore += ncWeight * (newNcSum / trueA.back());

        energyInc = newCurrentScore - currentScore; //is this even used?
        wasBadMove = deltaEnergy < 0;
        badProbability = exp(energyInc / T);
        makeChange = deltaEnergy >= 0 or randomReal(gen) <= exp(newCurrentScore / T);
        break;
    }
    case Score::inverse:
    {
        newCurrentScore += ecWeight / (newAligEdges / g1Edges);
        newCurrentScore += secWeight * (newAligEdges / g1Edges + newAligEdges / g2Edges)*0.5;
        newCurrentScore += s3Weight / (newAligEdges / (g1Edges + newInducedEdges - newAligEdges));
        newCurrentScore += icsWeight / (newAligEdges / newInducedEdges);
        newCurrentScore += localWeight / (newLocalScoreSum / n1);
        newCurrentScore += wecWeight / (newWecSum / (2 * g1Edges));
        newCurrentScore += ncWeight / (newNcSum / trueA.back());

        energyInc = newCurrentScore - currentScore;
        wasBadMove = energyInc < 0;
        badProbability = exp(energyInc / T);
        makeChange = (energyInc >= 0 or randomReal(gen) <= exp(energyInc / T));
        break;
    }
    case Score::maxFactor:
    {
	assert(icsWeight == 0.0);
        double maxScore = max(ncWeight*(newNcSum / trueA.back() - ncSum / trueA.back()), max(max(ecWeight*(newAligEdges / g1Edges - aligEdges / g1Edges), max(
            s3Weight*((newAligEdges / (g1Edges + newInducedEdges - newAligEdges) - (aligEdges / (g1Edges + inducedEdges - aligEdges)))),
            secWeight*0.5*(newAligEdges / g1Edges - aligEdges / g1Edges + newAligEdges / g2Edges - aligEdges / g2Edges))),
            max(localWeight*((newLocalScoreSum / n1) - (localScoreSum)),
                wecWeight*(newWecSum / (2 * g1Edges) - wecSum / (2 * g1Edges)))));

        double minScore = min(ncWeight*(newNcSum / trueA.back() - ncSum / trueA.back()), min(min(ecWeight*(newAligEdges / g1Edges - aligEdges / g1Edges), min(
            s3Weight*((newAligEdges / (g1Edges + newInducedEdges - newAligEdges) - (aligEdges / (g1Edges + inducedEdges - aligEdges)))),
            secWeight*0.5*(newAligEdges / g1Edges - aligEdges / g1Edges + newAligEdges / g2Edges - aligEdges / g2Edges))),
            min(localWeight*((newLocalScoreSum / n1) - (localScoreSum)),
                wecWeight*(newWecSum / (2 * g1Edges) - wecSum / (2 * g1Edges)))));

        newCurrentScore += ecWeight * (newAligEdges / g1Edges);
        newCurrentScore += secWeight * (newAligEdges / g1Edges + newAligEdges / g2Edges)*0.5;
        newCurrentScore += s3Weight * (newAligEdges / (g1Edges + newInducedEdges - newAligEdges));
        newCurrentScore += icsWeight * (newAligEdges / newInducedEdges);
        newCurrentScore += localWeight * (newLocalScoreSum / n1);
        newCurrentScore += wecWeight * (newWecSum / (2 * g1Edges));
        newCurrentScore += ncWeight * (newNcSum / trueA.back());

        energyInc = newCurrentScore - currentScore;
        wasBadMove = maxScore < -1 * minScore;
        badProbability = exp(energyInc / T);
        makeChange = maxScore >= -1 * minScore or randomReal(gen) <= exp(energyInc / T);
        break;
    }
    case Score::pareto: //Short circuit return to let the pareto front decide
    {
        if ((iterationsPerformed % 512 != 0))
            return true;
        vector<double> addScores(numOfMeasures);  //what alignments to keep instead of simulated annealing.
        addScores[scoreNamesToIndexes["ec"]] = (1.0*newAligEdges / g1Edges);
        addScores[scoreNamesToIndexes["s3"]] = (1.0*newAligEdges / (g1Edges + newInducedEdges - newAligEdges));
        addScores[scoreNamesToIndexes["ics"]] = (1.0*newAligEdges / newInducedEdges);
        addScores[scoreNamesToIndexes["sec"]] = (1.0*newAligEdges / g1Edges + newAligEdges / g2Edges)*0.5;
        addScores[scoreNamesToIndexes["tc"]] = (1.0*newTCSum);
        addScores[scoreNamesToIndexes["local"]] = (1.0*newLocalScoreSum / n1);
        addScores[scoreNamesToIndexes["wec"]] = (1.0*newWecSum / (2 * g1Edges));
        //addScores[scoreNamesToIndexes["ewec"]] = ewecWeight * (newEwecSum);
        addScores[scoreNamesToIndexes["nc"]] = (1.0*newNcSum / trueA.back());
#ifdef WEIGHTED
        addScores[scoreNamesToIndexes["mec"]] += (1.0*newAligEdges / (g1WeightedEdges + g2WeightedEdges));
        addScores[scoreNamesToIndexes["ses"]] += 1.0*newSquaredAligEdges/SquaredEdgeScore::getDenom();
#endif
        /*cout << "aligEdges: " << newAligEdges << endl;
        cout << "g1Edges: " << g1Edges << endl;
        cout << "newInducedEdges: " << newInducedEdges << endl;
        cout << "g2Edges: " << g2Edges << endl;
        cout << "newTCSum: " << newTCSum << endl;
        cout << "newLocalScoreSum: " << newLocalScoreSum << endl;
        cout << "n1: " << n1 << endl;
        cout << "newWecSum: " << newWecSum << endl;
        cout << "newNcSum: " << newNcSum << endl;
        cout << "trueA_back: " << trueA.back() << endl;
#ifdef WEIGHTED
        cout << "g1WeightedEdges: " << g1WeightedEdges << endl;
        cout << "g2WeightedEdges: " << g2WeightedEdges << endl;
        cout << "newSquaredAligEdges: " << newSquaredAligEdges << endl;
#endif*/
        newA = new vector<ushort>(0);
        vector<vector<ushort>*> toRemove = paretoFront.addAlignmentScores(newA, addScores, makeChange);
        if (makeChange) {
            for (unsigned int i = 0; i < toRemove.size(); i++)
                removeAlignmentData(toRemove[i]);
        }
        //std::cout << "ParetoFront:\n";
        //paretoFront.printAlignmentScores(cout);
        //cin.get();
        return makeChange;
        break;
    }
    }

    if (wasBadMove && (iterationsPerformed % 512 == 0 || (TCWeight > 0 && iterationsPerformed % 32 == 0))) { //this will never run in the case of iterationsPerformed never being changed so that it doesn't greatly slow down the program if for some reason iterationsPerformed doesn't need to be changed.
        if (sampledProbability.size() == 1000) {
            sampledProbability.erase(sampledProbability.begin());
        }
        sampledProbability.push_back(badProbability);
    }
    return makeChange;
}

int SANA::aligEdgesIncChangeOp(ushort source, ushort oldTarget, ushort newTarget) {
    int res = 0;
    const uint n = G1AdjLists[source].size();
    ushort neighbor;
    for (uint i = 0; i < n; ++i) {
        neighbor = G1AdjLists[source][i];
        res -= G2AdjMatrix[oldTarget][(*A)[neighbor]];
        res += G2AdjMatrix[newTarget][(*A)[neighbor]];
    }
    return res;
}

int SANA::aligEdgesIncSwapOp(ushort source1, ushort source2, ushort target1, ushort target2) {
    int res = 0;
    const uint n = G1AdjLists[source1].size();
    ushort neighbor;
    uint i = 0;
    for (; i < n; ++i) {
        neighbor = G1AdjLists[source1][i];
        res -= G2AdjMatrix[target1][(*A)[neighbor]];
        res += G2AdjMatrix[target2][(*A)[neighbor]];
    }
    const uint m = G1AdjLists[source2].size();
    for (i = 0; i < m; ++i) {
        neighbor = G1AdjLists[source2][i];
        res -= G2AdjMatrix[target2][(*A)[neighbor]];
        res += G2AdjMatrix[target1][(*A)[neighbor]];
    }
    //address case swapping between adjacent nodes with adjacent images:
#ifdef WEIGHTED
    res += (-1 << 1) & (G1AdjMatrix[source1][source2] + G2AdjMatrix[target1][target2]);
#else
    res += 2*(G1AdjMatrix[source1][source2] & G2AdjMatrix[target1][target2]);
#endif
    return res;
}

static int _edgeVal;
// UGLY GORY HACK BELOW!! Sometimes the edgeVal is crazily wrong, like way above 1,000, when it
// cannot possibly be greater than the number of networks we're aligning when WEIGHTED is on.
// It happens only rarely, so here I ask if the edgeVal is less than 1,000; if it's less than 1,000
// then we assume it's OK, otherwise we just ignore this edge entirely and say the diff is 0.
// Second problem: even if the edgeVal is correct, I couldn't seem to figure out the difference
// between the value of this ladder and the ladder with one edge added or removed.  Mathematically
// it should be edgeVal^2 - (edgeVal+1)^2 which is (2e + 1), but for some reason I had to make
// it 2*(e+1).  That seemed to work better.  So yeah... big ugly hack.
#define SQRDIFF(i,j) ((_edgeVal=G2AdjMatrix[i][(*A)[j]]), 2*((_edgeVal<1000?_edgeVal:0) + 1))
int SANA::squaredAligEdgesIncChangeOp(ushort source, ushort oldTarget, ushort newTarget) {
    int res = 0, diff;
    ushort neighbor;
    const uint n = G1AdjLists[source].size();
    for (uint i = 0; i < n; ++i) {
        neighbor = G1AdjLists[source][i];
        // Account for ushort edges? Or assume smaller graph is edge value 1?
        diff = SQRDIFF(oldTarget, neighbor);
        assert(fabs(diff)<1100);
        res -= diff>0?diff:0;
        diff = SQRDIFF(newTarget, neighbor);
        assert(fabs(diff)<1100);
        res += diff>0?diff:0;
    }
    return res;
}

int SANA::squaredAligEdgesIncSwapOp(ushort source1, ushort source2, ushort target1, ushort target2) {
    int res = 0, diff;
    ushort neighbor;
    const uint n = G1AdjLists[source1].size();
    uint i = 0;
    for (; i < n; ++i) {
        neighbor = G1AdjLists[source1][i];
        diff = SQRDIFF(target1, neighbor);
        assert(fabs(diff)<1100);
        res -= diff>0?diff:0;
        diff = SQRDIFF(target2, neighbor);
        assert(fabs(diff)<1100);
        res += diff>0?diff:0;
    }
    const uint m = G1AdjLists[source2].size();
    for (i = 0; i < m; ++i) {
        neighbor = G1AdjLists[source2][i];
        diff = SQRDIFF(target2, neighbor);
        assert(fabs(diff)<1100);
        res -= diff>0?diff:0;
        diff = SQRDIFF(target1, neighbor);
        assert(fabs(diff)<1100);
        res += diff>0?diff:0;
    }
    // How to do for squared?
    // address case swapping between adjacent nodes with adjacent images:
    if(G1AdjMatrix[source1][source2] and G2AdjMatrix[target1][target2])
    {
        res += 2 * SQRDIFF(target1,target2);
    }
    return res;
}

int SANA::inducedEdgesIncChangeOp(ushort source, ushort oldTarget, ushort newTarget) {
    int res = 0;
    const uint n = G2AdjLists[oldTarget].size();
    ushort neighbor;
    uint i = 0;
    for (; i < n; ++i) {
        neighbor = G2AdjLists[oldTarget][i];
        res -= (*assignedNodesG2)[neighbor];
    }
    const uint m = G2AdjLists[newTarget].size();
    for (i = 0; i < m; ++i) {
        neighbor = G2AdjLists[newTarget][i];
        res += (*assignedNodesG2)[neighbor];
    }
    //address case changing between adjacent nodes:
    res -= G2AdjMatrix[oldTarget][newTarget];
    return res;
}

double SANA::localScoreSumIncChangeOp(vector<vector<float> > const & sim, ushort const & source, ushort const & oldTarget, ushort const & newTarget) {
    return sim[source][newTarget] - sim[source][oldTarget];
}

double SANA::localScoreSumIncSwapOp(vector<vector<float> > const & sim, ushort const & source1, ushort const & source2, ushort const & target1, ushort const & target2) {
    return sim[source1][target2] - sim[source1][target1] + sim[source2][target1] - sim[source2][target2];
}

double SANA::WECIncChangeOp(ushort source, ushort oldTarget, ushort newTarget) {
    double res = 0;
    const uint n = G1AdjLists[source].size();
    ushort neighbor;
    for (uint j = 0; j < n; ++j) {
        neighbor = G1AdjLists[source][j];
        if (G2AdjMatrix[oldTarget][(*A)[neighbor]]) {
            res -= wecSims[source][oldTarget];
            res -= wecSims[neighbor][(*A)[neighbor]];
        }
        if (G2AdjMatrix[newTarget][(*A)[neighbor]]) {
            res += wecSims[source][newTarget];
            res += wecSims[neighbor][(*A)[neighbor]];
        }
    }
    return res;
}

double SANA::WECIncSwapOp(ushort source1, ushort source2, ushort target1, ushort target2) {
    double res = 0;
    const uint n = G1AdjLists[source1].size();
    ushort neighbor;
    for (uint j = 0; j < n; ++j) {
        neighbor = G1AdjLists[source1][j];
        if (G2AdjMatrix[target1][(*A)[neighbor]]) {
            res -= wecSims[source1][target1];
            res -= wecSims[neighbor][(*A)[neighbor]];
        }
        if (G2AdjMatrix[target2][(*A)[neighbor]]) {
            res += wecSims[source1][target2];
            res += wecSims[neighbor][(*A)[neighbor]];
        }
    }
    const uint m = G1AdjLists[source2].size();
    for (uint j = 0; j < m; ++j) {
        neighbor = G1AdjLists[source2][j];
        if (G2AdjMatrix[target2][(*A)[neighbor]]) {
            res -= wecSims[source2][target2];
            res -= wecSims[neighbor][(*A)[neighbor]];
        }
        if (G2AdjMatrix[target1][(*A)[neighbor]]) {
            res += wecSims[source2][target1];
            res += wecSims[neighbor][(*A)[neighbor]];
        }
    }
    //address case swapping between adjacent nodes with adjacent images:
#ifdef WEIGHTED
    if (G1AdjMatrix[source1][source2] > 0 and G2AdjMatrix[target1][target2] > 0) {
#else
    if (G1AdjMatrix[source1][source2] and G2AdjMatrix[target1][target2]) {
#endif
        res += 2*wecSims[source1][target1];
        res += 2*wecSims[source2][target2];
    }
    return res;
}

double SANA::EWECIncChangeOp(ushort source, ushort oldTarget, ushort newTarget){
    double score = 0;
    score = (EWECSimCombo(source, newTarget)) - (EWECSimCombo(source, oldTarget));
    return score;
}

double SANA::EWECIncSwapOp(ushort source1, ushort source2, ushort target1, ushort target2){
    double score = 0;
    score = (EWECSimCombo(source1, target2)) + (EWECSimCombo(source2, target1)) - (EWECSimCombo(source1, target1)) - (EWECSimCombo(source2, target2));
    if(G1AdjMatrix[source1][source2] and G2AdjMatrix[target1][target2]){
        score += ewec->getScore(ewec->getColIndex(target1, target2), ewec->getRowIndex(source1, source2))/(g1Edges); //correcting for missed edges when swapping 2 adjacent pairs
    }
    return score;
}

double SANA::EWECSimCombo(ushort source, ushort target){
    double score = 0;
    const uint n = G1AdjLists[source].size();
    ushort neighbor;
    for (uint i = 0; i < n; ++i) {
        neighbor = G1AdjLists[source][i];
        if (G2AdjMatrix[target][(*A)[neighbor]]) {
            int e1 = ewec->getRowIndex(source, neighbor);
            int e2 = ewec->getColIndex(target, (*A)[neighbor]);
            score+=ewec->getScore(e2,e1);
        }
    }
    return score/(2*g1Edges);
}

double SANA::TCIncChangeOp(ushort source, ushort oldTarget, ushort newTarget){
    double deltaTriangles = 0;
    const uint n = G1AdjLists[source].size();
    ushort neighbor1, neighbor2;
    for(uint i = 0; i < n; ++i){
        for(uint j = i+1; j < n; ++j){
            neighbor1 = G1AdjLists[source][i];
            neighbor2 = G1AdjLists[source][j];
            if(G1AdjMatrix[neighbor1][neighbor2]){
                //G1 has a triangle
                if(G2AdjMatrix[oldTarget][(*A)[neighbor1]] and G2AdjMatrix[oldTarget][(*A)[neighbor2]] and G2AdjMatrix[(*A)[neighbor1]][(*A)[neighbor2]]){
                    //G2 HAD a triangle
                    deltaTriangles -= 1;
                }

                if(G2AdjMatrix[newTarget][(*A)[neighbor1]] and G2AdjMatrix[newTarget][(*A)[neighbor2]] and G2AdjMatrix[(*A)[neighbor1]][(*A)[neighbor2]]){
                    //G2 GAINS a triangle
                    deltaTriangles += 1;
                }
            }
        }
    }
    return ((double)deltaTriangles/maxTriangles);
}

double SANA::TCIncSwapOp(ushort source1, ushort source2, ushort target1, ushort target2){
    double deltaTriangles = 0;
    const uint n = G1AdjLists[source1].size();
    ushort neighbor1, neighbor2;
    for(uint i = 0; i < n; ++i){
        for(uint j = i+1; j < n; ++j){
            neighbor1 = G1AdjLists[source1][i];
            neighbor2 = G1AdjLists[source1][j];
            if(G1AdjMatrix[neighbor1][neighbor2]){
                //G1 has a triangle
                if(G2AdjMatrix[target1][(*A)[neighbor1]] and G2AdjMatrix[target1][(*A)[neighbor2]] and G2AdjMatrix[(*A)[neighbor1]][(*A)[neighbor2]]){
                    //G2 HAD a triangle
                    deltaTriangles -= 1;
                }

                if((G2AdjMatrix[target2][(*A)[neighbor1]] and G2AdjMatrix[target2][(*A)[neighbor2]] and G2AdjMatrix[(*A)[neighbor1]][(*A)[neighbor2]])
                   || (neighbor1 == source2 and G2AdjMatrix[target2][target1] and G2AdjMatrix[target2][(*A)[neighbor2]] and G2AdjMatrix[target1][(*A)[neighbor2]])
                   || (neighbor2 == source2 and G2AdjMatrix[target2][(*A)[neighbor1]] and G2AdjMatrix[target2][target1] and G2AdjMatrix[(*A)[neighbor1]][target1])){
                    //G2 GAINS a triangle
                    deltaTriangles += 1;
                }
            }
        }
    }
    const uint m = G1AdjLists[source2].size();
    for(uint i = 0; i < m; ++i){
        for(uint j = i+1; j < m; ++j){
            neighbor1 = G1AdjLists[source2][i];
            neighbor2 = G1AdjLists[source2][j];
            if(G1AdjMatrix[neighbor1][neighbor2]){
                //G1 has a triangle
                if(G2AdjMatrix[target2][(*A)[neighbor1]] and G2AdjMatrix[target2][(*A)[neighbor2]] and G2AdjMatrix[(*A)[neighbor1]][(*A)[neighbor2]]){
                    //G2 HAD a triangle
                    deltaTriangles -= 1;
                }

                if((G2AdjMatrix[target1][(*A)[neighbor1]] and G2AdjMatrix[target1][(*A)[neighbor2]] and G2AdjMatrix[(*A)[neighbor1]][(*A)[neighbor2]])
                   || (neighbor1 == source1 and G2AdjMatrix[target1][target2] and G2AdjMatrix[target1][(*A)[neighbor2]] and G2AdjMatrix[target2][(*A)[neighbor2]])
                   || (neighbor2 == source1 and G2AdjMatrix[target1][(*A)[neighbor1]] and G2AdjMatrix[target1][target2] and G2AdjMatrix[(*A)[neighbor1]][target2])){
                    //G2 GAINS a triangle
                    deltaTriangles += 1;
                }
            }
        }
    }

    return ((double)deltaTriangles/maxTriangles);
}

int SANA::ncIncChangeOp(ushort source, ushort oldTarget, ushort newTarget) {
    int change = 0;
    if (trueA[source] == oldTarget) change -= 1;
    if (trueA[source] == newTarget) change += 1;
    return change;
}

int SANA::ncIncSwapOp(ushort source1, ushort source2, ushort target1, ushort target2) {
    int change = 0;
    if(trueA[source1] == target1) change -= 1;
    if(trueA[source2] == target2) change -= 1;
    if(trueA[source1] == target2) change += 1;
    if(trueA[source2] == target1) change += 1;
    return change;
}

void SANA::trackProgress(long long int i, bool end) {
    if (!enableTrackProgress) return;
    bool printDetails = false;
    bool printScores = false;
    bool checkScores = true;

    cout << i/iterationsPerStep << " (" << timer.elapsed() << "s): score = " << currentScore;
    cout <<  " P(" << avgEnergyInc << ", " << T << ") = " << acceptingProbability(avgEnergyInc, T) << ", pBad = " << trueAcceptingProbability() << endl;

    if (not (printDetails or printScores or checkScores)) return;
    Alignment Al(*A);
    //original one is commented out for testing sec
    //if (printDetails) cout << " (" << Al.numAlignedEdges(*G1, *G2) << ", " << G2->numNodeInducedSubgraphEdges(*A) << ")";
    if (printDetails) 
        cout << "Al.numAlignedEdges = " << Al.numAlignedEdges(*G1, *G2) << ", g1Edges = " <<g1Edges<< " ,g2Edges = "<<g2Edges<< endl;
    if (printScores) {
        SymmetricSubstructureScore S3(G1, G2);
        EdgeCorrectness EC(G1, G2);
        InducedConservedStructure ICS(G1, G2);
        SymmetricEdgeCoverage SEC(G1,G2);
        cout << "S3: " << S3.eval(Al) << "  EC: " << EC.eval(Al) << "  ICS: " << ICS.eval(Al) << "  SEC: " << SEC.eval(Al) <<endl;
    }
    if (checkScores) {
        double realScore = eval(Al);
        if (fabs(realScore-currentScore) > 0.000001) {
            cerr << "internal error: incrementally computed score (" << currentScore;
            cerr << ") is not correct (" << realScore << ")" << endl;
            currentScore = realScore;
        }
    }
    if (dynamic_tdecay) { // Code for estimating dynamic TDecay
        //The dynamic method uses linear interpolation to obtain an
        //an "ideal" P(bad) as a basis for SANA runs. If the current P(bad)
        //is significantly different from out "ideal" P(bad), then decay is either
        //"sped up" or "slowed down"
        int NSteps = 100;
        double fractional_time = (timer.elapsed()/(minutes*60));
        double lowIndex = floor(NSteps*fractional_time);
        double highIndex = ceil(NSteps*fractional_time);
        double betweenFraction = NSteps*fractional_time - lowIndex;
        double PLow = tau[lowIndex];
        double PHigh = tau[highIndex];

        double PBetween = PLow + betweenFraction * (PHigh - PLow);

        // if the ratio if off by more than a few percent, adjust.
        double ratio = acceptingProbability(avgEnergyInc, T) / PBetween;
        if (abs(1-ratio) >= .01 &&
            (ratio < 1 || SANAtime > .2)) // don't speed it up too soon
        {
            double shouldBe;
            shouldBe = -log(avgEnergyInc/(TInitial*log(PBetween)))/(SANAtime);
            if(SANAtime==0 || shouldBe != shouldBe || shouldBe <= 0)
            shouldBe = TDecay * (ratio >= 0 ? ratio*ratio : 0.5);
            cout << "TDecay " << TDecay << " too ";
            cout << (ratio < 1 ? "fast" : "slow") << " shouldBe " << shouldBe;
            TDecay = sqrt(TDecay * shouldBe); // geometric mean
            cout << "; try " << TDecay << endl;
        }
    }
}

Alignment SANA::runRestartPhases() {
    cout << "new alignments phase" << endl;
    Timer T;
    T.start();
    newAlignmentsCount = 0;
    while (T.elapsed() < minutesNewAlignments*60) {
        long long int iter = 0;
        // Alignment A = simpleRun(Alignment::random(n1, n2), 0.0, iter);
        Alignment A = simpleRun(getStartingAlignment(), 0.0, iter);
        uint i = getLowestIndex();
        double lowestScore = candidatesScores[i];
        if (currentScore > lowestScore) {
            candidates[i] = A;
            candidatesScores[i] = currentScore;
        }
        newAlignmentsCount++;
    }
    cout << "candidates phase" << endl;
    vector<long long int> iters(numCandidates, iterationsPerStep);
    for (uint i = 0; i < numCandidates; i++) {
        candidates[i] = simpleRun(candidates[i], minutesPerCandidate*60, iters[i]);
        candidatesScores[i] = currentScore;
    }
    cout << "finalist phase" << endl;
    uint i = getHighestIndex();
    return simpleRun(candidates[i], minutesFinalist*60, iters[i]);
}

#ifdef CORES
vector<ulong> SANA::getCoreCount() {
    return coreCount;
}
vector<vector<ulong>> SANA::getCoreFreq() {
    return coreFreq;
}
#endif

uint SANA::getLowestIndex() const {
    double lowestScore = candidatesScores[0];
    uint lowestIndex = 0;
    for (uint i = 1; i < numCandidates; i++) {
        if (candidatesScores[i] < lowestScore) {
            lowestScore = candidatesScores[i];
            lowestIndex = i;
        }
    }
    return lowestIndex;
}

uint SANA::getHighestIndex() const {
    double highestScore = candidatesScores[0];
    uint highestIndex = 0;
    for (uint i = 1; i < numCandidates; i++) {
        if (candidatesScores[i] > highestScore) {
            highestScore = candidatesScores[i];
            highestIndex = i;
        }
    }
    return highestIndex;
}

#define LOG10_LOW_TEMP -10
#define LOG10_HIGH_TEMP 10

void SANA::searchTemperaturesByLinearRegression() {

    //if(score == "pareto") //Running in pareto mode makes this function really slow
    //	return;             //and I don't know why, but sometimes I disable using this.
    //                      //otherwise my computer is very slow.
    map<double, double> pbadMap;
    cout << "Sampling pbads from 1E" << LOG10_LOW_TEMP<< " to 1E" << LOG10_HIGH_TEMP <<" for linear regression" << endl;
    for(double log_temp = LOG10_LOW_TEMP; log_temp <= LOG10_HIGH_TEMP; log_temp = log_temp + 1.0){
        pbadMap[log_temp] = pForTInitial(pow(10, log_temp));
        cout << log_temp << " temperature: " << pow(10, log_temp) << " pBad: " << pbadMap[log_temp] << endl;
    }
    double exponent;
    for (exponent = LOG10_LOW_TEMP; exponent <= LOG10_HIGH_TEMP; exponent++){
        if(pbadMap[exponent] > FinalPBad)
            break;
    }
    double binarySearchLeftEnd = exponent - 1;
    double binarySearchRightEnd = exponent;
    double mid = (binarySearchRightEnd + binarySearchLeftEnd) / 2;
    cout << "Beginning binary search for tFinal. " << "left bound: " << pow(10, binarySearchLeftEnd) << ", right bound: " << pow(10, binarySearchRightEnd) << endl;
    for(int j = 0; j < 4; j++){
        double temperature = pow(10, mid);
        double probability = pForTInitial(temperature);
        pbadMap[mid] = probability;
        cout << "Temperature: " << temperature << " pbad: " << probability << endl;
        if(probability > FinalPBad){
            binarySearchRightEnd = mid;
            mid = (binarySearchRightEnd + binarySearchLeftEnd) / 2;
        } else if(probability < FinalPBad/10){
            binarySearchLeftEnd = mid;
            mid = (binarySearchRightEnd + binarySearchLeftEnd) / 2;
        }
    }
    for (exponent = LOG10_HIGH_TEMP; exponent >= LOG10_LOW_TEMP; exponent--){
        if(pbadMap[exponent] < 0.985)
            break;
    }
    binarySearchLeftEnd = exponent;
    binarySearchRightEnd = exponent + 1;
    mid = (binarySearchRightEnd + binarySearchLeftEnd) / 2;
    cout << "Beginning binary search for tInitial. " << "left bound: " << pow(10, binarySearchLeftEnd) << ", right bound: " << pow(10, binarySearchRightEnd) << endl;
    for(int j = 0; j < 4; j++){
        double temperature = pow(10, mid);
        double probability = pForTInitial(temperature);
        pbadMap[mid] = probability;
        cout << "Temperature: " << temperature << " pbad: " << probability << endl;
        if(probability > 0.995){
            binarySearchRightEnd = mid;
            mid = (binarySearchRightEnd + binarySearchLeftEnd) / 2;
        } else if(probability < 0.985){
            binarySearchLeftEnd = mid;
            mid = (binarySearchRightEnd + binarySearchLeftEnd) / 2;
        }
    }
    LinearRegression linearRegression;
    linearRegression.setup(pbadMap);
    tuple<int, double, double, int, double, double, double, double> regressionResult = linearRegression.run();
    double lowerEnd = get<2>(regressionResult);
    double upperEnd = get<5>(regressionResult);
    cout << "Left endpoint of linear regression " << pow(10, lowerEnd) << endl;
    cout << "Right endpoint of linear regression " << pow(10, upperEnd) << endl;
    double startingTemperature = pow(10,LOG10_HIGH_TEMP);
    for (auto const& keyValue : pbadMap)
    {
        if(keyValue.second >= 0.985 && keyValue.first >= upperEnd){
            startingTemperature = pow(10, keyValue.first);
            cout << "Initial temperature is " << startingTemperature << " expected pbad is " << keyValue.second << endl;
            break;
        }
    }
    double endingTemperature = pow(10,LOG10_LOW_TEMP);
    double distanceFromTarget = std::numeric_limits<double>::max();
    double endingPbad = 0.0;
    for (auto const& keyValue : pbadMap)
    {
    	if (distanceFromTarget > abs(FinalPBad - keyValue.second) && pow(10, keyValue.first) <= startingTemperature){
    		distanceFromTarget = abs(FinalPBad - keyValue.second);
    		endingTemperature = pow(10, keyValue.first);
		endingPbad = keyValue.second;
    	}
    }
    cout << "Final temperature is " << endingTemperature << " expected pbad is " << endingPbad << endl;
    TInitial = startingTemperature;
    TFinal = endingTemperature;

}

void SANA::searchTemperaturesByStatisticalTest() {
    const double NUM_SAMPLES_RANDOM = 100;
    const double HIGH_THRESHOLD_P = 0.999999;
    const double LOW_THRESHOLD_P = 0.99;

    cerr<<endl;
    //find the threshold score between random and not random temperature
    Timer T;
    T.start();
    cout << "Computing distribution of scores of random alignments ";
    vector<double> upperBoundKScores(NUM_SAMPLES_RANDOM);
    for (uint i = 0; i < NUM_SAMPLES_RANDOM; i++) {
        upperBoundKScores[i] = scoreRandom();
    }
    cout << "(" <<  T.elapsedString() << ")" << endl;
    NormalDistribution dist(upperBoundKScores);
    double highThresholdScore = dist.quantile(HIGH_THRESHOLD_P);
    double lowThresholdScore = dist.quantile(LOW_THRESHOLD_P);
    cout << "Mean: " << dist.getMean() << endl;
    cout << "sd: " << dist.getSD() << endl;
    cout << LOW_THRESHOLD_P << " of random runs have a score <= " << lowThresholdScore << endl;
    cout << HIGH_THRESHOLD_P << " of random runs have a score <= " << highThresholdScore << endl;

    double lowerBoundTInitial = 0;
    double upperBoundTInitial = 1;
    while (not isRandomTInitial(upperBoundTInitial, highThresholdScore, lowThresholdScore)) {
        cout << endl;
        upperBoundTInitial *= 2;
    }
    upperBoundTInitial *= 2;    // one more doubling just to be sure
    cout << endl;
    if (upperBoundTInitial > 1) lowerBoundTInitial = upperBoundTInitial/4;

    uint n1 = G1->getNumNodes();
    uint n2 = G2->getNumNodes();
    cout << "Iterations per run: " << 10000.+100.*n1+10.*n2+n1*n2*0.1 << endl;

    uint count = 0;
    T.start();
    while (fabs(lowerBoundTInitial - upperBoundTInitial)/lowerBoundTInitial > 0.05 and
            count <= 10) {
        //search in log space
        double lowerBoundTInitialLog = log2(lowerBoundTInitial+1);
        double upperBoundTInitialLog = log2(upperBoundTInitial+1);
        double midTInitialLog = (lowerBoundTInitialLog+upperBoundTInitialLog)/2.;
        double midTInitial = exp2(midTInitialLog)-1;

        //we prefer false negatives (random scores classified as non-random)
        //than false positives (non-random scores classified as random)
        cout << "Test " << count << " (" << T.elapsedString() << "): ";
        count++;
        if (isRandomTInitial(midTInitial, highThresholdScore, lowThresholdScore)) {
            upperBoundTInitial = midTInitial;
            cout << " (random behavior)";
        }
        else {
            lowerBoundTInitial = midTInitial;
            cout << " (NOT random behavior)";
        }
        cout << " New range: (" << lowerBoundTInitial << ", " << upperBoundTInitial << ")" << endl;
    }
    //return the top of the range
    cout << "Final range: (" << lowerBoundTInitial << ", " << upperBoundTInitial << ")" << endl;
    cout << "Final TInitial: " << upperBoundTInitial << endl;
    cout << "Final P(Bad): " << pForTInitial(upperBoundTInitial) << endl;

    TInitial = upperBoundTInitial;
    TFinal = lowerBoundTInitial;

    setAcceptableTInitial();
    setAcceptableTFinal();
}

void SANA::setAcceptableTInitial(){
    TInitial = findAcceptableTInitial(TInitial);
}

void SANA::setAcceptableTFinal(){
    TFinal = findAcceptableTFinal(TFinal);
}

void SANA::setAcceptableTFinalFromManualTInitial(){
    TFinal = findAcceptableTFinalFromManualTInitial(TInitial);
}

double SANA::findAcceptableTInitial(double temperature){
    double exponent = log10 (temperature);
    double initialTemperature = pow(10, exponent);
    double initialProbability = pForTInitial(initialTemperature);
    cout << "Searching for an acceptable initial temperature" << endl;
    do{
        exponent += 0.1;
        initialTemperature = pow(10, exponent);
        initialProbability = pForTInitial(initialTemperature);
        cout << "Temperature: " << initialTemperature << " PBad: " << initialProbability << endl;
    } while(initialProbability < 0.99);
    cout << "Resulting tInitial: " << initialTemperature << endl;
    cout << "Expected initial probability: " << initialProbability << endl;
    return initialTemperature;
}

double SANA::findAcceptableTFinalFromManualTInitial(double temperature){
    double exponent = log10 (temperature);
    double finalTemperature = pow(10, exponent);
    double finalProbability = pForTInitial(finalTemperature);
    cout << "Searching for an acceptable final temperature based on the initial temperature" << endl;
    do{
        exponent -= 0.2;
        finalTemperature = pow(10, exponent);
        finalProbability = pForTInitial(finalTemperature);
        cout << "Temperature: " << finalTemperature << " PBad: " << finalProbability << endl;
    } while(finalProbability > 0.00001);
    cout << "Resulting tFinal: " << finalTemperature << endl;
    cout << "Expected final probability: " << finalProbability << endl;
    return finalTemperature;
}

double SANA::findAcceptableTFinal(double temperature){
    double exponent = log10 (temperature);
    double finalTemperature = pow(10, exponent);
    double finalProbability = pForTInitial(finalTemperature);
    cout << "Searching for an acceptable final temperature" << endl;
    do{
        exponent -= 0.1;
        finalTemperature = pow(10, exponent);
        finalProbability = pForTInitial(finalTemperature);
        cout << "Temperature: " << finalTemperature << " PBad: " << finalProbability << endl;
    } while(finalProbability > 0.00001);
    cout << "Resulting tFinal: " << finalTemperature << endl;
    cout << "Expected final probability: " << finalProbability << endl;
    return finalTemperature;
}

double SANA::solveTDecay() {
    double tdecay = -log(TFinal * 1.0 * TInitialScaling/(TInitial)) / (1);
    cout << "tdecay: " << tdecay << endl;
    return tdecay;
}

void SANA::setTDecayAutomatically() {
    TDecay = solveTDecay();
}

//takes a random alignment, lets it run for a certain number
//of iterations (ITERATIONS) with fixed temperature TInitial
//and returns its score
double SANA::scoreForTInitial(double TInitial) {
    uint ITERATIONS = 10000.+100.*n1+10.*n2+n1*n2*0.1; //heuristic value
    ITERATIONS = getIterPerSecond();
    double oldIterationsPerStep = this->iterationsPerStep;
    double oldTInitial = this->TInitial;
    bool oldRestart = restart;

    this->iterationsPerStep = ITERATIONS;
    this->TInitial = TInitial;
    constantTemp = true;
    enableTrackProgress = false;
    restart = false;

    long long int iter = 0;
    // simpleRun(Alignment::random(n1, n2), 0.0, iter);
    simpleRun(getStartingAlignment(), 1.0, iter);
    this->iterationsPerStep = oldIterationsPerStep;
    this->TInitial = oldTInitial;
    constantTemp = false;
    enableTrackProgress = true;
    restart = oldRestart;

    return currentScore;
}

string SANA::getFolder(){
    //create (if neccessary) and return the path of the measure combinations respcetive cache folder
    stringstream ss;
    ss << "mkdir -p " << "cache-pbad" << "/" << MC->toString() << "/";
    system(ss.str().c_str());
    stringstream sf;
    sf << "cache-pbad" << "/" << MC->toString() << "/";
    return sf.str();
}

string SANA::mkdir(const std::string& file){
    //create (if neccessary) and return the path of a path folder in the cache
    stringstream ss;
    ss << "mkdir -p " << getFolder() << file << "/";
    system(ss.str().c_str());
    stringstream sf;
    sf << getFolder() << file << "/";
    return sf.str();
}

double SANA::pForTInitial(double TInitial) {

    // T = TInitial;
    // double pBad;
    // vector<double> EIncs = energyIncSample(T);
 //    cout << "Trying TInitial " << T;
 //    //uint nBad = 0;
 //    //for(uint i=0; i<EIncs.size();i++)
    // //nBad += (randomReal(gen) <= exp(EIncs[i]/T));
 //    pBad = exp(avgEnergyInc/T); // (double)nBad/(EIncs.size());
 //    cout << " p(Bad) = " << pBad << endl;
    // return pBad;

    //Establish the amount of iterations per second before getPforTInitial otherwise it will be calculated with iterationsPerStep = 100000
    getIterPerSecond();

    uint ITERATIONS = 10000.+100.*n1+10.*n2+n1*n2*0.1; //heuristic value
    ITERATIONS = 100000;
    double oldIterationsPerStep = this->iterationsPerStep;
    double oldTInitial = this->TInitial;
    bool oldRestart = restart;

    this->iterationsPerStep = ITERATIONS;
    this->TInitial = TInitial;
    constantTemp = true;
    enableTrackProgress = true;
    restart = false;

    long long int iter = 0;
    // simpleRun(Alignment::random(n1, n2), 0.0, iter);
    double result = getPforTInitial(getStartingAlignment(), 1.0, iter);
    this->iterationsPerStep = oldIterationsPerStep;
    this->TInitial = oldTInitial;
    constantTemp = false;
    enableTrackProgress = true;
    restart = oldRestart;

    return result;
}

double SANA::getPforTInitial(const Alignment& startA, double maxExecutionSeconds,
        long long int& iter) {
    double result = 0.0;
    initDataStructures(startA);

    setInterruptSignal();
    for (; ; iter++) {
        T = temperatureFunction(iter, TInitial, TDecay);
        if (interrupt) {
            return result;
        }
        if (iter%iterationsPerStep == 0) {
            result = trueAcceptingProbability();
            if ((iter != 0 and timer.elapsed() > maxExecutionSeconds and sampledProbability.size() > 0) or iter > 5E7) {
                if(sampledProbability.size() == 0){
                    return 1;
                }else{
                    return result;
                }
            }
        } //This is somewhat redundant with iter, but this is specifically for counting total iterations in the entire SANA object.  If you want this changed, post a comment on one of Dillon's commits and he'll make it less redundant but he needs here for now.
        SANAIteration();
    }
    return result; //dummy return to shut compiler warning
}

bool SANA::isRandomTInitial(double TInitial, double highThresholdScore, double lowThresholdScore) {
    const double NUM_SAMPLES = 5;

    double score = scoreForTInitial(TInitial);
    cout << "T_initial = " << TInitial << ", score = " << score;
    //quick filter all the scores that are obviously not random
    if (score > highThresholdScore) return false;
    if (score < lowThresholdScore) return true;
    //make sure that alignments that passed the first test are truly random
    //(among NUM_SAMPLES runs, at least one of them has a p-value smaller than LOW_THRESHOLD_P)
    for (uint i = 0; i < NUM_SAMPLES; i++) {
        if (scoreForTInitial(TInitial) <= lowThresholdScore) return true;
    }
    return false;
}

double SANA::scoreRandom() {
    return eval(Alignment::randomAlignmentWithLocking(G1,G2));
}

double SANA::searchSpaceSizeLog() {
    //the search space size is (n2 choose n1) * n1!
    //we use the stirling approximation
    if (n2 == n1) return n1*log(n1)-n1;
    return n2*log(n2)-(n2-n1)*log(n2-n1)-n1;
}

Alignment SANA::hillClimbingAlignment(Alignment startAlignment, long long int idleCountTarget){
    long long int iter = 0;
    uint idleCount = 0;
    T = 0;
    initDataStructures(startAlignment); //this is redundant, but it's not that big of a deal.  Resets true probability.
    cout << "Beginning Final Pure Hill Climbing Stage" << endl;
    while(idleCount < idleCountTarget){
        if (iter%iterationsPerStep == 0) {
            trackProgress(iter);
        }
        double oldScore = currentScore;
        SANAIteration();
        if(abs(oldScore-currentScore) < 0.00001){
            idleCount++;
        }else{
            idleCount = 0;
        }
        iter++;
    }
    trackProgress(iter);
    return *A;
}

Alignment SANA::hillClimbingAlignment(long long int idleCountTarget){
    return hillClimbingAlignment(getStartingAlignment(), idleCountTarget);
}

void SANA::hillClimbingIterations(long long int iterTarget) {
    Alignment startA = getStartingAlignment();
    long long int iter = 1;

    initDataStructures(startA);
    T = 0;
    for (; iter < iterTarget ; iter++) {
        if (iter%iterationsPerStep == 0) {
            trackProgress(iter);
        }
        SANAIteration();
    }
    trackProgress(iter);
}

double SANA::expectedNumAccEInc(double temp, const vector<double>& energyIncSample) {
    double res = 0;
    for (uint i = 0; i < energyIncSample.size(); i++) {
        res += exp(energyIncSample[i]/temp);
    }
    return res;
}

//returns a sample of energy increments, with size equal to the number of iterations per second
//after hill climbing
vector<double> SANA::energyIncSample(double temp) {

    getIterPerSecond(); //this runs HC, so it updates the values
    //of A and currentScore (besides iterPerSecond)

    double iter = iterPerSecond;
    //cout << "Hill climbing score: " << currentScore << endl;
    //generate a sample of energy increments, with size equal to the number of iterations per second
    vector<double> EIncs(0);
    T = temp;
    for (uint i = 0; i < iter; i++) {
        SANAIteration();
        if (energyInc < 0) {
            EIncs.push_back(energyInc);
        }
    }
    //avgEnergyInc = vectorMean(EIncs);
    return EIncs;
}

double SANA::simpleSearchTInitial() {
    T = .5e-6;
    double pBad;
    do {
        T *= 2;
        vector<double> EIncs = energyIncSample(T);
        cout << "Trying TInitial " << T;
        pBad = exp(avgEnergyInc/T); // (double)nBad/(EIncs.size());
        cout << " p(Bad) = " << pBad << endl;
    } while(pBad < 0.9999999); // How close to 1? I pulled this out of my ass.
    return T;
}

double SANA::searchTDecay(double TInitial, double minutes) {

    double iter_t = minutes*60*getIterPerSecond();

    //commented out this method because it was bugged.

    //new TDecay method uses upper and lower tbounds
    if(lowerTBound != 0){
        double tdecay = -log(lowerTBound * 1.0 * TInitialScaling/(upperTBound)) / (1);
        cout << "\ntdecay: " << tdecay << "\n";
        return tdecay;
    }

    //old TDecay method
    vector<double> EIncs = energyIncSample();
    cout << "Total of " << EIncs.size() << " energy increment samples averaging " << vectorMean(EIncs) << endl;

    //find the temperature epsilon such that the expected number of these energy samples accepted is 1
    //by bisection, since the expected number is monotically increasing in epsilon

    //upper bound and lower bound of x
    uint N = EIncs.size();
    double ESum = vectorSum(EIncs);
    double EMin = vectorMin(EIncs);
    double EMax = vectorMax(EIncs);
    double x_left = abs(EMax)/log(N);
    double x_right = min(abs(EMin)/log(N), abs(ESum)/(N*log(N)));
    cout << "Starting range: (" << x_left << ", " << x_right << ")" << endl;

    const uint NUM_ITER = 100;
    for (uint i = 0; i < NUM_ITER; i++) {
        double x_mid = (x_left + x_right)/2;
        double y = expectedNumAccEInc(x_mid, EIncs);
        if (y < 1) x_left = x_mid;
        else if (y > 1) x_right = x_mid;
        else break;
    }

    double epsilon = (x_left + x_right)/2;
    cout << "Final range: (" << x_left << ", " << x_right << ")" << endl;
    cout << "Final epsilon: " << epsilon << endl;


    double lambda = log((TInitial)/epsilon)/(iter_t);
    cout << "Final T_decay: " << lambda << endl;
    return lambda;
}

double SANA::searchTDecay(double TInitial, uint iterations) {

    vector<double> EIncs = energyIncSample();
    cout << "Total of " << EIncs.size() << " energy increment samples averaging " << vectorMean(EIncs) << endl;

    //find the temperature epsilon such that the expected number of thes e energy samples accepted is 1
    //by bisection, since the expected number is monotically increasing in epsilon

    //upper bound and lower bound of x
    uint N = EIncs.size();
    double ESum = vectorSum(EIncs);
    double EMin = vectorMin(EIncs);
    double EMax = vectorMax(EIncs);
    double x_left = abs(EMax)/log(N);
    double x_right = min(abs(EMin)/log(N), abs(ESum)/(N*log(N)));
    cout << "Starting range: (" << x_left << ", " << x_right << ")" << endl;

    const uint NUM_ITER = 100;
    for (uint i = 0; i < NUM_ITER; i++) {
        double x_mid = (x_left + x_right)/2;
        double y = expectedNumAccEInc(x_mid, EIncs);
        if (y < 1) x_left = x_mid;
        else if (y > 1) x_right = x_mid;
        else break;
    }

    double epsilon = (x_left + x_right)/2;
    cout << "Final range: (" << x_left << ", " << x_right << ")" << endl;
    cout << "Final epsilon: " << epsilon << endl;
    long long int iter_t = (long long int)(iterations)*100000000;

    double lambda = log((TInitial*TInitialScaling)/epsilon)/(iter_t);
    cout << "Final T_decay: " << lambda << endl;
    return lambda;
}

double SANA::getIterPerSecond() {
    if (not initializedIterPerSecond) {
        initIterPerSecond();
    }
    return iterPerSecond;
}

void SANA::initIterPerSecond() {
    Timer T;
    T.start();
    cout << "Determining iteration speed...." << endl;

    long long int iter = 1E6;

    hillClimbingIterations(iter - 1);
    /*if (iter == 500000) {
        throw runtime_error("hill climbing stagnated after 0 iterations");
    }*/
    double res = iter/timer.elapsed();
    cout << "SANA does " << to_string(res)
         << " iterations per second (took " << T.elapsedString()
         << " doing " << iter << " iterations)" << endl;

    initializedIterPerSecond = true;
    iterPerSecond = res;
    std::ostringstream ss;
    ss << "progress_" << std::fixed << std::setprecision(0) << minutes;
    ofstream header(mkdir(ss.str()) + G1->getName() + "_" + G2->getName() + "_" + std::to_string(0) + ".csv");
    header << "time,score,avgEnergyInc,T,realTemp,pbad,lower,higher,timer" << endl;
    header.close();
}

void SANA::setDynamicTDecay() {
    dynamic_tdecay = true;
}

#ifdef WEIGHTED
void SANA::prune(string& startAligName) {
    std::cerr << "Starting to prune using " <<  startAligName << std::endl;
    int n = G1->getNumNodes();
    vector<ushort> alignment;
    
    stringstream errorMsg;
    string format = startAligName.substr(startAligName.size()-6);
    assert(format == ".align"); // currently only edgelist format is supported


    
    Alignment align =  Alignment::loadEdgeList(G1, G2, startAligName);
#ifdef REINDEX
    if(G1->hasNodeTypes())
        align.reIndexBefore_Iterations(G1->getNodeTypes_ReIndexMap());
    else if (lockFileName != "")
        align.reIndexBefore_Iterations(G1->getLocking_ReIndexMap());

#endif        


    unordered_map<ushort, ushort> reIndexedMap;    
    // This is when no reIndexing is done, just to simplify the code
    for(int i=0;i<n;i++)
        reIndexedMap[i] = i;

#ifdef REINDEX
    if(G1->hasNodeTypes())
        reIndexedMap = G1->getNodeTypes_ReIndexMap();
    else if (lockFileName != "")
        reIndexedMap = G1->getLocking_ReIndexMap();
#endif
    
    alignment = align.getMapping();
            
    std::cerr << alignment.size() << " " << n << std::endl;
    if ((int)alignment.size() != n) {
        errorMsg << "Alignment size (" << alignment.size() << ") less than number of nodes (" << n <<")";
        throw runtime_error(errorMsg.str().c_str());
    }
    set<pair<int,int>> removedEdges;
    for (int i = 0; i < n; i++) {
        int g1_node1 = reIndexedMap[i];
        int shadow_node = alignment[g1_node1];
        int m = G1AdjLists[i].size();
        for (int j = 0; j < m; j++) {
            if (G1AdjLists[i][j] < i)
                continue;
            int g1_node2 = reIndexedMap[G1AdjLists[i][j]];
            int shadow_end = alignment[g1_node2];

            assert(G1AdjMatrix[g1_node1][g1_node2] == 0 || G2AdjMatrix[shadow_node][shadow_end] > 0);
            assert(G1AdjMatrix[g1_node2][g1_node1] ==0 || G2AdjMatrix[shadow_end][shadow_node] > 0);

            G2AdjMatrix[shadow_node][shadow_end] -= G1AdjMatrix[g1_node1][g1_node2];
            G2AdjMatrix[shadow_end][shadow_node] -= G1AdjMatrix[g1_node1][g1_node2];
            if (G2AdjMatrix[shadow_node][shadow_end] == 0) {
                    removedEdges.insert(pair<int,int>(shadow_node,shadow_end));
            }
        }
    }
    vector<vector<ushort> > t_edgeList;
    vector<vector<ushort> > G2EdgeList;
    G2->getEdgeList(G2EdgeList);
    for (auto c : G2EdgeList) {
        if (removedEdges.find(pair<int,int>(c[0],c[1])) != removedEdges.end() or
                removedEdges.find(pair<int,int>(c[1],c[0])) != removedEdges.end()) {
            continue;
        }
        t_edgeList.push_back(c);
    }
    G2->setAdjMatrix(G2AdjMatrix);
    G2->getAdjLists(G2AdjLists);
    G2->setEdgeList(t_edgeList);
}
#endif
