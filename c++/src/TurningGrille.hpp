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
#include "concurrentqueue/concurrentqueue.h"


namespace org::voidland::concurrent
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

    uint64_t getOrdinal() const;
};


class GrilleInterval
{
private:
    Grille next;
    bool preincremented;
    uint64_t nextOrdinal;
    uint64_t end;

public:
    GrilleInterval(unsigned halfSideLength, uint64_t begin, uint64_t end);

    std::optional<Grille> cloneNext();
    const Grille* getNext();
};


class TurningGrilleCracker
{
private:
    inline static const std::string WORDS_FILE_PATH = "The_Oxford_3000.txt";
    inline static const unsigned MIN_DETECTED_WORD_COUNT = 16;  // Determined by gut feeling.

protected:
    unsigned sideLength;
    uint64_t grilleCount;
    std::atomic<uint64_t> grilleCountSoFar;

private:
    std::string cipherText;
    WordsTrie wordsTrie;

    std::atomic<bool> milestoneReportInProgress;
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
    virtual std::string milestone(std::chrono::high_resolution_clock::duration elapsedTime);
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
    MPMC_PortionQueue<Grille>& portionQueue;

    std::atomic<unsigned> consumerCount;
    moodycamel::ConcurrentQueue<std::thread> consumerThreads;
    std::list<std::thread> producerThreads;

    std::atomic<bool> shutdownOneConsumer;
    int improving;
    bool addingThreads;
    std::chrono::high_resolution_clock::duration prevElapsedTime;
    unsigned bestConsumerCount;
    std::chrono::high_resolution_clock::duration bestElapsedTime;

public:
    TurningGrilleCrackerProducerConsumer(const std::string& cipherText, unsigned consumerCount, unsigned producerCount, MPMC_PortionQueue<Grille>& portionQueue);

protected:
    void doBruteForce();
    std::string milestone(std::chrono::high_resolution_clock::duration elapsedTime);
    std::string milestonesSummary();

private:
    void startProducerThreads();
    void startInitialConsumerThreads();
    std::thread startConsumerThread();
};


class TurningGrilleCrackerWithPerfectParallelism :
    public TurningGrilleCracker
{
public:
    TurningGrilleCrackerWithPerfectParallelism(const std::string& cipherText);

protected:
    void doBruteForce();

private:
    std::list<std::thread> startWorkerThreads(unsigned workerCount);
};

}

#endif
