mod words_trie;
mod grille;

pub use grille::Grille;
use grille::GrilleInterval;
use words_trie::WordsTrie;
use super::VERBOSE;
use super::queue::MPMC_PortionQueue;

use std::vec::Vec;
use std::collections::BTreeSet;
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
use std::string::String;
use std::sync::LazyLock;
use regex::Regex;
use concurrent_queue::ConcurrentQueue;


pub static NOT_CAPITAL_ENGLISH_LETTERS_RE: LazyLock<Regex> = LazyLock::new(||
    {
        Regex::new(r"[^A-Z]").unwrap()
    });


#[derive(Clone)]
pub struct TurningGrilleCrackerMilestoneState
{
    start: Instant,
    milestoneStart: Instant,
    grilleCountAtMilestoneStart: u64,
    bestGrillesPerSecond: u64
}

impl TurningGrilleCrackerMilestoneState
{
    pub fn new() -> Self
    {
        Self
        {
            start: Instant::now(),
            milestoneStart: Instant::now(),
            grilleCountAtMilestoneStart: 0,
            bestGrillesPerSecond: 0
        }
    }
}

pub trait TurningGrilleCrackerImplDetails : Send + Sync
{
    fn modifyCrackerMilestoneState(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>,
        accessor: fn(cracker: &Arc<TurningGrilleCracker>, crackerMilestoneState: &mut TurningGrilleCrackerMilestoneState));
    fn cloneCrackerMilestoneState(self: Arc<Self>) -> TurningGrilleCrackerMilestoneState;
        
    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>);
    
    fn tryMilestone(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>, milestoneEnd: Instant, grilleCountSoFar: u64);
                
    fn milestonesSummary(self: Arc<Self>, _cracker: &Arc<TurningGrilleCracker>) -> String
    {
        String::from("")
    }
}

pub struct TurningGrilleCracker
{
    VERBOSE: bool,
    
    sideLength: usize,
    grilleCount: u64,
    grilleCountSoFar: AtomicU64,

    cipherText: Vec<char>,
    wordsTrie: Arc<WordsTrie>,
    candidates: Mutex<Option<BTreeSet<String>>>,
    
    implDetails: Arc<dyn TurningGrilleCrackerImplDetails>
}

impl TurningGrilleCracker
{
    const WORDS_FILE_PATH: &'static str = "3000words.txt";
    const MIN_DETECTED_WORD_COUNT: usize = 17;  // Determined by gut feeling.
    
    
    pub fn new(cipherText: &str, implDetails: Box<dyn TurningGrilleCrackerImplDetails>) -> Self
    {
        let cipherText: String = cipherText.to_uppercase();
        if NOT_CAPITAL_ENGLISH_LETTERS_RE.is_match(&cipherText)
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
            wordsTrie: Arc::new(WordsTrie::new(TurningGrilleCracker::WORDS_FILE_PATH)),
            candidates: Mutex::new(None),
            implDetails: Arc::from(implDetails)
        }
    }
    
    pub fn bruteForce(self: &Arc<Self>) -> BTreeSet<String>
    {
        self.candidates.lock().unwrap().replace(BTreeSet::new());
        self.implDetails.clone().modifyCrackerMilestoneState(self,
            | _, milestoneState: &mut TurningGrilleCrackerMilestoneState |
            {
                milestoneState.start = Instant::now();
                milestoneState.milestoneStart = milestoneState.start;
            });
            
        self.implDetails.clone().bruteForce(self);
        
        let end: Instant = Instant::now();
        let milestoneState: TurningGrilleCrackerMilestoneState = self.implDetails.clone().cloneCrackerMilestoneState();
        let elapsedTimeNs: u128 = end.duration_since(milestoneState.start).as_nanos();
        let grillsPerSecond: u64 = ((self.grilleCount as u128 * Duration::from_secs(1).as_nanos()) / elapsedTimeNs) as u64;
        
        eprint!("Average speed: {} grilles/s; best speed: {} grilles/s", grillsPerSecond, milestoneState.bestGrillesPerSecond);
        let summary: String = self.implDetails.clone().milestonesSummary(self);
        if !summary.is_empty()
        {
            eprint!("; {}", summary);
        }
        eprintln!();
        
        if self.grilleCountSoFar.load(Ordering::Acquire) != self.grilleCount
        {
            panic!("Some grilles got lost.");
        }
        
        self.candidates.lock().unwrap().take().unwrap()
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
        
        self.grilleCountSoFar.fetch_add(1, Ordering::Release) + 1
    }

    fn registerOneAppliedGrill(self: &Arc<Self>, grilleCountSoFar:u64)
    {
        if grilleCountSoFar % (self.grilleCount / 1000) == 0   // A milestone every 0.1%.
        {
            let milestoneEnd: Instant = Instant::now();
            self.implDetails.clone().tryMilestone(self, milestoneEnd, grilleCountSoFar);
        }
    }

    fn milestone(self: &Arc<TurningGrilleCracker>, milestoneEnd: Instant, grilleCountSoFar: u64, milestoneState: &mut TurningGrilleCrackerMilestoneState, milestoneDetailsStatus: String)
        -> Option<u64>
    {
        let elapsedTime: Duration = milestoneEnd.duration_since(milestoneState.milestoneStart);
        let elapsedTimeNs:u128 = elapsedTime.as_nanos();
        if elapsedTimeNs > 0
        {
            let grilleCountForMilestone: u64 = grilleCountSoFar - milestoneState.grilleCountAtMilestoneStart;
            let grillesPerSecond: u64 = (grilleCountForMilestone as u128 * Duration::from_secs(1).as_nanos() / elapsedTimeNs) as u64;
            if grillesPerSecond > milestoneState.bestGrillesPerSecond
            {
                milestoneState.bestGrillesPerSecond = grillesPerSecond;
            }
            
            if self.VERBOSE
            {
                let done: f32 = grilleCountSoFar as f32 * 100.0 / self.grilleCount as f32;

                eprint!("{:.1}% done; ", done);
                eprint!("current speed: {} grilles/s; ", grillesPerSecond);
                eprint!("best speed so far: {} grilles/s", milestoneState.bestGrillesPerSecond);
                if !milestoneDetailsStatus.is_empty()
                {
                    eprint!("; {}", milestoneDetailsStatus);
                }
                eprintln!();
            }
            
            milestoneState.milestoneStart = milestoneEnd;
            milestoneState.grilleCountAtMilestoneStart = grilleCountSoFar;
            
            Some(grillesPerSecond)
        }
        else
        {
            None
        }
    }
    
    fn findWordsAndReport(&self, candidate: &str)
    {
        let wordsFound: usize = self.wordsTrie.countWords(candidate);
        if wordsFound >= TurningGrilleCracker::MIN_DETECTED_WORD_COUNT
        {
            self.candidates.lock().unwrap().as_mut().unwrap().insert(candidate.to_string());
            if self.VERBOSE
            {
                println!("{}: {}", wordsFound, candidate);
            }
        }        
    }
}



struct ProducerConsumerMilestoneState
{
    improving: isize,
    addingThreads: bool,
    prevGrillesPerSecond: u64,
    bestConsumerCount: usize,
    bestGrillesPerSecond: u64,
    crackerMilestoneState: TurningGrilleCrackerMilestoneState
}

impl ProducerConsumerMilestoneState
{
    pub fn new() -> Self
    {
        Self
        {
            improving: 0,
            addingThreads: true,
            prevGrillesPerSecond: 0,
            bestConsumerCount: 0,
            bestGrillesPerSecond: 0,
            crackerMilestoneState: TurningGrilleCrackerMilestoneState::new()
        }
    }
}

pub struct TurningGrilleCrackerProducerConsumer
{
    initialConsumerCount: usize,
    producerCount: usize,
    portionQueue: Arc<dyn MPMC_PortionQueue<Grille>>,

    consumerCount: AtomicIsize,         // This may get negative for a short while, so don't make it unsigned.
    consumerThreads: ConcurrentQueue<JoinHandle<()>>,
    shutdownNConsumers: AtomicIsize,     // This may get negative for a short while, so don't make it unsigned.
    milestoneStateMutex: Mutex<ProducerConsumerMilestoneState>
}

impl TurningGrilleCrackerImplDetails for TurningGrilleCrackerProducerConsumer
{
    fn modifyCrackerMilestoneState(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>,
            accessor: fn(cracker: &Arc<TurningGrilleCracker>, crackerMilestoneState: &mut TurningGrilleCrackerMilestoneState))
    {
        let mut milestoneLock = self.milestoneStateMutex.lock().unwrap();
        accessor(cracker, &mut milestoneLock.crackerMilestoneState);
    }
    
    fn cloneCrackerMilestoneState(self: Arc<Self>) -> TurningGrilleCrackerMilestoneState
    {
        let milestoneLock = self.milestoneStateMutex.lock().unwrap();
        milestoneLock.crackerMilestoneState.clone()
    }
    
    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>)
    {
        let producerThreads: Vec<JoinHandle<()>> = self.startProducerThreads(cracker);
        self.startInitialConsumerThreads(cracker);
        
        for producerThread in &mut producerThreads.into_iter()
        {
            let _ = producerThread.join().unwrap();
        }
        
        self.portionQueue.ensureAllPortionsAreRetrieved();
        while cracker.grilleCountSoFar.load(Ordering::Acquire) < cracker.grilleCount // Ensures all work is done and no more consumers will be started or stopped.
        {
        }
        self.portionQueue.stopConsumers(self.consumerCount.load(Ordering::Acquire) as usize);

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
    
    fn tryMilestone(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>, milestoneEnd: Instant, grilleCountSoFar: u64)
    {
        let milestoneTryLock = self.milestoneStateMutex.try_lock();
        if milestoneTryLock.is_ok()
        {
            let mut milestoneState = milestoneTryLock.unwrap();
            
            let queueSize: usize = self.portionQueue.getSize();
            
            let mut milestoneStatus: String = String::new();
            if cracker.VERBOSE
            {
                milestoneStatus.push_str("consumers: ");
                milestoneStatus.push_str(self.consumerCount.load(Ordering::Relaxed).to_string().as_str());
                milestoneStatus.push_str("; queue size: ");
                milestoneStatus.push_str(queueSize.to_string().as_str());
                milestoneStatus.push_str("/");
                milestoneStatus.push_str(self.portionQueue.getMaxSize().to_string().as_str());
            }
            
            let grillesPerSecond: Option<u64> =
                cracker.milestone(milestoneEnd, grilleCountSoFar, &mut milestoneState.crackerMilestoneState, milestoneStatus);
            let grillesPerSecond: u64 =
                match grillesPerSecond
                {
                    None =>
                    {
                        return;
                    }
                    Some(grillesPerSecond) =>
                    {
                        grillesPerSecond
                    }
                };
            
            if grillesPerSecond > milestoneState.bestGrillesPerSecond
            {
                milestoneState.bestGrillesPerSecond = grillesPerSecond;
                milestoneState.bestConsumerCount = self.consumerCount.load(Ordering::Relaxed) as usize;
            }
            
            if grilleCountSoFar < cracker.grilleCount
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
                        self.shutdownNConsumers.fetch_add(1, Ordering::Release);
                    }
                }
    
                milestoneState.prevGrillesPerSecond = grillesPerSecond;
            }
        }
    }
    
    fn milestonesSummary(self: Arc<Self>, _cracker: &Arc<TurningGrilleCracker>) -> String
    {
        let milestoneState = self.milestoneStateMutex.lock().unwrap();
        let mut ret: String = String::from("best consumer count: ");
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
            shutdownNConsumers: AtomicIsize::new(0),
            
            milestoneStateMutex: Mutex::new(ProducerConsumerMilestoneState::new())
        }
    }
    
    fn startProducerThreads(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker>) -> Vec<JoinHandle<()>>
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
    
    fn startInitialConsumerThreads(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker>)
    {
        for _i in 0..self.initialConsumerCount
        {
            let _ = self.consumerThreads.push(self.startConsumerThread(cracker));
        }
    }
    
    fn startConsumerThread(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker>) -> JoinHandle<()>
    {
        self.consumerCount.fetch_add(1, Ordering::Release);
        
        let portionQueue: Arc<dyn MPMC_PortionQueue<Grille>> = self.portionQueue.clone();
        let cracker: Arc<TurningGrilleCracker> = cracker.clone();
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
                            self_.consumerCount.fetch_sub(1, Ordering::Release);
                            break;    
                        }
                        Some(grille) =>
                        {
                            cracker.applyGrille(&grille)
                        }
                    };
                    cracker.registerOneAppliedGrill(grilleCountSoFar);
                    
                    if self_.shutdownNConsumers.load(Ordering::Relaxed) > 0
                    {
                        if self_.shutdownNConsumers.fetch_sub(1, Ordering::AcqRel) > 0
                        {
                            if self_.consumerCount.fetch_sub(1, Ordering::AcqRel) > 1   // There should be at least one consumer running.
                            {
                                break;
                            }
                            else
                            {
                                self_.consumerCount.fetch_add(1, Ordering::Release);
                            }
                        }
                        else
                        {
                            self_.shutdownNConsumers.fetch_add(1, Ordering::Relaxed);
                        }
                    }
                }
            })
    }
}



struct SynclessMilestoneState
{
    grilleIntervalsCompletion: Vec<(Arc<AtomicU64>, u64)>,
    crackerMilestoneState: TurningGrilleCrackerMilestoneState
}

impl SynclessMilestoneState
{
    pub fn new() -> Self
    {
        Self
        {
            grilleIntervalsCompletion: Vec::<(Arc<AtomicU64>, u64)>::new(),
            crackerMilestoneState: TurningGrilleCrackerMilestoneState::new()
        }
    }
}

pub struct TurningGrilleCrackerSyncless
{
    workersCount: AtomicUsize,
    milestoneStateMutex: Mutex<SynclessMilestoneState>
}

impl TurningGrilleCrackerImplDetails for TurningGrilleCrackerSyncless
{
    fn modifyCrackerMilestoneState(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>,
            accessor: fn(cracker: &Arc<TurningGrilleCracker>, crackerMilestoneState: &mut TurningGrilleCrackerMilestoneState))
    {
        let mut milestoneLock = self.milestoneStateMutex.lock().unwrap();
        accessor(cracker, &mut milestoneLock.crackerMilestoneState);
    }

    fn cloneCrackerMilestoneState(self: Arc<Self>) -> TurningGrilleCrackerMilestoneState
    {
        let milestoneLock = self.milestoneStateMutex.lock().unwrap();
        milestoneLock.crackerMilestoneState.clone()
    }

    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>)
    {
        let cpuCount: usize = available_parallelism().unwrap().get();
        
        let workerThreads: Vec<JoinHandle<()>> = self.startWorkerThreads(cracker, cpuCount);

        for workerThread in &mut workerThreads.into_iter()
        {
            let _ = workerThread.join().unwrap();
        }
    }
    
    fn tryMilestone(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>, milestoneEnd: Instant, grilleCountSoFar: u64)
    {
        let milestoneTryLock = self.milestoneStateMutex.try_lock();
        if milestoneTryLock.is_ok()
        {
            let mut milestoneState = milestoneTryLock.unwrap();
            
            let mut milestoneStatus: String = String::new();        
            if cracker.VERBOSE
            {
                milestoneStatus.push_str("workers: ");
                milestoneStatus.push_str(self.workersCount.load(Ordering::Relaxed).to_string().as_str());
                milestoneStatus.push_str("; completion per worker: ");

                let mut first: bool = true;
                for (processedGrillsCount, totalGrillsCount) in &milestoneState.grilleIntervalsCompletion
                {
                    if !first
                    {
                        milestoneStatus.push_str("/");
                    }
                    else
                    {
                        first = false;
                    }
                    
                    let processedGrillsCount: u64 = processedGrillsCount.load(Ordering::Relaxed);
                    let completion: f32 = processedGrillsCount as f32 * 100.0 / *totalGrillsCount as f32;
                    milestoneStatus.push_str(format!("{:.1}", completion).as_str());
                }

                milestoneStatus.push_str("% done");
            }
            
            cracker.milestone(milestoneEnd, grilleCountSoFar, &mut milestoneState.crackerMilestoneState, milestoneStatus);
        }
    }
}

impl TurningGrilleCrackerSyncless
{
    pub fn new() -> Self
    {
        Self
        {
            workersCount: AtomicUsize::new(0),
            milestoneStateMutex: Mutex::new(SynclessMilestoneState::new())
        }
    }
    
    fn startWorkerThreads(self: &Arc<Self>, cracker: &Arc<TurningGrilleCracker>, workerCount: usize) -> Vec<JoinHandle<()>>
    {
        let mut workerThreads: Vec<JoinHandle<()>> = Vec::with_capacity(workerCount);
        
        let mut milestoneLock = self.milestoneStateMutex.lock().unwrap();
        milestoneLock.grilleIntervalsCompletion.reserve_exact(workerCount);
        
        let mut nextIntervalBegin: u64 = 0;
        let intervalLength: u64 = (cracker.grilleCount as f64 / workerCount as f64).round() as u64;
        for i in 0..workerCount
        {
            self.workersCount.fetch_add(1, Ordering::Relaxed);
            
            let nextIntervalEnd: u64 = if i < workerCount - 1 { nextIntervalBegin + intervalLength } else { cracker.grilleCount };
            let mut grilleInterval: GrilleInterval = GrilleInterval::new(cracker.sideLength / 2, nextIntervalBegin, nextIntervalEnd);
            
            let processedGrillsCount: Arc<AtomicU64> = Arc::new(AtomicU64::new(0));
            milestoneLock.grilleIntervalsCompletion.push((processedGrillsCount.clone(), (nextIntervalEnd - nextIntervalBegin)));
                
            let cracker: Arc<TurningGrilleCracker> = cracker.clone();
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
                        
                        processedGrillsCount.fetch_add(1, Ordering::Relaxed);
                    }
                    self_.workersCount.fetch_sub(1, Ordering::Relaxed);
                }));
                
            nextIntervalBegin += intervalLength;
        }
        
        workerThreads
    }
}



struct SerialMilestoneState
{
    crackerMilestoneState: TurningGrilleCrackerMilestoneState
}

impl SerialMilestoneState
{
    pub fn new() -> Self
    {
        Self
        {
            crackerMilestoneState: TurningGrilleCrackerMilestoneState::new()
        }
    }
}

pub struct TurningGrilleCrackerSerial
{
    milestoneStateMutex: Mutex<SerialMilestoneState>
}

impl TurningGrilleCrackerImplDetails for TurningGrilleCrackerSerial
{
    fn modifyCrackerMilestoneState(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>,
            accessor: fn(cracker: &Arc<TurningGrilleCracker>, crackerMilestoneState: &mut TurningGrilleCrackerMilestoneState))
    {
        let mut milestoneLock = self.milestoneStateMutex.lock().unwrap();
        accessor(cracker, &mut milestoneLock.crackerMilestoneState);
    }

    fn cloneCrackerMilestoneState(self: Arc<Self>) -> TurningGrilleCrackerMilestoneState
    {
        let milestoneLock = self.milestoneStateMutex.lock().unwrap();
        milestoneLock.crackerMilestoneState.clone()
    }

    fn bruteForce(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>)
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
            let mut milestoneLock = self.milestoneStateMutex.lock().unwrap();
            milestoneLock.crackerMilestoneState.bestGrillesPerSecond = bestGrillesPerSecond;
        }
    }

    fn tryMilestone(self: Arc<Self>, cracker: &Arc<TurningGrilleCracker>, milestoneEnd: Instant, grilleCountSoFar: u64)
    {
        let milestoneTryLock = self.milestoneStateMutex.try_lock();
        if milestoneTryLock.is_ok()
        {
            let mut milestoneState = milestoneTryLock.unwrap();
            cracker.milestone(milestoneEnd, grilleCountSoFar, &mut milestoneState.crackerMilestoneState, "".to_string());
        }
    }
}

impl TurningGrilleCrackerSerial
{
    pub fn new() -> Self
    {
        Self
        {
            milestoneStateMutex: Mutex::new(SerialMilestoneState::new())
        }
    }
}
