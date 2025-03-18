use super::MPMC_PortionQueue;
use super::NonBlockingQueue;

use crossbeam::utils::CachePadded;
use std::marker::PhantomData;
use std::marker::Send;
use std::marker::Sync;
use std::sync::atomic::AtomicUsize;
use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;
use std::sync::Mutex;
use std::sync::Condvar;

use super::non_blocking_queue::AsyncMpmcPortionQueue;
use super::non_blocking_queue::ConcurrentPortionQueue;

pub type AsyncMpmcBlownQueue<E> = MostlyNonBlockingPortionQueue::<E, AsyncMpmcPortionQueue<E>>;
pub type ConcurrentBlownQueue<E> = MostlyNonBlockingPortionQueue::<E, ConcurrentPortionQueue<E>>;


pub struct MostlyNonBlockingPortionQueue<E: Send + Sync, NBQ: NonBlockingQueue<E>>
{
    nonBlockingQueue: CachePadded<NBQ>,
    size: CachePadded<AtomicUsize>,
    
    notFullMutex: CachePadded<Mutex<bool>>,
    notFullCondition: CachePadded<Condvar>,
    notEmptyMutex: CachePadded<Mutex<bool>>,
    notEmptyCondition: CachePadded<Condvar>,
    emptyMutex: CachePadded<Mutex<bool>>,
    emptyCondition: CachePadded<Condvar>,
    
    aProducerIsWaiting: CachePadded<AtomicBool>,
    aConsumerIsWaiting: CachePadded<AtomicBool>,

    maxSize: usize,
    
    _t: PhantomData<E>
}

impl<E: Send + Sync, NBQ: NonBlockingQueue<E>> MostlyNonBlockingPortionQueue<E, NBQ>
{
    pub fn new(initialConsumerCount: usize, producerCount: usize) -> Self
    {
        let maxSize: usize = initialConsumerCount * producerCount * 1000;
        Self
        {
            _t: PhantomData,
            nonBlockingQueue: CachePadded::new(NBQ::new(maxSize)),
            maxSize: maxSize,
            size: CachePadded::new(AtomicUsize::new(0)),
            notFullMutex: CachePadded::new(Mutex::new(false)),
            notEmptyMutex: CachePadded::new(Mutex::new(false)),
            emptyMutex: CachePadded::new(Mutex::new(false)),
            notFullCondition: CachePadded::new(Condvar::new()),
            notEmptyCondition: CachePadded::new(Condvar::new()),
            emptyCondition: CachePadded::new(Condvar::new()),
            aProducerIsWaiting: CachePadded::new(AtomicBool::new(false)),
            aConsumerIsWaiting: CachePadded::new(AtomicBool::new(false))
        }
    }
}

impl<E: Send + Sync, NBQ: NonBlockingQueue<E>> MPMC_PortionQueue<E> for MostlyNonBlockingPortionQueue<E, NBQ>
{
    fn addPortion(&self, mut portion: E)
    {
        loop
        {
            if self.size.load(Ordering::Acquire) >= self.maxSize
            {
                let mut lock = self.notFullMutex.lock().unwrap();
                while self.size.load(Ordering::Acquire) >= self.maxSize
                {
                    self.aProducerIsWaiting.store(true, Ordering::Release);
                    lock = self.notFullCondition.wait(lock).unwrap();
                }
            }
            
            match self.nonBlockingQueue.tryEnqueue(portion)
            {
                Ok(_) => break,
                Err(error) => portion = error
            }
        }
        self.size.fetch_add(1, Ordering::Release);
        
        if self.aConsumerIsWaiting.compare_exchange(true, false, Ordering::AcqRel, Ordering::Relaxed).is_ok()
        {
            let _lock = self.notEmptyMutex.lock().unwrap();
            self.notEmptyCondition.notify_all();
        }
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        let mut portion: Option<E> = self.nonBlockingQueue.tryDequeue();
        
        if portion.is_none()
        {
            let mut workDone = self.notEmptyMutex.lock().unwrap();
            loop
            {
                if *workDone
                {
                    return None;
                }
                
                portion = self.nonBlockingQueue.tryDequeue();
                if portion.is_some()
                {
                    break;
                }
                
                self.aConsumerIsWaiting.store(true, Ordering::Release);
                workDone = self.notEmptyCondition.wait(workDone).unwrap();
            }
        }
        
        let newSize: usize = self.size.fetch_sub(1, Ordering::AcqRel) - 1;
        
        if self.aProducerIsWaiting.compare_exchange(true, false, Ordering::AcqRel, Ordering::Relaxed).is_ok()
        {
            let _lock = self.notFullMutex.lock().unwrap();
            self.notFullCondition.notify_all();
        }
        
        if newSize == 0
        {
            let _lock = self.emptyMutex.lock().unwrap();
            self.emptyCondition.notify_one();
        }
        
        portion
    }
    
    fn ensureAllPortionsAreRetrieved(&self)
    {
        {
            let _lock = self.notEmptyMutex.lock().unwrap();
            self.notEmptyCondition.notify_all();
        }

        {
            let mut lock = self.emptyMutex.lock().unwrap();
            while self.size.load(Ordering::Acquire) > 0
            {
                lock = self.emptyCondition.wait(lock).unwrap();
            }
        }        
    }
    
    fn stopConsumers(&self, _finalConsumerCount: usize)
    {
        let mut workDone = self.notEmptyMutex.lock().unwrap();
        *workDone = true;
        self.notEmptyCondition.notify_all();
    }
    
    fn getSize(&self) -> usize
    {
        self.size.load(Ordering::Relaxed)
    }
    
    fn getMaxSize(&self) -> usize
    {
        self.maxSize
    }
}
