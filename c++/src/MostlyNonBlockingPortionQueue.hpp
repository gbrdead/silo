#ifndef __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__
#define __VOIDLAND_MOSTLY_NON_BLOCKING_PORTION_QUEUE_HPP__

#include "NonBlockingQueue.hpp"
#include "MPMC_PortionQueue.hpp"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <memory>
#include <new>


namespace org::voidland::concurrent::queue
{


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"

template <typename E>
class MostlyNonBlockingPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
	alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> size;
	std::size_t maxSize;

	alignas(std::hardware_destructive_interference_size) std::mutex notFullMutex;
    std::condition_variable notFullCondition;

    alignas(std::hardware_destructive_interference_size) std::mutex notEmptyMutex;
    std::condition_variable notEmptyCondition;

    alignas(std::hardware_destructive_interference_size) std::mutex emptyMutex;
    std::condition_variable emptyCondition;

    alignas(std::hardware_destructive_interference_size) std::atomic<bool> aProducerIsWaiting;
    alignas(std::hardware_destructive_interference_size) std::atomic<bool> aConsumerIsWaiting;

    // Keep the wrapped queue as a separately allocated object, otherwise the performance will suffer.
	alignas(std::hardware_destructive_interference_size) std::unique_ptr<NonBlockingQueue<E>> nonBlockingQueue;
    bool workDone;

public:
    static std::unique_ptr<MostlyNonBlockingPortionQueue<E>> createConcurrentBlownQueue(std::size_t maxSize);
    static std::unique_ptr<MostlyNonBlockingPortionQueue<E>> createAtomicBlownQueue(std::size_t maxSize);
    static std::unique_ptr<MostlyNonBlockingPortionQueue<E>> createLockfreeBlownQueue(std::size_t maxSize);
    static std::unique_ptr<MostlyNonBlockingPortionQueue<E>> createOneTBB_BlownQueue(std::size_t maxSize);

    MostlyNonBlockingPortionQueue(std::size_t maxSize, std::unique_ptr<NonBlockingQueue<E>> nonBlockingQueue);

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
std::unique_ptr<MostlyNonBlockingPortionQueue<E>> MostlyNonBlockingPortionQueue<E>::createConcurrentBlownQueue(std::size_t maxSize)
{
    std::unique_ptr<queue::NonBlockingQueue<E>> nonBlockingQueue = std::make_unique<ConcurrentPortionQueue<E>>(maxSize);
	return std::make_unique<MostlyNonBlockingPortionQueue<E>>(maxSize, std::move(nonBlockingQueue));
}

template <typename E>
std::unique_ptr<MostlyNonBlockingPortionQueue<E>> MostlyNonBlockingPortionQueue<E>::createAtomicBlownQueue(std::size_t maxSize)
{
    std::unique_ptr<queue::NonBlockingQueue<E>> nonBlockingQueue = std::make_unique<AtomicPortionQueue<E>>(maxSize);
	return std::make_unique<MostlyNonBlockingPortionQueue<E>>(maxSize, std::move(nonBlockingQueue));
}

template <typename E>
std::unique_ptr<MostlyNonBlockingPortionQueue<E>> MostlyNonBlockingPortionQueue<E>::createLockfreeBlownQueue(std::size_t maxSize)
{
    std::unique_ptr<queue::NonBlockingQueue<E>> nonBlockingQueue = std::make_unique<LockfreePortionQueue<E>>(maxSize);
	return std::make_unique<MostlyNonBlockingPortionQueue<E>>(maxSize, std::move(nonBlockingQueue));
}

template <typename E>
std::unique_ptr<MostlyNonBlockingPortionQueue<E>> MostlyNonBlockingPortionQueue<E>::createOneTBB_BlownQueue(std::size_t maxSize)
{
    std::unique_ptr<queue::NonBlockingQueue<E>> nonBlockingQueue = std::make_unique<OneTBB_PortionQueue<E>>(maxSize);
	return std::make_unique<MostlyNonBlockingPortionQueue<E>>(maxSize, std::move(nonBlockingQueue));
}


template <typename E>
MostlyNonBlockingPortionQueue<E>::MostlyNonBlockingPortionQueue(std::size_t maxSize, std::unique_ptr<NonBlockingQueue<E>> nonBlockingQueue) :
	size(0),
    maxSize(maxSize),
    notFullMutex(),
	notFullCondition(),
    notEmptyMutex(),
	notEmptyCondition(),
    emptyMutex(),
    emptyCondition(),
	aProducerIsWaiting(false),
	aConsumerIsWaiting(false),
	nonBlockingQueue(std::move(nonBlockingQueue)),
    workDone(false)
{
}

template <typename E>
void MostlyNonBlockingPortionQueue<E>::addPortion(const E& portion)
{
    E portionCopy(portion);
    this->addPortion(std::move(portionCopy));
}

template <typename E>
void MostlyNonBlockingPortionQueue<E>::addPortion(E&& portion)
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

        if (this->nonBlockingQueue->tryEnqueue(std::move(portion)))
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

template <typename E>
std::optional<E> MostlyNonBlockingPortionQueue<E>::retrievePortion()
{
    E portion;

    if (!this->nonBlockingQueue->tryDequeue(portion))
    {
        std::unique_lock lock(this->notEmptyMutex);
        while (true)
        {
            if (this->workDone)
            {
                return std::nullopt;
            }

        	if (this->nonBlockingQueue->tryDequeue(portion))
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

template <typename E>
void MostlyNonBlockingPortionQueue<E>::ensureAllPortionsAreRetrieved()
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

template <typename E>
void MostlyNonBlockingPortionQueue<E>::stopConsumers(std::size_t finalConsumerCount)
{
	std::lock_guard lock(this->notEmptyMutex);
	this->workDone = true;
	this->notEmptyCondition.notify_all();
}

template <typename E>
std::size_t MostlyNonBlockingPortionQueue<E>::getSize()
{
    return this->size.load(std::memory_order_relaxed);
}

template <typename E>
std::size_t MostlyNonBlockingPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}


}

#endif
