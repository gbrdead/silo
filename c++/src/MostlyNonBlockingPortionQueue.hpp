#ifndef __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__
#define __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__

#include "NonBlockingQueue.hpp"
#include "MPMC_PortionQueue.hpp"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <memory>
#include <type_traits>
#include <new>



namespace org::voidland::concurrent::queue
{


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"

template <typename E, typename NBQ>
class MostlyNonBlockingPortionQueue :
    public MPMC_PortionQueue<E>
{
	static_assert(std::is_convertible<NBQ*, NonBlockingQueue<E>*>::value);
	static_assert(std::is_constructible<NBQ, std::size_t>::value);

private:
	alignas(std::hardware_destructive_interference_size) NBQ nonBlockingQueue;
	alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> size;

    alignas(std::hardware_destructive_interference_size) std::mutex notFullMutex;
    std::condition_variable notFullCondition;
    alignas(std::hardware_destructive_interference_size) std::mutex notEmptyMutex;
    std::condition_variable notEmptyCondition;
    alignas(std::hardware_destructive_interference_size) std::mutex emptyMutex;
    std::condition_variable emptyCondition;

	alignas(std::hardware_destructive_interference_size) std::atomic<bool> aProducerIsWaiting;
	alignas(std::hardware_destructive_interference_size) std::atomic<bool> aConsumerIsWaiting;

	alignas(std::hardware_destructive_interference_size) std::size_t maxSize;
    bool workDone;

public:
    MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::optional<E> retrievePortion();
    void ensureAllPortionsAreRetrieved();
    void stopConsumers(std::size_t finalConsumerCount);
    std::size_t getSize();
    std::size_t getMaxSize();
};

#pragma GCC diagnostic pop


template <typename E>
using ConcurrentBlownQueue = MostlyNonBlockingPortionQueue<E, ConcurrentPortionQueue<E>>;
template <typename E>
using AtomicBlownQueue = MostlyNonBlockingPortionQueue<E, AtomicPortionQueue<E>>;
template <typename E>
using LockfreeBlownQueue = MostlyNonBlockingPortionQueue<E, LockfreePortionQueue<E>>;
template <typename E>
using OneTBB_BlownQueue = MostlyNonBlockingPortionQueue<E, OneTBB_PortionQueue<E>>;


template <typename E, typename NBQ>
MostlyNonBlockingPortionQueue<E, NBQ>::MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    maxSize(initialConsumerCount * producerCount * 1000),
	nonBlockingQueue(this->maxSize),
    size(0),
    workDone(false),
    notFullMutex(),
    notEmptyMutex(),
    emptyMutex(),
    notFullCondition(),
    notEmptyCondition(),
    emptyCondition(),
	aProducerIsWaiting(false),
	aConsumerIsWaiting(false)
{
}

template <typename E, typename NBQ>
void MostlyNonBlockingPortionQueue<E, NBQ>::addPortion(const E& portion)
{
    E portionCopy(portion);
    this->addPortion(std::move(portionCopy));
}

template <typename E, typename NBQ>
void MostlyNonBlockingPortionQueue<E, NBQ>::addPortion(E&& portion)
{
    while (true)
    {
        if (this->size.load(std::memory_order_acquire) >= this->maxSize)
        {
            std::unique_lock lock(this->notFullMutex);

            while (this->size.load(std::memory_order_acquire) >= this->maxSize)
            {
            	this->aProducerIsWaiting.store(true, std::memory_order_release);
                this->notFullCondition.wait(lock);
            }
        }

        if (this->nonBlockingQueue.tryEnqueue(std::move(portion)))
        {
        	break;
        }
    }
    this->size.fetch_add(1, std::memory_order_release);

    bool expected = true;
    if (this->aConsumerIsWaiting.compare_exchange_strong(expected, false, std::memory_order_acq_rel, std::memory_order_relaxed))
    {
        std::lock_guard lock(this->notEmptyMutex);
        this->notEmptyCondition.notify_all();
    }
}

template <typename E, typename NBQ>
std::optional<E> MostlyNonBlockingPortionQueue<E, NBQ>::retrievePortion()
{
    E portion;

    if (!this->nonBlockingQueue.tryDequeue(portion))
    {
        std::unique_lock lock(this->notEmptyMutex);
        while (true)
        {
            if (this->workDone)
            {
                return std::nullopt;
            }

        	if (this->nonBlockingQueue.tryDequeue(portion))
        	{
        		break;
        	}

        	this->aConsumerIsWaiting.store(true, std::memory_order_release);
        	this->notEmptyCondition.wait(lock);
        }
    }
    std::size_t newSize = this->size.fetch_sub(1, std::memory_order_acq_rel) - 1;

    bool expected = true;
    if (this->aProducerIsWaiting.compare_exchange_strong(expected, false, std::memory_order_acq_rel, std::memory_order_relaxed))
    {
        std::lock_guard lock(this->notFullMutex);
        this->notFullCondition.notify_all();
    }

    if (newSize == 0)
    {
        std::lock_guard lock(this->emptyMutex);
        this->emptyCondition.notify_one();
    }

    return portion;
}

template <typename E, typename NBQ>
void MostlyNonBlockingPortionQueue<E, NBQ>::ensureAllPortionsAreRetrieved()
{
    {
        std::lock_guard lock(this->notEmptyMutex);
        this->notEmptyCondition.notify_all();
    }

    {
        std::unique_lock lock(this->emptyMutex);
        while (this->size.load(std::memory_order_acquire) > 0)
        {
            this->emptyCondition.wait(lock);
        }
    }
}

template <typename E, typename NBQ>
void MostlyNonBlockingPortionQueue<E, NBQ>::stopConsumers(std::size_t finalConsumerCount)
{
	std::lock_guard lock(this->notEmptyMutex);
	this->workDone = true;
	this->notEmptyCondition.notify_all();
}

template <typename E, typename NBQ>
std::size_t MostlyNonBlockingPortionQueue<E, NBQ>::getSize()
{
    return this->size.load(std::memory_order_relaxed);
}

template <typename E, typename NBQ>
std::size_t MostlyNonBlockingPortionQueue<E, NBQ>::getMaxSize()
{
    return this->maxSize;
}


}

#endif
