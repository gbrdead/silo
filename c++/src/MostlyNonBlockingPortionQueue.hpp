#ifndef __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__
#define __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__

#include "NonBlockingQueue.hpp"
#include "MPMC_PortionQueue.hpp"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <memory>



namespace org::voidland::concurrent::queue
{


template <class E>
class MostlyNonBlockingPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
	std::unique_ptr<NonBlockingQueue<E>> nonBlockingQueue;

    std::size_t maxSize;
    std::atomic<std::size_t> size;

    bool workDone;

    std::mutex notFullMutex;
    std::mutex notEmptyMutex;
    std::mutex emptyMutex;
    std::condition_variable notFullCondition;
    std::condition_variable notEmptyCondition;
    std::condition_variable emptyCondition;

    std::atomic<unsigned> blockedProducers;
    std::atomic<unsigned> blockedConsumers;

public:
    MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount, std::unique_ptr<NonBlockingQueue<E>> nonBlockingQueue);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::optional<E> retrievePortion();
    void ensureAllPortionsAreRetrieved();
    void stopConsumers(std::size_t consumerCount);
    std::size_t getSize();
    std::size_t getMaxSize();
    unsigned getBlockedProducers();
    unsigned getBlockedConsumers();
};


template <class E>
MostlyNonBlockingPortionQueue<E>::MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount, std::unique_ptr<NonBlockingQueue<E>> nonBlockingQueue) :
	nonBlockingQueue(std::move(nonBlockingQueue)),
    maxSize(initialConsumerCount * producerCount * 1000),
    size(0),
    workDone(false),
    notFullMutex(),
    notEmptyMutex(),
    emptyMutex(),
    notFullCondition(),
    notEmptyCondition(),
    emptyCondition(),
	blockedProducers(0),
	blockedConsumers(0)
{
	this->nonBlockingQueue->setSizeParameters(producerCount, this->maxSize + producerCount);
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

    while (true)
    {
        if (this->size >= this->maxSize)
        {
        	this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
            std::unique_lock lock(this->notFullMutex);

            while (this->size >= this->maxSize)
            {
                this->notFullCondition.wait(lock);
            }
            this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);
        }

        if (this->nonBlockingQueue->tryEnqueue(std::move(portion)))
        {
        	break;
        }
    }

    if (newSize == this->maxSize * 1 / 4)
    {
    	this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
        std::lock_guard lock(this->notEmptyMutex);
        this->notEmptyCondition.notify_all();
        this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);
    }
}

template <class E>
std::optional<E> MostlyNonBlockingPortionQueue<E>::retrievePortion()
{
    E portion;

    if (!this->nonBlockingQueue->tryDequeue(portion))
    {
    	this->blockedConsumers.fetch_add(1, std::memory_order_relaxed);
        std::unique_lock lock(this->notEmptyMutex);
        while (true)
        {
            if (this->workDone)
            {
            	this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);
                return std::nullopt;
            }
        	if (this->nonBlockingQueue->tryDequeue(portion))
        	{
        		break;
        	}
        	this->notEmptyCondition.wait(lock);
        }
        this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);
    }

    std::size_t newSize = --this->size;
    if (newSize == this->maxSize * 3 / 4)
    {
    	this->blockedConsumers.fetch_add(1, std::memory_order_relaxed);
        std::lock_guard lock(this->notFullMutex);
        this->notFullCondition.notify_all();
        this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);
    }
    if (newSize == 0)
    {
    	this->blockedConsumers.fetch_add(1, std::memory_order_relaxed);
        std::lock_guard lock(this->emptyMutex);
        this->emptyCondition.notify_one();
        this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);
    }

    return portion;
}

template <class E>
void MostlyNonBlockingPortionQueue<E>::ensureAllPortionsAreRetrieved()
{
    {
        std::lock_guard lock(this->notEmptyMutex);
        this->notEmptyCondition.notify_all();
    }

    {
        std::unique_lock lock(this->emptyMutex);
        while (this->size > 0)
        {
            this->emptyCondition.wait(lock);
        }
    }
}

template <class E>
void MostlyNonBlockingPortionQueue<E>::stopConsumers(std::size_t consumerCount)
{
	std::lock_guard lock(this->notEmptyMutex);
	this->workDone = true;
	this->notEmptyCondition.notify_all();
}

template <class E>
std::size_t MostlyNonBlockingPortionQueue<E>::getSize()
{
    return this->size.load(std::memory_order_relaxed);
}

template <class E>
std::size_t MostlyNonBlockingPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}

template <class E>
unsigned MostlyNonBlockingPortionQueue<E>::getBlockedProducers()
{
	return this->blockedProducers.load(std::memory_order_relaxed);
}

template <class E>
unsigned MostlyNonBlockingPortionQueue<E>::getBlockedConsumers()
{
	return this->blockedConsumers.load(std::memory_order_relaxed);
}


}

#endif
