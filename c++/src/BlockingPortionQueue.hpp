#ifndef __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__
#define __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__

#include "MPMC_PortionQueue.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <thread>
#include <oneapi/tbb/concurrent_queue.h>


namespace org::voidland::concurrent::queue
{


template <class E>
class TextbookPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
    std::size_t maxSize;
    std::queue<E> queue;

    // Not using counting_semaphore because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104928 .
    std::mutex mutex;
    std::condition_variable fullCondition;
    std::condition_variable emptyCondition;
    bool workDone;

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
    workDone(false)
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
    std::unique_lock lock(this->mutex);

    while (this->queue.size() >= this->maxSize)
    {
        this->emptyCondition.wait(lock);
    }

    this->queue.push(std::move(portion));

    this->fullCondition.notify_one();
}

template <class E>
std::optional<E> TextbookPortionQueue<E>::retrievePortion()
{
    std::unique_lock lock(this->mutex);

    while (this->queue.empty())
    {
        if (this->workDone)
        {
            return std::nullopt;
        }

        this->fullCondition.wait(lock);
    }

    E portion = std::move(this->queue.front());
    this->queue.pop();

    this->emptyCondition.notify_one();

    return portion;
}

template <class E>
void TextbookPortionQueue<E>::ensureAllPortionsAreRetrieved()
{
    std::unique_lock lock(this->mutex);

    while (!this->queue.empty())
    {
        this->emptyCondition.wait(lock);
    }
}

template <class E>
void TextbookPortionQueue<E>::stopConsumers(std::size_t consumerCount)
{
    std::unique_lock lock(this->mutex);

    this->workDone = true;

    this->fullCondition.notify_all();
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
