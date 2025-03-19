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
inline static const std::string CLEAR_TEXT_PATH = "decrypted_msg.txt";


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
                    while (!stop.load(std::memory_order_relaxed));
                }
            });
    }

    std::this_thread::sleep_for(std::chrono::minutes(1));
    stop.store(true, std::memory_order_relaxed);

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
        std::string clearText;
        {
            std::ifstream clearTextStream(CLEAR_TEXT_PATH);
            if (clearTextStream.fail())
            {
                throw std::ios::failure("Cannot open file: " + CLEAR_TEXT_PATH);
            }

            std::getline(clearTextStream, clearText);
        }
        boost::to_upper(clearText);
        boost::erase_all_regex(clearText, turning_grille::NOT_CAPITAL_ENGLISH_LETTERS_RE);

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

            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::ConcurrentBlownQueue<turning_grille::Grille>>(initialConsumerCount, producerCount);
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "atomic")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::AtomicBlownQueue<turning_grille::Grille>>(initialConsumerCount, producerCount);
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "lockfree")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::LockfreeBlownQueue<turning_grille::Grille>>(initialConsumerCount, producerCount);
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "onetbb")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::OneTBB_BlownQueue<turning_grille::Grille>>(initialConsumerCount, producerCount);
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "onetbb_bounded")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::OneTBB_BoundedPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount);
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "sync_bounded")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::SyncBoundedPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount);
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
        }
        else if (arg == "textbook")
        {
            unsigned initialConsumerCount = cpuCount * 3;
            unsigned producerCount = cpuCount;

            std::unique_ptr<queue::MPMC_PortionQueue<turning_grille::Grille>> portionQueue =
            		std::make_unique<queue::TextbookPortionQueue<turning_grille::Grille>>(initialConsumerCount, producerCount);
            crackerImplDetails = std::make_unique<turning_grille::TurningGrilleCrackerProducerConsumer>(initialConsumerCount, producerCount, std::move(portionQueue));
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
        if (!VERBOSE)
        {
        	heatCpu();
        }
        std::set<std::string> candidates = cracker.bruteForce();

        if (candidates.find(clearText) == candidates.end())
        {
        	throw std::logic_error("The correct clear text was not found among the decrypted candidates.");
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << boost::core::demangle(typeid(e).name()) << ": " << e.what() << std::endl;
        return 1;
    }
}
