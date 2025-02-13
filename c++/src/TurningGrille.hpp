#ifndef __VOIDLAND_TURNING_GRILL_HPP__
#define __VOIDLAND_TURNING_GRILL_HPP__

#include "WordsTrie.hpp"
#include "MPMC_PortionQueue.hpp"

#include <string>
#include <cstdint>
#include <chrono>
#include <list>
#include <thread>
#include <vector>
#include <memory>
#include "concurrentqueue/concurrentqueue.h"

using namespace org::voidland::concurrent;


namespace org::voidland::concurrent
{

extern bool VERBOSE;

}


namespace org::voidland::concurrent::turning_grille
{


class Grille
{
private:
    unsigned halfSideLength;
    std::vector<uint8_t> holes;

public:
    Grille();
    Grille(unsigned halfSideLength, uint64_t ordinal);
    Grille(unsigned halfSideLength, std::vector<uint8_t>&& holes);
    Grille(const Grille& other);
    Grille(Grille&& other);

    Grille& operator=(const Grille& other);
    Grille& operator=(Grille&& other);

    Grille& operator++();

    bool isHole(unsigned x, unsigned y, unsigned rotation) const;
};


class GrilleInterval
{
private:
    Grille next;
    bool preincremented;
    uint64_t begin;
    uint64_t nextOrdinal;
    uint64_t end;

public:
    GrilleInterval(unsigned halfSideLength, uint64_t begin, uint64_t end);

    std::optional<Grille> cloneNext();
    const Grille* getNext();

    float calculateCompletion() const;
};


class TurningGrilleCracker
{
private:
    inline static const std::string WORDS_FILE_PATH = "3000words.txt";
    inline static const unsigned MIN_DETECTED_WORD_COUNT = 17;  // Determined by gut feeling.

protected:
    unsigned sideLength;
    uint64_t grilleCount;
    std::atomic<uint64_t> grilleCountSoFar;

private:
    std::string cipherText;
    WordsTrie wordsTrie;

    std::mutex milestoneReportMutex;
    std::chrono::high_resolution_clock::time_point start;

    uint64_t grilleCountAtMilestoneStart;
    std::chrono::high_resolution_clock::time_point milestoneStart;
    uint64_t bestGrillesPerSecond;

public:
    TurningGrilleCracker(const std::string& cipherText);
    virtual ~TurningGrilleCracker();

    void bruteForce();

protected:
    virtual void doBruteForce() = 0;
    virtual std::string milestone(uint64_t grillesPerSecond);
    virtual std::string milestonesSummary();

    void applyGrille(const Grille& grill);

private:
    void findWordsAndReport(const std::string& candidate);
};


class TurningGrilleCrackerProducerConsumer :
    public TurningGrilleCracker
{
private:
    unsigned initialConsumerCount;
    unsigned producerCount;
    std::unique_ptr<queue::MPMC_PortionQueue<Grille>> portionQueue;

    std::atomic<unsigned> consumerCount;
    moodycamel::ConcurrentQueue<std::thread> consumerThreads;
    std::list<std::thread> producerThreads;

    std::atomic<bool> shutdownOneConsumer;
    int improving;
    bool addingThreads;
    uint64_t prevGrillesPerSecond;
    unsigned bestConsumerCount;
    uint64_t bestGrillesPerSecond;

public:
    TurningGrilleCrackerProducerConsumer(const std::string& cipherText, unsigned consumerCount, unsigned producerCount, std::unique_ptr<queue::MPMC_PortionQueue<Grille>> portionQueue);

protected:
    void doBruteForce();
    std::string milestone(uint64_t grillesPerSecond);
    std::string milestonesSummary();

private:
    void startProducerThreads();
    void startInitialConsumerThreads();
    std::thread startConsumerThread();
};


class TurningGrilleCrackerSyncless :
    public TurningGrilleCracker
{
private:
	std::atomic<unsigned> workersCount;
	std::list<std::thread> workerThreads;
	std::list<GrilleInterval> grilleIntervals;

public:
    TurningGrilleCrackerSyncless(const std::string& cipherText);

protected:
    void doBruteForce();
    std::string milestone(uint64_t grillesPerSecond);

private:
    void startWorkerThreads(unsigned workerCount);
};

}

#endif
