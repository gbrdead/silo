#ifndef __VOIDLAND_MOSTLY_NON_BLOCKING_WORK_QUEUE_HPP__
#define __VOIDLAND_MOSTLY_NON_BLOCKING_WORK_QUEUE_HPP__

#include "MPMC_PortionQueue.hpp"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <boost/lockfree/queue.hpp>
#include <oneapi/tbb/concurrent_queue.h>
#include "concurrentqueue/concurrentqueue.h"
#include "atomic_queue/atomic_queue.h"


namespace org::voidland::concurrent
{


template <class E>
class MostlyNonBlockingPortionQueue :
    public MPMC_PortionQueue<E>
{
protected:
    std::size_t maxSize;

private:
    std::atomic<std::size_t> size;
    bool workDone;

    std::mutex fullMutex;
    std::mutex emptyMutex;
    std::mutex notEmptyMutex;
    std::condition_variable fullCondition;
    std::condition_variable emptyCondition;
    std::condition_variable notEmptyCondition;

public:
    MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::optional<E> retrievePortion();
    void ensureAllPortionsAreRetrieved();
    void stopConsumers(std::size_t consumerCount);
    std::size_t getSize();
    std::size_t getMaxSize();

protected:
    virtual bool tryEnqueue(E&& portion) = 0;
    virtual bool tryDequeue(E& portion) = 0;
};


template <class E>
MostlyNonBlockingPortionQueue<E>::MostlyNonBlockingPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    maxSize(initialConsumerCount * producerCount * 1000),
    size(0),
    workDone(false)
{
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
    while (!this->tryEnqueue(std::move(portion)));

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

    if (!this->tryDequeue(portion))
    {
        std::unique_lock lock(this->emptyMutex);
        while (!this->tryDequeue(portion))
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



template <class E>
class ConcurrentPortionQueue :
    public MostlyNonBlockingPortionQueue<E>
{
private:
    moodycamel::ConcurrentQueue<E> queue;

public:
    ConcurrentPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

protected:
    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
ConcurrentPortionQueue<E>::ConcurrentPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    MostlyNonBlockingPortionQueue<E>(initialConsumerCount, producerCount),
    queue(this->maxSize + producerCount, 0, producerCount)
{
}

template <class E>
bool ConcurrentPortionQueue<E>::tryEnqueue(E&& portion)
{
    return this->queue.enqueue(std::move(portion));
}

template <class E>
bool ConcurrentPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_dequeue(portion);
}



template <class E>
class AtomicPortionQueue :
    public MostlyNonBlockingPortionQueue<E>
{
private:
    atomic_queue::AtomicQueueB2<E> queue;

public:
    AtomicPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

protected:
    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
AtomicPortionQueue<E>::AtomicPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    MostlyNonBlockingPortionQueue<E>(initialConsumerCount, producerCount),
    queue(this->maxSize + producerCount)
{
}

template <class E>
bool AtomicPortionQueue<E>::tryEnqueue(E&& portion)
{
    this->queue.push(std::move(portion));
    return true;
}

template <class E>
bool AtomicPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}



template <class E>
class LockfreePortionQueue :
    public MostlyNonBlockingPortionQueue<E>
{
private:
    boost::lockfree::queue<E*> queue;

public:
    LockfreePortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);
    ~LockfreePortionQueue();

protected:
    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
LockfreePortionQueue<E>::LockfreePortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    MostlyNonBlockingPortionQueue<E>(initialConsumerCount, producerCount),
    queue(this->maxSize + producerCount)
{
}

template <class E>
LockfreePortionQueue<E>::~LockfreePortionQueue()
{
    this->queue.consume_all(
        [](E* portionCopy)
        {
            delete portionCopy;
        });
}

template <class E>
bool LockfreePortionQueue<E>::tryEnqueue(E&& portion)
{
    E* portionCopy = new E(std::move(portion));
    bool success = this->queue.bounded_push(portionCopy);
    if (!success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}

template <class E>
bool LockfreePortionQueue<E>::tryDequeue(E& portion)
{
    E* portionCopy;
    bool success = this->queue.pop(portionCopy);
    if (success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}



template <class E>
class OneTBB_PortionQueue :
    public MostlyNonBlockingPortionQueue<E>
{
private:
    oneapi::tbb::concurrent_queue<E> queue;

public:
    OneTBB_PortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

protected:
    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
OneTBB_PortionQueue<E>::OneTBB_PortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    MostlyNonBlockingPortionQueue<E>(initialConsumerCount, producerCount)
{
}

template <class E>
bool OneTBB_PortionQueue<E>::tryEnqueue(E&& portion)
{
    this->queue.push(std::move(portion));
    return true;
}

template <class E>
bool OneTBB_PortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}


}

#endif
