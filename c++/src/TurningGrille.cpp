#include "TurningGrille.hpp"

#include <stdexcept>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <optional>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>


namespace org::voidland::concurrent
{

bool VERBOSE = (std::getenv("VERBOSE") != nullptr) ? boost::iequals("true", std::getenv("VERBOSE")) : false;



Grille::Grille() :
    halfSideLength(0)
{
}

Grille::Grille(unsigned halfSideLength, uint64_t ordinal) :
    halfSideLength(halfSideLength),
    holes(halfSideLength * halfSideLength, 0)
{
    for (std::vector<uint8_t>::iterator i = this->holes.begin(); (ordinal != 0) && (i != this->holes.end()); i++)
    {
        *i = (ordinal & 0b11);
        ordinal >>= 2;
    }
}

Grille::Grille(const Grille& other) :
    halfSideLength(other.halfSideLength),
    holes(other.holes)
{
}

Grille::Grille(Grille&& other) :
    halfSideLength(other.halfSideLength),
    holes(std::move(other.holes))
{
    other.halfSideLength = 0;
}

Grille::Grille(unsigned halfSideLength, std::vector<uint8_t>&& holes) :
    halfSideLength(halfSideLength),
    holes(std::move(holes))
{
}

Grille& Grille::operator=(const Grille& other)
{
    this->halfSideLength = other.halfSideLength;
    this->holes = other.holes;
    return *this;
}

Grille& Grille::operator=(Grille&& other)
{
    this->halfSideLength = other.halfSideLength;
    other.halfSideLength = 0;
    this->holes = std::move(other.holes);
    return *this;
}

Grille& Grille::operator++()
{
    for (unsigned i = 0; i < this->holes.size(); i++)
    {
        if (this->holes[i] < 3)
        {
            this->holes[i]++;
            break;
        }
        this->holes[i] = 0;
    }

    return *this;
}

bool Grille::isHole(unsigned x, unsigned y, unsigned rotation) const
{
    unsigned sideLength = this->halfSideLength * 2;

    int origX, origY;
    switch (rotation)
    {
        case 0:
        {
            origX = x;
            origY = y;
            break;
        }
        case 1:
        {
            origX = y;
            origY = sideLength - 1 - x;
            break;
        }
        case 2:
        {
            origX = sideLength - 1 - x;
            origY = sideLength - 1 - y;
            break;
        }
        case 3:
        {
            origX = sideLength - 1 - y;
            origY = x;
            break;
        }
        default:
        {
            throw std::invalid_argument("");
        }
    }

    uint8_t quadrant;
    unsigned holeX, holeY;
    if (origX < this->halfSideLength)
    {
        if (origY < this->halfSideLength)
        {
            quadrant = 0;
            holeX = origX;
            holeY = origY;
        }
        else
        {
            quadrant = 3;
            holeX = sideLength - 1 - origY;
            holeY = origX;
        }
    }
    else
    {
        if (origY < this->halfSideLength)
        {
            quadrant = 1;
            holeX = origY;
            holeY = sideLength - 1 - origX;
        }
        else
        {
            quadrant = 2;
            holeX = sideLength - 1 - origX;
            holeY = sideLength - 1 - origY;
        }
    }

    return this->holes[holeX * this->halfSideLength + holeY] == quadrant;
}

uint64_t Grille::getOrdinal() const
{
    uint64_t ordinal = 0;
    for (unsigned i = 0; i < this->halfSideLength * this->halfSideLength; i++)
    {
        ordinal |= (this->holes[i] << i*2);
    }
    return ordinal;
}


GrilleInterval::GrilleInterval(unsigned halfSideLength, uint64_t begin, uint64_t end) :
    next(halfSideLength, begin),
    preincremented(true),
    nextOrdinal(begin),
    end(end)
{
}

std::optional<Grille> GrilleInterval::cloneNext()
{
    if (this->nextOrdinal >= this->end)
    {
        return std::nullopt;
    }

    if (!this->preincremented)
    {
        ++this->next;
    }

    Grille current(this->next);

    ++this->next;
    this->preincremented = true;

    this->nextOrdinal++;
    return current;
}

const Grille* GrilleInterval::getNext()
{
    if (this->nextOrdinal >= this->end)
    {
        return nullptr;
    }

    if (!this->preincremented)
    {
        ++this->next;
    }
    this->preincremented = false;

    this->nextOrdinal++;
    return &this->next;
}


TurningGrilleCracker::TurningGrilleCracker(const std::string& cipherText) :
    cipherText(cipherText),
    wordsTrie(TurningGrilleCracker::WORDS_FILE_PATH),
    milestoneReportInProgress(false),
    grilleCountAtMilestoneStart(0),
    bestGrillesPerSecond(0),
    grilleCountSoFar(0)
{
    boost::to_upper(this->cipherText);

    const static boost::regex nonLettersRe("[^A-Z]");
    if (boost::regex_match(this->cipherText, nonLettersRe))
    {
        throw std::invalid_argument("The ciphertext must contain only English letters.");
    }

    this->sideLength = (unsigned)std::sqrt((float)this->cipherText.length());
    if (this->sideLength % 2 != 0  ||  this->sideLength * this->sideLength != this->cipherText.length())
    {
        throw std::invalid_argument("The ciphertext length must be a square of an odd number.");
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
    this->milestoneStart = this->start = std::chrono::high_resolution_clock::now();
    this->doBruteForce();
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<int64_t, std::nano>::rep elapsedTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - this->start).count();
    uint64_t grillsPerSecond = this->grilleCount * std::nano::den / elapsedTimeNs;

    std::cerr << "Average speed: " << grillsPerSecond << " grilles/s; best speed: " << this->bestGrillesPerSecond << " grilles/s";
    std::string summary = this->milestonesSummary();
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

std::string TurningGrilleCracker::milestone(uint64_t grillesPerSecond)
{
    return "";
}

std::string TurningGrilleCracker::milestonesSummary()
{
    return "";
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
        std::chrono::high_resolution_clock::time_point milestoneEnd = std::chrono::high_resolution_clock::now();

        bool expectedMilestoneInProgress = false;
        if (this->milestoneReportInProgress.compare_exchange_strong(expectedMilestoneInProgress, true))
        {
            std::chrono::high_resolution_clock::time_point milestoneStart = this->milestoneStart;
            uint64_t grilleCountAtMilestoneStart = this->grilleCountAtMilestoneStart;

            if (milestoneEnd > milestoneStart  &&  grilleCountSoFar > grilleCountAtMilestoneStart)
            {
                std::chrono::high_resolution_clock::duration elapsedTime = milestoneEnd - milestoneStart;
                uint64_t grilleCountForMilestone = grilleCountSoFar - grilleCountAtMilestoneStart;

                std::chrono::duration<int64_t, std::nano>::rep elapsedTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsedTime).count();
                uint64_t grillesPerSecond = grilleCountForMilestone * std::nano::den / elapsedTimeNs;

                if (grillesPerSecond > this->bestGrillesPerSecond)
                {
                    this->bestGrillesPerSecond = grillesPerSecond;
                }

                std::string milestoneStatus = this->milestone(grillesPerSecond);

                if (org::voidland::concurrent::VERBOSE)
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

            this->milestoneReportInProgress = false;
        }
    }
}

void TurningGrilleCracker::findWordsAndReport(const std::string& candidate)
{
    unsigned wordsFound = this->wordsTrie.countWords(candidate);
    if (wordsFound >= TurningGrilleCracker::MIN_DETECTED_WORD_COUNT)
    {
        if (org::voidland::concurrent::VERBOSE)
        {
            std::cout << (std::to_string(wordsFound) + ": " + candidate + '\n');
        }
    }
}



TurningGrilleCrackerProducerConsumer::TurningGrilleCrackerProducerConsumer(const std::string& cipherText, unsigned initialConsumerCount, unsigned producerCount, MPMC_PortionQueue<Grille>& portionQueue) :
    TurningGrilleCracker(cipherText),
    initialConsumerCount(initialConsumerCount),
    producerCount(producerCount),
    consumerCount(0),
    portionQueue(portionQueue),
    shutdownOneConsumer(false),
    addingThreads(true),
    prevGrillesPerSecond(0),
    bestGrillesPerSecond(prevGrillesPerSecond),
    bestConsumerCount(0),
    improving(0)
{
}

void TurningGrilleCrackerProducerConsumer::doBruteForce()
{
    this->startProducerThreads();
    this->startInitialConsumerThreads();

    for (std::thread& producerThread : this->producerThreads)
    {
        producerThread.join();
    }

    this->portionQueue.ensureAllPortionsAreRetrieved();
    while (this->grilleCountSoFar < this->grilleCount);   // Ensures all work is done and no more consumers will be started or stopped.
    this->portionQueue.stopConsumers(this->consumerCount);

    std::thread consumerThread;
    while (this->consumerThreads.try_dequeue(consumerThread))
    {
        consumerThread.join();
    }
}

std::string TurningGrilleCrackerProducerConsumer::milestone(uint64_t grillesPerSecond)
{
    // Locking a mutex and notifying a condition variable are time-expensive but the thread is blocked for most of this time.
    // In order to utilize the CPUs fully we need more consumers than the count of CPUs.
    // Here we continuously try to find the best consumer count for the current conditions.

    std::size_t queueSize = this->portionQueue.getSize();

    if (grillesPerSecond > this->bestGrillesPerSecond)
    {
        this->bestGrillesPerSecond = grillesPerSecond;
        this->bestConsumerCount = this->consumerCount;
    }

    if (this->grilleCountSoFar < this->grilleCount)
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
                this->consumerThreads.enqueue(this->startConsumerThread());
            }
            else
            {
                this->shutdownOneConsumer = true;
            }
        }

        this->prevGrillesPerSecond = grillesPerSecond;
    }

    if (org::voidland::concurrent::VERBOSE)
    {
        std::ostringstream s;
        s << "consumer threads: " << this->consumerCount << "; queue size: " << queueSize << " / " << this->portionQueue.getMaxSize();
        return s.str();
    }
    return "";
}

std::string TurningGrilleCrackerProducerConsumer::milestonesSummary()
{
    std::ostringstream s;
    s << "best consumer threads: " << this->bestConsumerCount;
    return s.str();
}

void TurningGrilleCrackerProducerConsumer::startProducerThreads()
{
    uint64_t nextIntervalBegin = 0;
    uint64_t intervalLength = std::llround((long double)this->grilleCount / this->producerCount);
    for (unsigned i = 0; i < this->producerCount; i++)
    {
        this->producerThreads.push_back(std::thread
            {
                [this, nextIntervalBegin, i, intervalLength]
                {
                    GrilleInterval grilleInterval(this->sideLength / 2, nextIntervalBegin,
                        (i < this->producerCount - 1) ? (nextIntervalBegin + intervalLength) : this->grilleCount);

                    std::optional<Grille> grill;
                    while ((grill = grilleInterval.cloneNext()))
                    {
                        this->portionQueue.addPortion(std::move(*grill));
                    }
                }
            });

        nextIntervalBegin += intervalLength;
    }
}

void TurningGrilleCrackerProducerConsumer::startInitialConsumerThreads()
{
    for (unsigned i = 0; i < this->initialConsumerCount; i++)
    {
        this->consumerThreads.enqueue(this->startConsumerThread());
    }
}

std::thread TurningGrilleCrackerProducerConsumer::startConsumerThread()
{
    this->consumerCount++;
    return std::thread
        {
            [this]
            {
                std::optional<Grille> grille;
                while ((grille = this->portionQueue.retrievePortion()).has_value())
                {
                    this->applyGrille(*grille);
                    bool shutdown = this->shutdownOneConsumer.exchange(false);
                    if (shutdown && this->consumerCount > 1)
                    {
                        break;
                    }
                }
                this->consumerCount--;
            }
        };
}



TurningGrilleCrackerWithPerfectParallelism::TurningGrilleCrackerWithPerfectParallelism(const std::string& cipherText) :
    TurningGrilleCracker(cipherText)
{
}

void TurningGrilleCrackerWithPerfectParallelism::doBruteForce()
{
    unsigned cpuCount = std::thread::hardware_concurrency();

    std::list<std::thread> workerThreads = this->startWorkerThreads(cpuCount);

    for (std::thread& workerThread : workerThreads)
    {
        workerThread.join();
    }
}

std::list<std::thread> TurningGrilleCrackerWithPerfectParallelism::startWorkerThreads(unsigned workerCount)
{
    std::list<std::thread> workerThreads;

    uint64_t nextIntervalBegin = 0;
    uint64_t intervalLength = std::llround((long double)this->grilleCount / workerCount);
    for (unsigned i = 0; i < workerCount; i++)
    {
        workerThreads.push_back(std::thread
            {
                [this, nextIntervalBegin, i, workerCount, intervalLength]
                {
                    GrilleInterval grilleInterval(this->sideLength / 2, nextIntervalBegin,
                        (i < workerCount - 1) ? (nextIntervalBegin + intervalLength) : this->grilleCount);

                    const Grille* grille;
                    while ((grille = grilleInterval.getNext()))
                    {
                        this->applyGrille(*grille);
                    }
                }
            });

        nextIntervalBegin += intervalLength;
    }

    return workerThreads;
}

}
