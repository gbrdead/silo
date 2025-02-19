#include "BlockingPortionQueue.hpp"
#include "MostlyNonBlockingPortionQueue.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <list>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <boost/algorithm/string.hpp>
#include <boost/core/demangle.hpp>
#include "TurningGrilleCracker.hpp"

using namespace org::voidland::concurrent;


inline static const std::string CIPHER_TEXT_PATH = "encrypted_msg.txt";


static void heatCpu()
{
    unsigned cpuCount = std::thread::hardware_concurrency();
    std::vector<std::thread> workerThreads;
    workerThreads.reserve(cpuCount);

    std::atomic<bool> stop(false);

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
            arg = "syncless";
        }

        std::unique_ptr<turning_grille::TurningGrilleCrackerImplDetails> crackerImplDetails;
        
        unsigned cpuCount = std::thread::hardware_concurrency();
        if (arg == "concurrent")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::NonBlockingQueue<turning_grille::Grille>> nonBlockingQueue =
            		std::make_unique<queue::ConcurrentPortionQueue<turning_grille::Grille>>();
            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::MostlyNonBlockingPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount, std::move(nonBlockingQueue));
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "atomic")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::NonBlockingQueue<turning_grille::Grille>> nonBlockingQueue =
            		std::make_unique<queue::AtomicPortionQueue<turning_grille::Grille>>();
            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::MostlyNonBlockingPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount, std::move(nonBlockingQueue));
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "lockfree")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::NonBlockingQueue<turning_grille::Grille>> nonBlockingQueue =
            		std::make_unique<queue::LockfreePortionQueue<turning_grille::Grille>>();
            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::MostlyNonBlockingPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount, std::move(nonBlockingQueue));
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "sync_bounded")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount,
            		std::make_unique<queue::SyncBoundedPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount));
        }
        else if (arg == "textbook")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount,
            		std::make_unique<queue::TextbookPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount));
        }
        else if (arg == "onetbb")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::NonBlockingQueue<turning_grille::Grille>> nonBlockingQueue =
            		std::make_unique<queue::OneTBB_PortionQueue<turning_grille::Grille>>();
            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::MostlyNonBlockingPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount, std::move(nonBlockingQueue));
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "onetbb_bounded")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount,
            		std::make_unique<queue::OneTBB_BoundedPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount));
        }
        else if (arg == "syncless")
        {
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerSyncless>();
        }
        else if (arg == "serial")
        {
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerSerial>();
        }
        else
        {
            throw std::invalid_argument("Unexpected argument: " + arg);
        }

        turning_grille::TurningGrilleCracker cracker(cipherText, std::move(crackerImplDetails));
        heatCpu();
        cracker.bruteForce();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << boost::core::demangle(typeid(e).name()) << ": " << e.what() << std::endl;
    }
}
