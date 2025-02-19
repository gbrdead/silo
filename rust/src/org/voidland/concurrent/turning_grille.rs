mod words_trie;
mod grille;

pub use grille::Grille;
use grille::GrilleInterval;
use words_trie::WordsTrie;
use super::VERBOSE;
use super::queue::MPMC_PortionQueue;

use std::vec::Vec;
use std::clone::Clone;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::AtomicUsize;
use std::sync::atomic::AtomicIsize;
use std::sync::atomic::Ordering;
use std::sync::Mutex;
use std::sync::Arc;
use std::thread;
use std::thread::JoinHandle;
use std::thread::available_parallelism;
use std::time::Instant;
use std::time::Duration;
use regex::Regex;
use std::string::String;
use concurrent_queue::ConcurrentQueue;


pub trait TurningGrilleCrackerImplDetails<M> : Send + Sync
{
    fn takeMilestoneState(&self) -> M;
    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker<M>>);
    fn milestone(self: Arc<Self>, _cracker: &Arc<TurningGrilleCracker<M>>, _milestoneState: &mut M, _grillesPerSecond: u64) -> String
    {
        String::new()
    }
    fn milestonesSummary(self: Arc<Self>, _cracker: &Arc<TurningGrilleCracker<M>>, _milestoneState: &mut M) -> String
    {
        String::new()
    }
}

struct TurningGrilleCrackerUnsync<M>
{
    grilleCountAtMilestoneStart: u64,
    start: Instant,
    milestoneStart: Instant,
    bestGrillesPerSecond: u64,
    milestoneState: M
}

pub struct TurningGrilleCracker<M>
{
    VERBOSE: bool,
    
    sideLength: usize,
    grilleCount: u64,
    grilleCountSoFar: AtomicU64,

    cipherText: Vec<char>,
    wordsTrie: Arc<WordsTrie>,
    
    milestoneReportMutex: Mutex<TurningGrilleCrackerUnsync<M>>,
    
    implDetails: Arc<dyn TurningGrilleCrackerImplDetails<M>>
}

impl<M> TurningGrilleCracker<M>
{
    const WORDS_FILE_PATH: &str = "3000words.txt";
    const MIN_DETECTED_WORD_COUNT: usize = 17;  // Determined by gut feeling.
    
    
    pub fn new(cipherText: &str, implDetails: Box<dyn TurningGrilleCrackerImplDetails<M>>) -> Self
    {
        let cipherText: String = cipherText.to_uppercase();
        let nonLettersRe = Regex::new(r"[^A-Z]").unwrap();
        if nonLettersRe.is_match(&cipherText)
        {
            panic!("The ciphertext must contain only English letters.");
        }
        let cipherText: Vec<char> = cipherText.chars().collect();
        
        let sideLength: usize = (cipherText.len() as f32).sqrt() as usize;
        if sideLength == 0  ||  sideLength % 2 != 0  ||  sideLength * sideLength != cipherText.len()
        {
            panic!("The ciphertext length must be a square of a positive odd number.");
        }
        
        let mut grilleCount: u64 = 1;
        for _i in 0..(sideLength * sideLength / 4)
        {
            grilleCount *= 4;
        }
        
        Self
        {
            VERBOSE: *VERBOSE,
            sideLength: sideLength,
            grilleCount: grilleCount,
            grilleCountSoFar: AtomicU64::new(0),
            cipherText: cipherText,
            wordsTrie: Arc::new(WordsTrie::new(TurningGrilleCracker::<M>::WORDS_FILE_PATH)),
            milestoneReportMutex: Mutex::new(TurningGrilleCrackerUnsync
                {
                    grilleCountAtMilestoneStart: 0,
                    start: Instant::now(),
                    milestoneStart: Instant::now(),
                    bestGrillesPerSecond: 0,
                    milestoneState: implDetails.takeMilestoneState()
                }),
            implDetails: Arc::from(implDetails)
        }
    }
    
    pub fn bruteForce(self: &Arc<Self>)
    {
        {
            let mut selfUnsync = self.milestoneReportMutex.lock().unwrap();
            selfUnsync.start = Instant::now();
            selfUnsync.milestoneStart = selfUnsync.start;
        }
        
        self.implDetails.clone().bruteForce(self);
        
        {
            let mut selfUnsync = self.milestoneReportMutex.lock().unwrap();
            let end: Instant = Instant::now();
            let elapsedTimeNs: u128 = end.duration_since(selfUnsync.start).as_nanos();
            let grillsPerSecond: u64 = ((self.grilleCount as u128 * Duration::from_secs(1).as_nanos()) / elapsedTimeNs) as u64;
            
            eprint!("Average speed: {} grilles/s; best speed: {} grilles/s", grillsPerSecond, selfUnsync.bestGrillesPerSecond);
            let summary: String = self.implDetails.clone().milestonesSummary(self, &mut selfUnsync.milestoneState);
            if !summary.is_empty()
            {
                eprint!("; {}", summary);
            }
            eprintln!();
        }
        
        if self.grilleCountSoFar.load(Ordering::SeqCst) != self.grilleCount
        {
            panic!("Some grilles got lost.");
        }
    }
    
    fn applyGrille(self: &Arc<Self>, grille: &Grille) -> u64
    {
        let cipherTextLength: usize = self.cipherText.len();
        let mut candidateBuf: String = String::with_capacity(cipherTextLength);
        
        for rotation in 0..4
        {
            for y in 0..self.sideLength
            {
                for x in 0..self.sideLength
                {
                    if grille.isHole(x, y, rotation)
                    {
                        candidateBuf.push(self.cipherText[y * self.sideLength + x]);
                    }
                }
            }
        }
        
        self.findWordsAndReport(&candidateBuf);
        let mut candidateRevBuf: String = String::with_capacity(cipherTextLength);
        // The following loop may be replaced by: candidateBuf.chars().rev().collect_into(&mut candidateRevBuf);
        for c in candidateBuf.chars().rev()
        {
            candidateRevBuf.push(c);
        }
        self.findWordsAndReport(&candidateRevBuf);
        
        self.grilleCountSoFar.fetch_add(1, Ordering::SeqCst) + 1
    }

    fn registerOneAppliedGrill(self: &Arc<Self>, grilleCountSoFar:u64)
    {
        if grilleCountSoFar % (self.grilleCount / 1000) == 0   // A milestone every 0.1%.
        {
            let milestoneEnd: Instant = Instant::now();
            
            let milestoneReportTryLock = self.milestoneReportMutex.try_lock();
            if milestoneReportTryLock.is_ok()
            {
                let mut selfUnsync = milestoneReportTryLock.unwrap();
                
                let elapsedTime: Duration = milestoneEnd.duration_since(selfUnsync.milestoneStart);
                let elapsedTimeNs:u128 = elapsedTime.as_nanos();
                if elapsedTimeNs > 0
                {
                    let grilleCountForMilestone: u64 = grilleCountSoFar - selfUnsync.grilleCountAtMilestoneStart;
                    let grillesPerSecond: u64 = (grilleCountForMilestone as u128 * Duration::from_secs(1).as_nanos() / elapsedTimeNs) as u64;
                    if grillesPerSecond > selfUnsync.bestGrillesPerSecond
                    {
                        selfUnsync.bestGrillesPerSecond = grillesPerSecond;
                    }
                    
                    let milestoneStatus: String = self.implDetails.clone().milestone(self, &mut selfUnsync.milestoneState, grillesPerSecond);
                    
                    if self.VERBOSE
                    {
                        let done: f32 = grilleCountSoFar as f32 * 100.0 / self.grilleCount as f32;
    
                        eprint!("{:.1}% done; ", done);
                        eprint!("current speed: {} grilles/s; ", grillesPerSecond);
                        eprint!("best speed so far: {} grilles/s", selfUnsync.bestGrillesPerSecond);
                        if !milestoneStatus.is_empty()
                        {
                            eprint!("; {}", milestoneStatus);
                        }
                        eprintln!();
                    }
                    
                    selfUnsync.milestoneStart = milestoneEnd;
                    selfUnsync.grilleCountAtMilestoneStart = grilleCountSoFar;
                }
            }
        }
    }
    
    fn findWordsAndReport(&self, candidate: &str)
    {
        let wordsFound: usize = self.wordsTrie.countWords(candidate);
        if wordsFound >= TurningGrilleCracker::<M>::MIN_DETECTED_WORD_COUNT
        {
            if self.VERBOSE
            {
                println!("{}: {}", wordsFound, candidate);
            }
        }        
    }
}



pub struct ProducerConsumerMilestoneDetails
{
    improving: isize,
    addingThreads: bool,
    prevGrillesPerSecond: u64,
    bestConsumerCount: usize,
    bestGrillesPerSecond: u64    
}

pub struct TurningGrilleCrackerProducerConsumer
{
    initialConsumerCount: usize,
    producerCount: usize,
    portionQueue: Arc<dyn MPMC_PortionQueue<Grille>>,

    consumerCount: AtomicIsize,         // This may get negative for a short while, so don't make it unsigned.
    consumerThreads: ConcurrentQueue<JoinHandle<()>>,
    shutdownNConsumers: AtomicIsize     // This may get negative for a short while, so don't make it unsigned.
}

impl TurningGrilleCrackerImplDetails<ProducerConsumerMilestoneDetails> for TurningGrilleCrackerProducerConsumer
{
    fn takeMilestoneState(&self) -> ProducerConsumerMilestoneDetails
    {
        ProducerConsumerMilestoneDetails
        {
            improving: 0,
            addingThreads: true,
            prevGrillesPerSecond: 0,
            bestConsumerCount: 0,
            bestGrillesPerSecond: 0 
        }
    }
    
    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>>)
    {
        let producerThreads: Vec<JoinHandle<()>> = self.startProducerThreads(cracker);
        self.startInitialConsumerThreads(cracker);
        
        for producerThread in &mut producerThreads.into_iter()
        {
            let _ = producerThread.join().unwrap();
        }
        
        self.portionQueue.ensureAllPortionsAreRetrieved();
        while cracker.grilleCountSoFar.load(Ordering::SeqCst) < cracker.grilleCount // Ensures all work is done and no more consumers will be started or stopped.
        {
        }
        self.portionQueue.stopConsumers(self.consumerCount.load(Ordering::SeqCst) as usize);

        loop
        {
            match self.consumerThreads.pop()
            {
                Ok(consumerThread) =>
                {
                    let _ = consumerThread.join().unwrap();
                }
                Err(_) =>
                {
                    break;
                }
            }
        }
    }
    
    fn milestone(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>>, milestoneState: &mut ProducerConsumerMilestoneDetails, grillesPerSecond: u64) -> String
    {
        let queueSize: usize = self.portionQueue.getSize();
        
        if grillesPerSecond > milestoneState.bestGrillesPerSecond
        {
            milestoneState.bestGrillesPerSecond = grillesPerSecond;
            milestoneState.bestConsumerCount = self.consumerCount.load(Ordering::SeqCst) as usize;
        }
        
        if cracker.grilleCountSoFar.load(Ordering::SeqCst) < cracker.grilleCount
        {
            if grillesPerSecond < milestoneState.prevGrillesPerSecond
            {
                milestoneState.improving -= 1;
            }
            else if grillesPerSecond > milestoneState.prevGrillesPerSecond
            {
                milestoneState.improving += 1;
            }

            if milestoneState.improving >= 1 || milestoneState.improving <= -2
            {
                if milestoneState.improving < 0
                {
                    milestoneState.addingThreads = !milestoneState.addingThreads;
                }
                milestoneState.improving = 0;

                if milestoneState.addingThreads
                {
                    let _ = self.consumerThreads.push(self.startConsumerThread(cracker));
                }
                else
                {
                    self.shutdownNConsumers.fetch_add(1, Ordering::SeqCst);
                }
            }

            milestoneState.prevGrillesPerSecond = grillesPerSecond;
        }
        
        let mut ret: String = String::new();        
        if cracker.VERBOSE
        {
            ret.push_str("consumer threads: ");
            ret.push_str(self.consumerCount.load(Ordering::SeqCst).to_string().as_str());
            ret.push_str("; queue size: ");
            ret.push_str(queueSize.to_string().as_str());
            ret.push_str(" / ");
            ret.push_str(self.portionQueue.getMaxSize().to_string().as_str());
        }
        ret
    }
    
    fn milestonesSummary(self: Arc<Self>, _cracker: &Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>>, milestoneState: &mut ProducerConsumerMilestoneDetails) -> String
    {
        let mut ret: String = String::from("best consumer threads: ");
        ret.push_str(milestoneState.bestConsumerCount.to_string().as_str());
        ret
    }
}

impl TurningGrilleCrackerProducerConsumer
{
    pub fn new(initialConsumerCount: usize, producerCount: usize, portionQueue: Arc<dyn MPMC_PortionQueue<Grille>>) -> Self
    {
        Self
        {
            initialConsumerCount: initialConsumerCount,
            producerCount: producerCount,
            portionQueue: portionQueue,

            consumerCount: AtomicIsize::new(0),
            consumerThreads: ConcurrentQueue::<JoinHandle<()>>::unbounded(),
            shutdownNConsumers: AtomicIsize::new(0)
        }
    }
    
    fn startProducerThreads(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>>) -> Vec<JoinHandle<()>>
    {
        let mut producerThreads: Vec<JoinHandle<()>> = Vec::with_capacity(self.producerCount);
        
        let mut nextIntervalBegin: u64 = 0;
        let intervalLength: u64 = (cracker.grilleCount as f64 / self.producerCount as f64).round() as u64;
        for i in 0..self.producerCount
        {
            let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = self.portionQueue.clone();
            let mut grilleInterval: GrilleInterval = GrilleInterval::new(cracker.sideLength / 2, nextIntervalBegin,
                if i < self.producerCount - 1 { nextIntervalBegin + intervalLength } else { cracker.grilleCount });
                
            producerThreads.push(thread::spawn(move ||
                {
                    loop
                    {
                        let grille: Option<Grille> = grilleInterval.cloneNext();
                        match grille
                        {
                            None =>
                            {
                                break;    
                            }
                            Some(grille) =>
                            {
                                portionQueue.addPortion(grille);
                            }
                        };
                    }
                }));
                
            nextIntervalBegin += intervalLength;
        }
        
        producerThreads
    }
    
    fn startInitialConsumerThreads(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>>)
    {
        for _i in 0..self.initialConsumerCount
        {
            let _ = self.consumerThreads.push(self.startConsumerThread(cracker));
        }
    }
    
    fn startConsumerThread(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>>) -> JoinHandle<()>
    {
        self.consumerCount.fetch_add(1, Ordering::SeqCst);
        
        let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = self.portionQueue.clone();
        let cracker: Arc<TurningGrilleCracker<ProducerConsumerMilestoneDetails>> = cracker.clone();
        let self_: Arc<Self> = self.clone();
        thread::spawn(move ||
            {
                loop
                {
                    let grille: Option<Grille> = portionQueue.retrievePortion();
                    let grilleCountSoFar:u64 = match grille
                    {
                        None =>
                        {
                            self_.consumerCount.fetch_sub(1, Ordering::SeqCst);
                            break;    
                        }
                        Some(grille) =>
                        {
                            cracker.applyGrille(&grille)
                        }
                    };
                    cracker.registerOneAppliedGrill(grilleCountSoFar);
                    
                    if self_.shutdownNConsumers.load(Ordering::SeqCst) > 0
                    {
                        if self_.shutdownNConsumers.fetch_sub(1, Ordering::SeqCst) > 0
                        {
                            if self_.consumerCount.fetch_sub(1, Ordering::SeqCst) > 1   // There should be at least one consumer running.
                            {
                                break;
                            }
                            else
                            {
                                self_.consumerCount.fetch_add(1, Ordering::SeqCst);
                            }
                        }
                        else
                        {
                            self_.shutdownNConsumers.fetch_add(1, Ordering::SeqCst);
                        }
                    }
                }
            })
    }
}



pub struct SynclessMilestoneDetails
{
    grilleIntervalsCompletion: Vec<(Arc<AtomicU64>, u64)>
}

pub struct TurningGrilleCrackerSyncless
{
    workersCount: AtomicUsize
}

impl TurningGrilleCrackerImplDetails<SynclessMilestoneDetails> for TurningGrilleCrackerSyncless
{
    fn takeMilestoneState(&self) -> SynclessMilestoneDetails
    {
        SynclessMilestoneDetails
        {
            grilleIntervalsCompletion: Vec::<(Arc<AtomicU64>, u64)>::new()
        }
    }

    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker<SynclessMilestoneDetails>>)
    {
        let cpuCount: usize = available_parallelism().unwrap().get();
        
        let workerThreads: Vec<JoinHandle<()>> = self.startWorkerThreads(cracker, cpuCount);

        for workerThread in &mut workerThreads.into_iter()
        {
            let _ = workerThread.join().unwrap();
        }
    }
    
    fn milestone(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker<SynclessMilestoneDetails>>, milestoneState: &mut SynclessMilestoneDetails, _grillesPerSecond: u64) -> String
    {
        let mut ret: String = String::new();        
        if cracker.VERBOSE
        {
            ret.push_str("worker threads: ");
            ret.push_str(self.workersCount.load(Ordering::SeqCst).to_string().as_str());
            ret.push_str("; completion per thread: ");

            let mut first: bool = true;
            for (processedGrillsCount, totalGrillsCount) in &milestoneState.grilleIntervalsCompletion
            {
                if !first
                {
                    ret.push_str("/");
                }
                else
                {
                    first = false;
                }
                
                let processedGrillsCount: u64 = processedGrillsCount.load(Ordering::SeqCst);
                let completion: f32 = processedGrillsCount as f32 * 100.0 / *totalGrillsCount as f32;
                ret.push_str(format!("{:.1}", completion).as_str());
            }
            
            ret.push_str("% done");
        }
        ret
    }
}

impl TurningGrilleCrackerSyncless
{
    pub fn new() -> Self
    {
        Self
        {
            workersCount: AtomicUsize::new(0)
        }
    }
    
    fn startWorkerThreads(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker<SynclessMilestoneDetails>>, workerCount: usize) -> Vec<JoinHandle<()>>
    {
        let mut workerThreads: Vec<JoinHandle<()>> = Vec::with_capacity(workerCount);
        
        let mut crackerUnsync = cracker.milestoneReportMutex.lock().unwrap();
        crackerUnsync.milestoneState.grilleIntervalsCompletion.reserve_exact(workerCount);
        
        let mut nextIntervalBegin: u64 = 0;
        let intervalLength: u64 = (cracker.grilleCount as f64 / workerCount as f64).round() as u64;
        for i in 0..workerCount
        {
            self.workersCount.fetch_add(1, Ordering::SeqCst);
            
            let nextIntervalEnd: u64 = if i < workerCount - 1 { nextIntervalBegin + intervalLength } else { cracker.grilleCount };
            let mut grilleInterval: GrilleInterval = GrilleInterval::new(cracker.sideLength / 2, nextIntervalBegin, nextIntervalEnd);
            
            let processedGrillsCount: Arc<AtomicU64> = Arc::new(AtomicU64::new(0));
            crackerUnsync.milestoneState.grilleIntervalsCompletion.push((processedGrillsCount.clone(), (nextIntervalEnd - nextIntervalBegin)));
                
            let cracker: Arc<TurningGrilleCracker<SynclessMilestoneDetails>> = cracker.clone();
            let self_: Arc<Self> = self.clone();
            workerThreads.push(thread::spawn(move ||
                {
                    loop
                    {
                        let grille: Option<&Grille> = grilleInterval.getNext();
                        let grilleCountSoFar:u64 = match grille
                        {
                            None =>
                            {
                                break;    
                            }
                            Some(grille) =>
                            {
                                cracker.applyGrille(grille)
                            }
                        };
                        cracker.registerOneAppliedGrill(grilleCountSoFar);
                        
                        processedGrillsCount.fetch_add(1, Ordering::SeqCst);
                    }
                    self_.workersCount.fetch_sub(1, Ordering::SeqCst);
                }));
                
            nextIntervalBegin += intervalLength;
        }
        
        workerThreads
    }
}



pub struct SerialMilestoneDetails
{
}

pub struct TurningGrilleCrackerSerial
{
}

impl TurningGrilleCrackerImplDetails<SerialMilestoneDetails> for TurningGrilleCrackerSerial
{
    fn takeMilestoneState(&self) -> SerialMilestoneDetails
    {
        SerialMilestoneDetails
        {
        }
    }

    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker<SerialMilestoneDetails>>)
    {
        let mut grilleInterval: GrilleInterval = GrilleInterval::new(cracker.sideLength / 2, 0, cracker.grilleCount);
        let mut milestoneStart: Instant = Instant::now();
        let mut grilleCountAtMilestoneStart:u64 = 0;
        let mut bestGrillesPerSecond: u64 = 0;
        
        loop
        {
            let grille: Option<&Grille> = grilleInterval.getNext();
            let grilleCountSoFar:u64 = match grille
            {
                None =>
                {
                    break;    
                }
                Some(grille) =>
                {
                    cracker.applyGrille(grille)
                }
            };
            
            if grilleCountSoFar % (cracker.grilleCount / 1000) == 0   // A milestone every 0.1%.
            {
                let milestoneEnd: Instant = Instant::now();
                let elapsedTime: Duration = milestoneEnd.duration_since(milestoneStart);
                let elapsedTimeNs:u128 = elapsedTime.as_nanos();
                if elapsedTimeNs > 0
                {
                    let grilleCountForMilestone: u64 = grilleCountSoFar - grilleCountAtMilestoneStart;
                    let grillesPerSecond: u64 = (grilleCountForMilestone as u128 * Duration::from_secs(1).as_nanos() / elapsedTimeNs) as u64;
                    if grillesPerSecond > bestGrillesPerSecond
                    {
                        bestGrillesPerSecond = grillesPerSecond;
                    }
                    
                    if cracker.VERBOSE
                    {
                        let done: f32 = grilleCountSoFar as f32 * 100.0 / cracker.grilleCount as f32;

                        eprint!("{:.1}% done; ", done);
                        eprint!("current speed: {} grilles/s; ", grillesPerSecond);
                        eprint!("best speed so far: {} grilles/s", bestGrillesPerSecond);
                        eprintln!();
                    }
                    
                    milestoneStart = milestoneEnd;
                    grilleCountAtMilestoneStart = grilleCountSoFar;
                }                
            }
        }
        {
            let mut crackerUnsync = cracker.milestoneReportMutex.lock().unwrap();
            crackerUnsync.bestGrillesPerSecond = bestGrillesPerSecond;
        }
    }
}

impl TurningGrilleCrackerSerial
{
    pub fn new() -> Self
    {
        Self
        {
        }
    }
}
