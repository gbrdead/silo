#ifndef __VOIDLAND_TURNING_GRILLE_HPP__
#define __VOIDLAND_TURNING_GRILLE_HPP__

#include "WordsTrie.hpp"
#include "MPMC_PortionQueue.hpp"
#include "Grille.hpp"

#include <string>
#include <cstdint>
#include <chrono>
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include "concurrentqueue/concurrentqueue.h"

using namespace org::voidland::concurrent;


namespace org::voidland::concurrent
{

extern bool VERBOSE;

}


namespace org::voidland::concurrent::turning_grille
{


class TurningGrilleCracker;


class TurningGrilleCrackerImplDetails
{
public:
	virtual ~TurningGrilleCrackerImplDetails();

	virtual void bruteForce(TurningGrilleCracker& cracker) = 0;

	virtual std::string milestone(TurningGrilleCracker& cracker, uint64_t grillesPerSecond);
	virtual std::string milestonesSummary(TurningGrilleCracker& cracker);
};


class TurningGrilleCracker
{
	friend class TurningGrilleCrackerProducerConsumer;
	friend class TurningGrilleCrackerSyncless;

private:
    inline static const std::string WORDS_FILE_PATH = "3000words.txt";
    inline static const unsigned MIN_DETECTED_WORD_COUNT = 17;  // Determined by gut feeling.

private:
    unsigned sideLength;
    uint64_t grilleCount;
    std::atomic<uint64_t> grilleCountSoFar;

    std::string cipherText;
    WordsTrie wordsTrie;

    std::mutex milestoneReportMutex;
    std::chrono::steady_clock::time_point start;

    uint64_t grilleCountAtMilestoneStart;
    std::chrono::steady_clock::time_point milestoneStart;
    uint64_t bestGrillesPerSecond;

    std::unique_ptr<TurningGrilleCrackerImplDetails> implDetails;

public:
    TurningGrilleCracker(const std::string& cipherText, std::unique_ptr<TurningGrilleCrackerImplDetails> implDetails);
    virtual ~TurningGrilleCracker();

    void bruteForce();

private:
    void applyGrille(const Grille& grill);
    void findWordsAndReport(const std::string& candidate);
};


class TurningGrilleCrackerProducerConsumer :
		public TurningGrilleCrackerImplDetails
{
private:
    unsigned initialConsumerCount;
    unsigned producerCount;
    std::unique_ptr<queue::MPMC_PortionQueue<Grille>> portionQueue;

    std::atomic<unsigned> consumerCount;
    moodycamel::ConcurrentQueue<std::thread> consumerThreads;

    std::atomic<int> shutdownNConsumers;	// This may get negative, so don't make it unsigned.
    int improving;
    bool addingThreads;
    uint64_t prevGrillesPerSecond;
    unsigned bestConsumerCount;
    uint64_t bestGrillesPerSecond;

public:
    TurningGrilleCrackerProducerConsumer(unsigned initialConsumerCount, unsigned producerCount, std::unique_ptr<queue::MPMC_PortionQueue<Grille>> portionQueue);

    void bruteForce(TurningGrilleCracker& cracker);
    std::string milestone(TurningGrilleCracker& cracker, uint64_t grillesPerSecond);
    std::string milestonesSummary(TurningGrilleCracker& cracker);

private:
    std::vector<std::thread> startProducerThreads(TurningGrilleCracker& cracker);
    void startInitialConsumerThreads(TurningGrilleCracker& cracker);
    std::thread startConsumerThread(TurningGrilleCracker& cracker);
};


class TurningGrilleCrackerSyncless :
		public TurningGrilleCrackerImplDetails
{
private:
	std::atomic<unsigned> workersCount;
	std::vector<std::pair<std::unique_ptr<std::atomic<uint64_t>>, uint64_t>> grilleIntervalsCompletion;

public:
    TurningGrilleCrackerSyncless();

    void bruteForce(TurningGrilleCracker& cracker);
    std::string milestone(TurningGrilleCracker& cracker, uint64_t grillesPerSecond);

private:
    std::vector<std::thread> startWorkerThreads(TurningGrilleCracker& cracker, unsigned workerCount);
};


}

#endif
