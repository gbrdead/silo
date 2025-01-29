#ifndef __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__
#define __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__

#include "MPMC_PortionQueue.hpp"

#include <queue>
#include <mutex>
#include <semaphore>
#include <utility>
#include <thread>
#include <oneapi/tbb/concurrent_queue.h>


namespace org::voidland::concurrent
{


template <class E>
class TextbookPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
    std::size_t maxSize;
    std::queue<E> queue;

    std::mutex mutex;
    std::counting_semaphore<> fullSemaphore;
    std::counting_semaphore<> emptySemaphore;

public:
    TextbookPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::optional<E> retrievePortion();
    void ensureAllPortionsAreRetrieved();
    void stopConsumers(std::size_t consumerCount);
    std::size_t getSize();
    std::size_t getMaxSize();
};


template <class E>
TextbookPortionQueue<E>::TextbookPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    maxSize(initialConsumerCount * producerCount * 1000),
    fullSemaphore(0),
    emptySemaphore(maxSize)
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
    this->emptySemaphore.acquire();
    {
        std::unique_lock lock(this->mutex);
        this->queue.push(std::move(portion));
    }
    this->fullSemaphore.release();
}

template <class E>
std::optional<E> TextbookPortionQueue<E>::retrievePortion()
{
    this->fullSemaphore.acquire();

    E portion;
    {
        std::unique_lock lock(this->mutex);

        if (this->queue.empty())
        {
            return std::nullopt;
        }

        portion = std::move(this->queue.front());
        this->queue.pop();
    }

    this->emptySemaphore.release();

    return portion;
}

template <class E>
void TextbookPortionQueue<E>::ensureAllPortionsAreRetrieved()
{
    for (std::size_t i = 0; i < this->maxSize; i++)
    {
        this->emptySemaphore.acquire();
    }
}

template <class E>
void TextbookPortionQueue<E>::stopConsumers(std::size_t consumerCount)
{
    this->fullSemaphore.release(consumerCount);
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
class OneTBB_BoundedPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
    std::size_t maxSize;
    oneapi::tbb::concurrent_bounded_queue<std::optional<E>> queue;

public:
    OneTBB_BoundedPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::optional<E> retrievePortion();
    void ensureAllPortionsAreRetrieved();
    void stopConsumers(std::size_t consumerCount);
    std::size_t getSize();
    std::size_t getMaxSize();
};


template <class E>
OneTBB_BoundedPortionQueue<E>::OneTBB_BoundedPortionQueue(std::size_t initialConsumerCount, std::size_t producerCount) :
    maxSize(initialConsumerCount * producerCount * 1000)
{
    this->queue.set_capacity(this->maxSize);
}

template <class E>
void OneTBB_BoundedPortionQueue<E>::addPortion(const E& portion)
{
    this->queue.push(portion);
}

template <class E>
void OneTBB_BoundedPortionQueue<E>::addPortion(E&& portion)
{
    this->queue.push(std::move(portion));
}

template <class E>
std::optional<E> OneTBB_BoundedPortionQueue<E>::retrievePortion()
{
    std::optional<E> portion;
    this->queue.pop(portion);
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


}

#endif
