#![allow(non_snake_case)]
#![allow(non_camel_case_types)]

#![feature(mpmc_channel)]

pub mod voidland;

use voidland::concurrent::VERBOSE;
use voidland::concurrent::queue::MostlyNonBlockingPortionQueue;
use voidland::concurrent::queue::MPMC_PortionQueue;
use voidland::concurrent::queue::StdTextbookPortionQueue;
use voidland::concurrent::queue::ParkingLotTextbookPortionQueue;
use voidland::concurrent::queue::SyncMpmcPortionQueue;
use voidland::concurrent::turning_grille::NOT_CAPITAL_ENGLISH_LETTERS_RE;
use voidland::concurrent::turning_grille::Grille;
use voidland::concurrent::turning_grille::TurningGrilleCracker;
use voidland::concurrent::turning_grille::TurningGrilleCrackerImplDetails;
use voidland::concurrent::turning_grille::TurningGrilleCrackerProducerConsumer;
use voidland::concurrent::turning_grille::TurningGrilleCrackerSyncless;
use voidland::concurrent::turning_grille::TurningGrilleCrackerSerial;

use std::fs::read_to_string;
use std::string::String;
use std::str::Lines;
use std::env;
use std::thread::available_parallelism;
use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;
use std::sync::Arc;
use std::thread;
use std::thread::JoinHandle;
use std::time;
use std::collections::BTreeSet;


const CIPHER_TEXT_PATH: &str = "encrypted_msg.txt";
const CLEAR_TEXT_PATH: &str = "decrypted_msg.txt";


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
                while !stop.load(Ordering::Relaxed)
                {
                }
            }));
    }
    
    thread::sleep(time::Duration::from_secs(60));
    stop.store(true, Ordering::Relaxed);
    
    for workerThread in &mut workerThreads.into_iter()
    {
        let _ = workerThread.join().unwrap();
    }
}

fn main()
{
    let cipherFileContent: String = read_to_string(CIPHER_TEXT_PATH).unwrap();
    let mut cipherFileLines: Lines = cipherFileContent.lines();
    let cipherText: String = cipherFileLines.next().unwrap().to_string();
    let minDetectedWordCount: usize = cipherFileLines.next().unwrap().parse().unwrap();
    
    let clearText: String = read_to_string(CLEAR_TEXT_PATH).unwrap().lines().next().unwrap().to_string();
    let clearText: String = clearText.to_uppercase();
    let clearText: String = NOT_CAPITAL_ENGLISH_LETTERS_RE.replace_all(&clearText, "").to_string();
    
    let args: Vec<String> = env::args().collect();
    let arg: &str;
    if args.len() >= 2
    {
        arg = &args[1];
    }
    else
    {
        arg = "syncless";
    }
    
    let crackerImplDetails: Box<dyn TurningGrilleCrackerImplDetails>;

    let cpuCount: usize = available_parallelism().unwrap().get();
    match arg
    {
        "concurrent" =>
        {
            let initialConsumerCount: usize = cpuCount * 3;
            let producerCount: usize = cpuCount;
            let maxQueueSize: usize = initialConsumerCount * producerCount * 1000;

            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = Arc::new(MostlyNonBlockingPortionQueue::createConcurrentBlownQueue(maxQueueSize));
            crackerImplDetails = Box::new(TurningGrilleCrackerProducerConsumer::new(initialConsumerCount, producerCount, portionQueue));
        }
        "async_mpmc" =>
        {
            let initialConsumerCount: usize = cpuCount * 3;
            let producerCount: usize = cpuCount;
            let maxQueueSize: usize = initialConsumerCount * producerCount * 1000;

            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = Arc::new(MostlyNonBlockingPortionQueue::createAsyncMPMC_BlownQueue(maxQueueSize));
            crackerImplDetails = Box::new(TurningGrilleCrackerProducerConsumer::new(initialConsumerCount, producerCount, portionQueue));
        }
        "textbook" =>
        {
            let initialConsumerCount: usize = cpuCount * 3;
            let producerCount: usize = cpuCount;
            let maxQueueSize: usize = initialConsumerCount * producerCount * 1000;

            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = Arc::new(StdTextbookPortionQueue::new(maxQueueSize));
            crackerImplDetails = Box::new(TurningGrilleCrackerProducerConsumer::new(initialConsumerCount, producerCount, portionQueue));
        }
        "textbook_pl" =>
        {
            let initialConsumerCount: usize = cpuCount * 3;
            let producerCount: usize = cpuCount;
            let maxQueueSize: usize = initialConsumerCount * producerCount * 1000;

            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = Arc::new(ParkingLotTextbookPortionQueue::new(maxQueueSize));
            crackerImplDetails = Box::new(TurningGrilleCrackerProducerConsumer::new(initialConsumerCount, producerCount, portionQueue));
        }
        "sync_mpmc" =>
        {
            let initialConsumerCount: usize = cpuCount * 3;
            let producerCount: usize = cpuCount;
            let maxQueueSize: usize = initialConsumerCount * producerCount * 1000;

            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = Arc::new(SyncMpmcPortionQueue::new(maxQueueSize));
            crackerImplDetails = Box::new(TurningGrilleCrackerProducerConsumer::new(initialConsumerCount, producerCount, portionQueue));
        }
        "syncless" =>
        {
            crackerImplDetails = Box::new(TurningGrilleCrackerSyncless::new());
        }
        "serial" =>
        {
            crackerImplDetails = Box::new(TurningGrilleCrackerSerial::new());
        }
        _ =>
        {
            panic!("Unexpected argument: {}", arg);
        }
    }
    
    let cracker: Arc<TurningGrilleCracker> = Arc::new(TurningGrilleCracker::new(&cipherText, minDetectedWordCount, crackerImplDetails));
    if !*VERBOSE
    {
        heatCpu();
    }
    let candidates: BTreeSet<String> = cracker.bruteForce();
    
    if !candidates.contains(&clearText)
    {
        panic!("The correct clear text was not found among the decrypted candidates.");
    }
}
