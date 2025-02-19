#![allow(non_snake_case)]
#![allow(non_camel_case_types)]

pub mod org;

use org::voidland::concurrent::queue::ConcurrentPortionQueue;
use org::voidland::concurrent::queue::NonBlockingQueue;
use org::voidland::concurrent::queue::MostlyNonBlockingPortionQueue;
use org::voidland::concurrent::queue::MPMC_PortionQueue;
use org::voidland::concurrent::queue::TextbookPortionQueue;
use org::voidland::concurrent::turning_grille::Grille;
use org::voidland::concurrent::turning_grille::TurningGrilleCracker;
use org::voidland::concurrent::turning_grille::TurningGrilleCrackerImplDetails;
use org::voidland::concurrent::turning_grille::TurningGrilleCrackerProducerConsumer;
use org::voidland::concurrent::turning_grille::ProducerConsumerMilestoneDetails;
use org::voidland::concurrent::turning_grille::TurningGrilleCrackerSyncless;
use org::voidland::concurrent::turning_grille::SynclessMilestoneDetails;
use org::voidland::concurrent::turning_grille::TurningGrilleCrackerSerial;
use org::voidland::concurrent::turning_grille::SerialMilestoneDetails;

use std::fs::read_to_string;
use std::string::String;
use std::env;
use std::thread::available_parallelism;
use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;
use std::sync::Arc;
use std::thread;
use std::thread::JoinHandle;
use std::time;


const CIPHER_TEXT_PATH: &str = "encrypted_msg.txt";


fn heatCpu()
{
    let cpuCount: usize = available_parallelism().unwrap().get();
    let mut workerThreads: Vec<JoinHandle<_>> = Vec::new();
    
    let stop: Arc<AtomicBool> = Arc::new(AtomicBool::new(false));
    
    for _i in 0..cpuCount
    {
        let stop = stop.clone();
        workerThreads.push(thread::spawn(move ||
            {
                while !stop.load(Ordering::SeqCst)
                {
                }
            }));
    }
    
    thread::sleep(time::Duration::from_secs(60));
    stop.store(true, Ordering::SeqCst);
    
    for workerThread in &mut workerThreads.into_iter()
    {
        let _ = workerThread.join().unwrap();
    }
}

fn main()
{
    let cipherText: String = read_to_string(CIPHER_TEXT_PATH).unwrap().lines().next().unwrap().to_string();
    
    let args: Vec<String> = env::args().collect();
    let arg: &str;
    if args.len() > 1
    {
        arg = &args[1];
    }
    else
    {
        arg = "syncless";
    }

    let cpuCount: usize = available_parallelism().unwrap().get();
    match arg
    {
        "concurrent" =>
        {
            let initialConsumerCount: usize = cpuCount * 3;
            let producerCount:usize = cpuCount;

            let nonBlockingQueue: Box<dyn NonBlockingQueue<Grille>> = Box::new(ConcurrentPortionQueue::<Grille>::new());
            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> =
                Arc::new(MostlyNonBlockingPortionQueue::new(initialConsumerCount, producerCount, nonBlockingQueue));
            let crackerImplDetails: Box<dyn TurningGrilleCrackerImplDetails<ProducerConsumerMilestoneDetails>> =
                Box::new(TurningGrilleCrackerProducerConsumer::new(initialConsumerCount, producerCount, portionQueue));
            let cracker: Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>> =
                Arc::new(TurningGrilleCracker::new(&cipherText, crackerImplDetails));
                
            heatCpu();
            cracker.bruteForce();
        }
        "textbook" =>
        {
            let initialConsumerCount: usize = cpuCount * 3;
            let producerCount:usize = cpuCount;

            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> =
                Arc::new(TextbookPortionQueue::new(initialConsumerCount, producerCount));
            let crackerImplDetails: Box<dyn TurningGrilleCrackerImplDetails<ProducerConsumerMilestoneDetails>> =
                Box::new(TurningGrilleCrackerProducerConsumer::new(initialConsumerCount, producerCount, portionQueue));
            let cracker: Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>> =
                Arc::new(TurningGrilleCracker::new(&cipherText, crackerImplDetails));

            heatCpu();
            cracker.bruteForce();
        }
        "syncless" =>
        {
            let crackerImplDetails: Box<dyn TurningGrilleCrackerImplDetails<SynclessMilestoneDetails>> =
                Box::new(TurningGrilleCrackerSyncless::new());
            let cracker: Arc<TurningGrilleCracker<SynclessMilestoneDetails>> =
                Arc::new(TurningGrilleCracker::new(&cipherText, crackerImplDetails));
                
            heatCpu();
            cracker.bruteForce();
        }
        "serial" =>
        {
            let crackerImplDetails: Box<dyn TurningGrilleCrackerImplDetails<SerialMilestoneDetails>> =
                Box::new(TurningGrilleCrackerSerial::new());
            let cracker: Arc<TurningGrilleCracker<SerialMilestoneDetails>> =
                Arc::new(TurningGrilleCracker::new(&cipherText, crackerImplDetails));
                
            heatCpu();
            cracker.bruteForce();
        }
        _ =>
        {
            panic!("Unexpected argument: {}", arg);
        }
    }
}
