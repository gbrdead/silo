#ifndef __VOIDLAND_TURNING_GRILLE_HPP__
#define __VOIDLAND_TURNING_GRILLE_HPP__

#include "WordsTrie.hpp"
#include <voidland/blown_queue/MPMC_PortionQueue.hpp>
#include "Grille.hpp"

#include <string>
#include <cstdint>
#include <chrono>
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include <optional>
#include <set>
#include <boost/algorithm/string/regex.hpp>

#include <voidland/blown_queue/blown_queue_config.h>
#ifdef HAVE_MOODYCAMEL_CONCURRENT_QUEUE
#	ifdef HAVE_MOODYCAMEL_IN_INCLUDE_PATH
#		include <concurrentqueue/moodycamel/concurrentqueue.h>
#	else
#		include <concurrentqueue/concurrentqueue.h>
#	endif
#endif


using namespace voidland::concurrent;


namespace voidland::concurrent
{

extern bool VERBOSE;

}


namespace voidland::concurrent::turning_grille
{


extern boost::regex NOT_CAPITAL_ENGLISH_LETTERS_RE;


class TurningGrilleCracker;


class TurningGrilleCrackerImplDetails
{
public:
	virtual ~TurningGrilleCrackerImplDetails();

	virtual void bruteForce(TurningGrilleCracker& cracker) = 0;

	virtual void tryMilestone(TurningGrilleCracker& cracker, const std::chrono::steady_clock::time_point& milestoneEnd, uint64_t grilleCountSoFar) = 0;
	virtual std::string milestonesSummary(TurningGrilleCracker& cracker);
};


class TurningGrilleCracker
{
	friend class TurningGrilleCrackerProducerConsumer;
	friend class TurningGrilleCrackerSyncless;
	friend class TurningGrilleCrackerSerial;

private:
    inline static const std::string WORDS_FILE_PATH = "3000words.txt";
    inline static const unsigned MIN_DETECTED_WORD_COUNT = 17;  // Determined by gut feeling.

private:
    unsigned sideLength;
    uint64_t grilleCount;
    std::atomic<uint64_t> grilleCountSoFar;

    std::string cipherText;
    WordsTrie wordsTrie;
    std::mutex candidatesMutex;
    std::set<std::string> candidates;

    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point milestoneStart;
    uint64_t grilleCountAtMilestoneStart;
    uint64_t bestGrillesPerSecond;

    std::unique_ptr<TurningGrilleCrackerImplDetails> implDetails;

public:
    TurningGrilleCracker(const std::string& cipherText, std::unique_ptr<TurningGrilleCrackerImplDetails> implDetails);
    virtual ~TurningGrilleCracker();

    std::set<std::string> bruteForce();

private:
    uint64_t applyGrille(const Grille& grill);
    void findWordsAndReport(const std::string& candidate);
    void registerOneAppliedGrill(uint64_t grilleCountSoFar);
    std::optional<uint64_t> milestone(const std::chrono::steady_clock::time_point& milestoneEnd, uint64_t grilleCountSoFar, const std::string& milestoneDetailsStatus);
};


class TurningGrilleCrackerProducerConsumer :
		public TurningGrilleCrackerImplDetails
{
private:
    unsigned initialConsumerCount;
    unsigned producerCount;
    std::unique_ptr<queue::MPMC_PortionQueue<Grille>> portionQueue;

    std::atomic<int> consumerCount;			// This may get negative for a short while, so don't make it unsigned.
    moodycamel::ConcurrentQueue<std::thread> consumerThreads;
    std::atomic<int> shutdownNConsumers;	// This may get negative for a short while, so don't make it unsigned.

    std::mutex milestoneStateMutex;
    int improving;
    bool addingThreads;
    uint64_t prevGrillesPerSecond;
    unsigned bestConsumerCount;
    uint64_t bestGrillesPerSecond;

public:
    TurningGrilleCrackerProducerConsumer(unsigned initialConsumerCount, unsigned producerCount, std::unique_ptr<queue::MPMC_PortionQueue<Grille>> portionQueue);

    void bruteForce(TurningGrilleCracker& cracker);
	void tryMilestone(TurningGrilleCracker& cracker, const std::chrono::steady_clock::time_point& milestoneEnd, uint64_t grilleCountSoFar);
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

	std::mutex milestoneStateMutex;
	std::vector<std::pair<std::unique_ptr<std::atomic<uint64_t>>, uint64_t>> grilleIntervalsCompletion;

public:
    TurningGrilleCrackerSyncless();

    void bruteForce(TurningGrilleCracker& cracker);
	void tryMilestone(TurningGrilleCracker& cracker, const std::chrono::steady_clock::time_point& milestoneEnd, uint64_t grilleCountSoFar);

private:
    std::vector<std::thread> startWorkerThreads(TurningGrilleCracker& cracker, unsigned workerCount);
};


class TurningGrilleCrackerSerial :
		public TurningGrilleCrackerImplDetails
{
public:
    TurningGrilleCrackerSerial();

    void bruteForce(TurningGrilleCracker& cracker);
	void tryMilestone(TurningGrilleCracker& cracker, const std::chrono::steady_clock::time_point& milestoneEnd, uint64_t grilleCountSoFar);
};


}

#endif
