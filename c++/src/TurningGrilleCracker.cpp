#include "TurningGrilleCracker.hpp"

#include <stdexcept>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <optional>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>


namespace org::voidland::concurrent
{

bool VERBOSE = (std::getenv("VERBOSE") != nullptr) && boost::iequals("true", std::getenv("VERBOSE"));

}

namespace org::voidland::concurrent::turning_grille
{


TurningGrilleCracker::TurningGrilleCracker(const std::string& cipherText, std::unique_ptr<TurningGrilleCrackerImplDetails> implDetails) :
    cipherText(cipherText),
    wordsTrie(TurningGrilleCracker::WORDS_FILE_PATH),
    grilleCountAtMilestoneStart(0),
    bestGrillesPerSecond(0),
    grilleCountSoFar(0),
	implDetails(std::move(implDetails))
{
    boost::to_upper(this->cipherText);

    const static boost::regex nonLettersRe("[^A-Z]");
    if (boost::regex_match(this->cipherText, nonLettersRe))
    {
        throw std::invalid_argument("The ciphertext must contain only English letters.");
    }

    this->sideLength = (unsigned)std::sqrt((float)this->cipherText.length());
    if (this->sideLength == 0  ||  this->sideLength % 2 != 0  ||  this->sideLength * this->sideLength != this->cipherText.length())
    {
        throw std::invalid_argument("The ciphertext length must be a square of a positive odd number.");
    }

    this->grilleCount = 1;
    for (unsigned i = 0; i < (this->sideLength * this->sideLength) / 4; i++)
    {
        this->grilleCount *= 4;
    }
}

TurningGrilleCracker::~TurningGrilleCracker()
{
}

void TurningGrilleCracker::bruteForce()
{
    this->milestoneStart = this->start = std::chrono::steady_clock::now();

    this->implDetails->bruteForce(*this);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<int64_t, std::nano>::rep elapsedTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - this->start).count();
    uint64_t grillsPerSecond = this->grilleCount * std::nano::den / elapsedTimeNs;

    std::cerr << "Average speed: " << grillsPerSecond << " grilles/s; best speed: " << this->bestGrillesPerSecond << " grilles/s";
    std::string summary = this->implDetails->milestonesSummary(*this);
    if (summary.length() > 0)
    {
        std::cerr << "; " << summary;
    }
    std::cerr << std::endl;

    if (this->grilleCountSoFar != this->grilleCount)
    {
        throw std::runtime_error("Some grilles got lost.");
    }
}

void TurningGrilleCracker::applyGrille(const Grille& grille)
{
    std::string candidateBuf;
    candidateBuf.reserve(this->cipherText.length());

    for (unsigned rotation = 0; rotation < 4; rotation++)
    {
        for (unsigned y = 0; y < this->sideLength; y++)
        {
            for (unsigned x = 0; x < this->sideLength; x++)
            {
               if (grille.isHole(x, y, rotation))
               {
                   candidateBuf += this->cipherText[y * this->sideLength + x];
               }
            }
        }
    }

    this->findWordsAndReport(candidateBuf);
    std::reverse(candidateBuf.begin(), candidateBuf.end());
    this->findWordsAndReport(candidateBuf);

    uint64_t grilleCountSoFar = ++this->grilleCountSoFar;
    if (grilleCountSoFar % (this->grilleCount / 1000) == 0)   // A milestone every 0.1%.
    {
        std::chrono::steady_clock::time_point milestoneEnd = std::chrono::steady_clock::now();

        if (this->milestoneReportMutex.try_lock())
        {
        	std::lock_guard lock(this->milestoneReportMutex, std::adopt_lock);

            if (milestoneEnd > this->milestoneStart  &&  grilleCountSoFar > this->grilleCountAtMilestoneStart)
            {
                std::chrono::steady_clock::duration elapsedTime = milestoneEnd - this->milestoneStart;
                uint64_t grilleCountForMilestone = grilleCountSoFar - this->grilleCountAtMilestoneStart;

                std::chrono::duration<int64_t, std::nano>::rep elapsedTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsedTime).count();
                uint64_t grillesPerSecond = grilleCountForMilestone * std::nano::den / elapsedTimeNs;

                if (grillesPerSecond > this->bestGrillesPerSecond)
                {
                    this->bestGrillesPerSecond = grillesPerSecond;
                }

                std::string milestoneStatus = this->implDetails->milestone(*this, grillesPerSecond);

                if (VERBOSE)
                {
                    float done = grilleCountSoFar * 100.0 / this->grilleCount;

                    std::cerr << std::fixed << std::setprecision(1) << done << "% done; ";
                    std::cerr << "current speed: " << grillesPerSecond << " grilles/s; ";
                    std::cerr << "best speed so far: " << this->bestGrillesPerSecond << " grilles/s";
                    if (milestoneStatus.length() > 0)
                    {
                        std::cerr << "; " << milestoneStatus;
                    }
                    std::cerr << std::endl;
                }

                this->milestoneStart = milestoneEnd;
                this->grilleCountAtMilestoneStart = grilleCountSoFar;
            }
        }
    }
}

void TurningGrilleCracker::findWordsAndReport(const std::string& candidate)
{
    unsigned wordsFound = this->wordsTrie.countWords(candidate);
    if (wordsFound >= TurningGrilleCracker::MIN_DETECTED_WORD_COUNT)
    {
        if (VERBOSE)
        {
            std::cout << (std::to_string(wordsFound) + ": " + candidate + '\n');
        }
    }
}


TurningGrilleCrackerProducerConsumer::TurningGrilleCrackerProducerConsumer(unsigned initialConsumerCount, unsigned producerCount, std::unique_ptr<queue::MPMC_PortionQueue<Grille>> portionQueue) :
    initialConsumerCount(initialConsumerCount),
    producerCount(producerCount),
    consumerCount(0),
    portionQueue(std::move(portionQueue)),
	shutdownNConsumers(0),
    addingThreads(true),
    prevGrillesPerSecond(0),
    bestGrillesPerSecond(prevGrillesPerSecond),
    bestConsumerCount(0),
    improving(0)
{
}

void TurningGrilleCrackerProducerConsumer::bruteForce(TurningGrilleCracker& cracker)
{
	std::vector<std::thread> producerThreads = this->startProducerThreads(cracker);
    this->startInitialConsumerThreads(cracker);

    for (std::thread& producerThread : producerThreads)
    {
        producerThread.join();
    }

    this->portionQueue->ensureAllPortionsAreRetrieved();
    while (cracker.grilleCountSoFar < cracker.grilleCount);   // Ensures all work is done and no more consumers will be started or stopped.
    this->portionQueue->stopConsumers(this->consumerCount);

    std::thread consumerThread;
    while (this->consumerThreads.try_dequeue(consumerThread))
    {
        consumerThread.join();
    }
}

std::string TurningGrilleCrackerProducerConsumer::milestone(TurningGrilleCracker& cracker, uint64_t grillesPerSecond)
{
    // Locking a mutex and notifying a condition variable are time-expensive but the thread is blocked for most of this time.
    // In order to utilize the CPUs fully we need more consumers than the count of CPUs.
    // Here we continuously try to find the best consumer count for the current conditions.

    std::size_t queueSize = this->portionQueue->getSize();

    if (grillesPerSecond > this->bestGrillesPerSecond)
    {
        this->bestGrillesPerSecond = grillesPerSecond;
        this->bestConsumerCount = this->consumerCount;
    }

    if (cracker.grilleCountSoFar < cracker.grilleCount)
    {
        if (grillesPerSecond < this->prevGrillesPerSecond)
        {
            this->improving--;
        }
        else if (grillesPerSecond > this->prevGrillesPerSecond)
        {
            this->improving++;
        }

        if (this->improving >= 1 || this->improving <= -2)
        {
            if (this->improving < 0)
            {
                this->addingThreads = !this->addingThreads;
            }
            this->improving = 0;

            if (this->addingThreads)
            {
                this->consumerThreads.enqueue(this->startConsumerThread(cracker));
            }
            else
            {
                this->shutdownNConsumers++;
            }
        }

        this->prevGrillesPerSecond = grillesPerSecond;
    }

    if (VERBOSE)
    {
        std::ostringstream s;
        s << "consumer threads: " << this->consumerCount << "; queue size: " << queueSize << " / " << this->portionQueue->getMaxSize();
        return s.str();
    }
    return "";
}

std::string TurningGrilleCrackerProducerConsumer::milestonesSummary(TurningGrilleCracker& cracker)
{
    std::ostringstream s;
    s << "best consumer threads: " << this->bestConsumerCount;
    return s.str();
}

std::vector<std::thread> TurningGrilleCrackerProducerConsumer::startProducerThreads(TurningGrilleCracker& cracker)
{
	std::vector<std::thread> producerThreads;
	producerThreads.reserve(this->producerCount);

    uint64_t nextIntervalBegin = 0;
    uint64_t intervalLength = std::lround((double)cracker.grilleCount / this->producerCount);
    for (unsigned i = 0; i < this->producerCount; i++)
    {
    	std::unique_ptr<GrilleInterval> grilleInterval = std::make_unique<GrilleInterval>(cracker.sideLength / 2, nextIntervalBegin,
            (i < this->producerCount - 1) ? (nextIntervalBegin + intervalLength) : cracker.grilleCount);

        producerThreads.push_back(std::thread
            {
                [this, grilleInterval = std::move(grilleInterval)]
                {
                    while (true)
					{
                    	std::optional<Grille> grille = grilleInterval->cloneNext();
                    	if (!grille)
                    	{
                    		break;
                    	}
                    	this->portionQueue->addPortion(std::move(*grille));
					}
                }
            });

        nextIntervalBegin += intervalLength;
    }

    return producerThreads;
}

void TurningGrilleCrackerProducerConsumer::startInitialConsumerThreads(TurningGrilleCracker& cracker)
{
    for (unsigned i = 0; i < this->initialConsumerCount; i++)
    {
        this->consumerThreads.enqueue(this->startConsumerThread(cracker));
    }
}

std::thread TurningGrilleCrackerProducerConsumer::startConsumerThread(TurningGrilleCracker& cracker)
{
    this->consumerCount++;
    return std::thread
        {
            [this, &cracker]
            {
                while (true)
                {
                	std::optional<Grille> grille = this->portionQueue->retrievePortion();
                	if (!grille.has_value())
                	{
                		this->consumerCount--;
                		break;
                	}

                    cracker.applyGrille(*grille);

                    if (this->shutdownNConsumers > 0)
                    {
						if (this->shutdownNConsumers.fetch_sub(1) > 0)
						{
							if (this->consumerCount.fetch_sub(1) > 1)		// There should be at least one consumer running.
							{
								break;
							}
							else
							{
								this->consumerCount++;
							}
						}
						else
						{
							this->shutdownNConsumers++;
						}
                    }
                }
            }
        };
}


TurningGrilleCrackerSyncless::TurningGrilleCrackerSyncless() :
	workersCount(0)
{
}

void TurningGrilleCrackerSyncless::bruteForce(TurningGrilleCracker& cracker)
{
    unsigned cpuCount = std::thread::hardware_concurrency();

    std::vector<std::thread> workerThreads = this->startWorkerThreads(cracker, cpuCount);

    for (std::thread& workerThread : workerThreads)
    {
        workerThread.join();
    }
}

std::vector<std::thread> TurningGrilleCrackerSyncless::startWorkerThreads(TurningGrilleCracker& cracker, unsigned workerCount)
{
	std::vector<std::thread> workerThreads;
	workerThreads.reserve(workerCount);

	this->grilleIntervalsCompletion.reserve(workerCount);

    uint64_t nextIntervalBegin = 0;
    uint64_t intervalLength = std::lround((double)cracker.grilleCount / workerCount);
    for (unsigned i = 0; i < workerCount; i++)
    {
    	this->workersCount++;

    	uint64_t nextIntervalEnd = (i < workerCount - 1) ? (nextIntervalBegin + intervalLength) : cracker.grilleCount;
    	std::unique_ptr<GrilleInterval> grilleInterval = std::make_unique<GrilleInterval>(cracker.sideLength / 2, nextIntervalBegin, nextIntervalEnd);

    	this->grilleIntervalsCompletion.emplace_back(std::make_pair<std::unique_ptr<std::atomic<uint64_t>>, uint64_t>(
				std::make_unique<std::atomic<uint64_t>>(0),
				(nextIntervalEnd - nextIntervalBegin)));
    	std::atomic<uint64_t> *processedGrillsCount = this->grilleIntervalsCompletion.back().first.get();

        workerThreads.push_back(std::thread
            {
                [this, &cracker, grilleInterval = std::move(grilleInterval), processedGrillsCount]
                {
                	while (true)
                	{
                		const Grille* grille = grilleInterval->getNext();
                		if (grille == nullptr)
						{
                			break;
						}
                        cracker.applyGrille(*grille);
                        (*processedGrillsCount)++;
                	}
                    this->workersCount--;
                }
            });

        nextIntervalBegin += intervalLength;
    }

    return workerThreads;
}

std::string TurningGrilleCrackerSyncless::milestone(TurningGrilleCracker& cracker, uint64_t grillesPerSecond)
{
    if (VERBOSE)
    {
        std::ostringstream s;
        s << "worker threads: " << this->workersCount << "; completion per thread: ";

        bool first = true;
        for (const std::pair<std::unique_ptr<std::atomic<uint64_t>>, uint64_t>& intervalCompletion : this->grilleIntervalsCompletion)
        {
            if (!first)
            {
            	s << "/";
            }
            else
            {
                first = false;
            }

            float completion = (*intervalCompletion.first) * 100.0f / intervalCompletion.second;
            s << std::fixed << std::setprecision(1) << completion;
        }
        s << "% done";

        return s.str();
    }
    return "";
}


TurningGrilleCrackerImplDetails::~TurningGrilleCrackerImplDetails()
{
}

std::string TurningGrilleCrackerImplDetails::milestone(TurningGrilleCracker& cracker, uint64_t grillesPerSecond)
{
	return std::string();
}

std::string TurningGrilleCrackerImplDetails::milestonesSummary(TurningGrilleCracker& cracker)
{
	return std::string();
}


}
