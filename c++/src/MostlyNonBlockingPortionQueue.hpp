#ifndef __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__
#define __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__

#include "MPMC_PortionQueue.hpp"
#include "UnboundedNonBlockingQueue.hpp"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <utility>


namespace org::voidland::concurrent::queue
{


template <class E>
class MostlyNonBlockingPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
	std::unique_ptr<UnboundedNonBlockingQueue<E>> unboundedQueue;

    std::size_t maxSize;
    std::atomic<std::size_t> size;
    bool workDone;

    std::mutex fullMutex;
    std::mutex emptyMutex;
    std::mutex notEmptyMutex;
    std::condition_variable fullCondition;
    std::condition_variable emptyCondition;
    std::condition_variable notEmptyCondition;

public:
    MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount, std::unique_ptr<UnboundedNonBlockingQueue<E>> unboundedQueue);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::optional<E> retrievePortion();
    void ensureAllPortionsAreRetrieved();
    void stopConsumers(std::size_t consumerCount);
    std::size_t getSize();
    std::size_t getMaxSize();
};


template <class E>
MostlyNonBlockingPortionQueue<E>::MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount, std::unique_ptr<UnboundedNonBlockingQueue<E>> unboundedQueue) :
	unboundedQueue(std::move(unboundedQueue)),
    maxSize(initialConsumerCount * producerCount * 1000),
    size(0),
    workDone(false)
{
	this->unboundedQueue->setSizeParameters(producerCount, this->maxSize + producerCount);
}

template <class E>
void MostlyNonBlockingPortionQueue<E>::addPortion(const E& portion)
{
    E portionCopy(portion);
    this->addPortion(std::move(portionCopy));
}

template <class E>
void MostlyNonBlockingPortionQueue<E>::addPortion(E&& portion)
{
    std::size_t newSize = ++this->size;

    do
    {
        if (this->size > this->maxSize)
        {
            std::unique_lock lock(this->fullMutex);
            while (this->size > this->maxSize)
            {
                this->fullCondition.wait(lock);
            }
        }
    }
    while (!this->unboundedQueue->tryEnqueue(std::move(portion)));

    if (newSize == this->maxSize * 1 / 4)
    {
        std::lock_guard lock(this->emptyMutex);
        this->emptyCondition.notify_all();
    }
}

template <class E>
std::optional<E> MostlyNonBlockingPortionQueue<E>::retrievePortion()
{
    E portion;

    if (!this->unboundedQueue->tryDequeue(portion))
    {
        std::unique_lock lock(this->emptyMutex);
        while (!this->unboundedQueue->tryDequeue(portion))
        {
            if (this->workDone)
            {
                return std::nullopt;
            }
            this->emptyCondition.wait(lock);
        }
    }

    std::size_t newSize = --this->size;
    if (newSize == this->maxSize * 3 / 4)
    {
        std::lock_guard lock(this->fullMutex);
        this->fullCondition.notify_all();
    }
    if (newSize == 0)
    {
        std::lock_guard lock(this->notEmptyMutex);
        this->notEmptyCondition.notify_one();
    }

    return portion;
}

template <class E>
void MostlyNonBlockingPortionQueue<E>::ensureAllPortionsAreRetrieved()
{
    {
        std::lock_guard lock(this->emptyMutex);
        this->emptyCondition.notify_all();
    }

    {
        std::unique_lock lock(this->notEmptyMutex);
        while (this->size > 0)
        {
            this->notEmptyCondition.wait(lock);
        }
    }
}

template <class E>
void MostlyNonBlockingPortionQueue<E>::stopConsumers(std::size_t consumerCount)
{
    this->workDone = true;
    {
        std::lock_guard lock(this->emptyMutex);
        this->emptyCondition.notify_all();
    }
}

template <class E>
std::size_t MostlyNonBlockingPortionQueue<E>::getSize()
{
    return this->size;
}

template <class E>
std::size_t MostlyNonBlockingPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}


}

#endif
