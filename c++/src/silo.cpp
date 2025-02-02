#include "BlockingPortionQueue.hpp"
#include "MostlyNonBlockingPortionQueue.hpp"
#include "TurningGrille.hpp"
using namespace org::voidland::concurrent;

#include <fstream>
#include <string>
#include <iostream>
#include <list>
#include <thread>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <boost/core/demangle.hpp>


inline static const std::string CIPHER_TEXT_PATH = "encrypted_msg.txt";


static void heatCpu()
{
    unsigned cpuCount = std::thread::hardware_concurrency();
    std::list<std::thread> workerThreads;

    volatile bool stop = false;

    for (unsigned i = 0; i < cpuCount; i++)
    {
        workerThreads.push_back(std::thread
            {
                [&stop]
                {
                    while (!stop);
                }
            });
    }

    std::this_thread::sleep_for(std::chrono::minutes(1));
    stop = true;

    for (std::thread& workerThread : workerThreads)
    {
        workerThread.join();
    }
}

int main(int argc, char *argv[])
{
    try
    {
        std::string cipherText;
        {
            std::ifstream cipherTextStream(CIPHER_TEXT_PATH);
            if (cipherTextStream.fail())
            {
                throw std::ios::failure("Cannot open file: " + CIPHER_TEXT_PATH);
            }

            std::getline(cipherTextStream, cipherText);
        }

        std::string arg;
        if (argc >= 2)
        {
            arg = argv[1];
            boost::to_lower(arg);
        }
        else
        {
            arg = "perfect";
        }

        std::unique_ptr<TurningGrilleCracker> cracker;
        std::unique_ptr<MPMC_PortionQueue<Grille>> portionQueue;
        
        unsigned cpuCount = std::thread::hardware_concurrency();
        if (arg == "concurrent")
        {
            unsigned initialConsumerCount = cpuCount * 1.5;
            unsigned producerCount = cpuCount;

            portionQueue.reset(new ConcurrentPortionQueue<Grille>(initialConsumerCount, producerCount));
            cracker.reset(new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, *portionQueue));
        }
        else if (arg == "atomic")
        {
            unsigned initialConsumerCount = cpuCount * 2;
            unsigned producerCount = cpuCount;

            portionQueue.reset(new AtomicPortionQueue<Grille>(initialConsumerCount, producerCount));
            cracker.reset(new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, *portionQueue));
        }
        else if (arg == "lockfree")
        {
            unsigned initialConsumerCount = cpuCount * 2.5;
            unsigned producerCount = cpuCount;

            portionQueue.reset(new LockfreePortionQueue<Grille>(initialConsumerCount, producerCount));
            cracker.reset(new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, *portionQueue));
        }
        else if (arg == "textbook")
        {
            unsigned initialConsumerCount = cpuCount * 4;
            unsigned producerCount = cpuCount;

            portionQueue.reset(new TextbookPortionQueue<Grille>(initialConsumerCount, producerCount));
            cracker.reset(new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, *portionQueue));
        }
        else if (arg == "onetbb")
        {
            unsigned initialConsumerCount = cpuCount * 1;
            unsigned producerCount = cpuCount;

            portionQueue.reset(new OneTBB_PortionQueue<Grille>(initialConsumerCount, producerCount));
            cracker.reset(new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, *portionQueue));
        }
        else if (arg == "onetbb_bounded")
        {
            unsigned initialConsumerCount = cpuCount * 1;
            unsigned producerCount = cpuCount;

            portionQueue.reset(new OneTBB_BoundedPortionQueue<Grille>(initialConsumerCount, producerCount));
            cracker.reset(new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, *portionQueue));
        }
        else if (arg == "perfect")
        {
            cracker.reset(new TurningGrilleCrackerWithPerfectParallelism(cipherText));
        }
        else
        {
            throw std::invalid_argument("Unexpected argument: " + arg);
        }

        heatCpu();
        cracker->bruteForce();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << boost::core::demangle(typeid(e).name()) << ": " << e.what() << std::endl;
    }
}
