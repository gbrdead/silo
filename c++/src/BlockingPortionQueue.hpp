#ifndef __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__
#define __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__

#include "MPMC_PortionQueue.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <thread>
#include <oneapi/tbb/concurrent_queue.h>
#include <boost/thread/sync_bounded_queue.hpp>


namespace org::voidland::concurrent::queue
{


template <class E>
class TextbookPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
	std::queue<E> queue;
	bool workDone;

    std::size_t maxSize;

    // Not using counting_semaphore because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104928 .
    std::mutex mutex;
    std::condition_variable notEmptyCondition;
    std::condition_variable notFullCondition;

    std::atomic<unsigned> blockedProducers;
    std::atomic<unsigned> blockedConsumers;

public:
    TextbookPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

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
TextbookPortionQueue<E>::TextbookPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
	queue(),
    maxSize(initialConsumerCount * producerCount * 1000),
    workDone(false),
	mutex(),
	notEmptyCondition(),
	notFullCondition(),
	blockedProducers(0),
	blockedConsumers(0)
{
}

template <class E>
void TextbookPortionQueue<E>::addPortion(const E& portion)
{
    E portionCopy(portion);
    this->addPortion(std::move(portionCopy));
}

template <class E>
void TextbookPortionQueue<E>::addPortion(E&& portion)
{
	this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
    std::unique_lock lock(this->mutex);

    while (this->queue.size() >= this->maxSize)
    {
        this->notFullCondition.wait(lock);
    }
    this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);

    this->queue.push(std::move(portion));

    this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
    this->notEmptyCondition.notify_one();
    this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);
}

template <class E>
std::optional<E> TextbookPortionQueue<E>::retrievePortion()
{
	this->blockedConsumers.fetch_add(1, std::memory_order_relaxed);
    std::unique_lock lock(this->mutex);

    while (this->queue.empty())
    {
        if (this->workDone)
        {
        	this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);
            return std::nullopt;
        }

        this->notEmptyCondition.wait(lock);
    }
    this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);

    E portion = std::move(this->queue.front());
    this->queue.pop();

    this->blockedConsumers.fetch_add(1, std::memory_order_relaxed);
    this->notFullCondition.notify_one();
    this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);

    return portion;
}

template <class E>
void TextbookPortionQueue<E>::ensureAllPortionsAreRetrieved()
{
    std::unique_lock lock(this->mutex);

    while (!this->queue.empty())
    {
        this->notFullCondition.wait(lock);
    }
}

template <class E>
void TextbookPortionQueue<E>::stopConsumers(std::size_t consumerCount)
{
    std::unique_lock lock(this->mutex);

    this->workDone = true;

    this->notEmptyCondition.notify_all();
}

template <class E>
std::size_t TextbookPortionQueue<E>::getSize()
{
    std::unique_lock lock(this->mutex);

    return this->queue.size();
}

template <class E>
std::size_t TextbookPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}

template <class E>
unsigned TextbookPortionQueue<E>::getBlockedProducers()
{
	return this->blockedProducers.load(std::memory_order_relaxed);
}

template <class E>
unsigned TextbookPortionQueue<E>::getBlockedConsumers()
{
	return this->blockedConsumers.load(std::memory_order_relaxed);
}



template <class E>
class OneTBB_BoundedPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
    std::size_t maxSize;
    oneapi::tbb::concurrent_bounded_queue<std::optional<E>> queue;

    std::atomic<unsigned> blockedProducers;
    std::atomic<unsigned> blockedConsumers;

public:
    OneTBB_BoundedPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

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
OneTBB_BoundedPortionQueue<E>::OneTBB_BoundedPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    maxSize(initialConsumerCount * producerCount * 1000),
	queue(),
	blockedProducers(0),
	blockedConsumers(0)
{
    this->queue.set_capacity(this->maxSize);
}

template <class E>
void OneTBB_BoundedPortionQueue<E>::addPortion(const E& portion)
{
	this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
    this->queue.push(portion);
    this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);
}

template <class E>
void OneTBB_BoundedPortionQueue<E>::addPortion(E&& portion)
{
	this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
    this->queue.push(std::move(portion));
    this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);
}

template <class E>
std::optional<E> OneTBB_BoundedPortionQueue<E>::retrievePortion()
{
    std::optional<E> portion;
    this->blockedConsumers.fetch_add(1, std::memory_order_relaxed);
    this->queue.pop(portion);
    this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);
    return portion;
}

template <class E>
void OneTBB_BoundedPortionQueue<E>::ensureAllPortionsAreRetrieved()
{
    while (!this->queue.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

template <class E>
void OneTBB_BoundedPortionQueue<E>::stopConsumers(std::size_t consumerCount)
{
    for (std::size_t i = 0; i < consumerCount; i++)
    {
        this->queue.push(std::nullopt);
    }
}

template <class E>
std::size_t OneTBB_BoundedPortionQueue<E>::getSize()
{
    return this->queue.size();
}

template <class E>
std::size_t OneTBB_BoundedPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}

template <class E>
unsigned OneTBB_BoundedPortionQueue<E>::getBlockedProducers()
{
	return this->blockedProducers.load(std::memory_order_relaxed);
}

template <class E>
unsigned OneTBB_BoundedPortionQueue<E>::getBlockedConsumers()
{
	return this->blockedConsumers.load(std::memory_order_relaxed);
}



template <class E>
class SyncBoundedPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
    std::size_t maxSize;
    boost::sync_bounded_queue<std::optional<E>> queue;

    std::atomic<unsigned> blockedProducers;
    std::atomic<unsigned> blockedConsumers;

public:
    SyncBoundedPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

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
SyncBoundedPortionQueue<E>::SyncBoundedPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    maxSize(initialConsumerCount * producerCount * 1000),
	queue(this->maxSize),
	blockedProducers(0),
	blockedConsumers(0)
{
}

template <class E>
void SyncBoundedPortionQueue<E>::addPortion(const E& portion)
{
	this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
    this->queue.push_back(portion);
    this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);
}

template <class E>
void SyncBoundedPortionQueue<E>::addPortion(E&& portion)
{
	this->blockedProducers.fetch_add(1, std::memory_order_relaxed);
    this->queue.push_back(std::move(portion));
    this->blockedProducers.fetch_sub(1, std::memory_order_relaxed);
}

template <class E>
std::optional<E> SyncBoundedPortionQueue<E>::retrievePortion()
{
    std::optional<E> portion;
    this->blockedConsumers.fetch_add(1, std::memory_order_relaxed);
    this->queue.pull_front(portion);
    this->blockedConsumers.fetch_sub(1, std::memory_order_relaxed);
    return portion;
}

template <class E>
void SyncBoundedPortionQueue<E>::ensureAllPortionsAreRetrieved()
{
    while (!this->queue.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

template <class E>
void SyncBoundedPortionQueue<E>::stopConsumers(std::size_t consumerCount)
{
    for (std::size_t i = 0; i < consumerCount; i++)
    {
        this->queue.push_back(std::nullopt);
    }
}

template <class E>
std::size_t SyncBoundedPortionQueue<E>::getSize()
{
    return this->queue.size();
}

template <class E>
std::size_t SyncBoundedPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}

template <class E>
unsigned SyncBoundedPortionQueue<E>::getBlockedProducers()
{
	return this->blockedProducers.load(std::memory_order_relaxed);
}

template <class E>
unsigned SyncBoundedPortionQueue<E>::getBlockedConsumers()
{
	return this->blockedConsumers.load(std::memory_order_relaxed);
}


}

#endif
