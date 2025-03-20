use super::MPMC_PortionQueue;
use super::NonBlockingQueue;
use super::non_blocking_queue::ConcurrentPortionQueue;
use super::non_blocking_queue::AsyncMPMC_PortionQueue;

use std::sync::atomic::AtomicUsize;
use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;
use std::sync::Mutex;
use std::sync::Condvar;
use crossbeam::utils::CachePadded;


struct Size
{
    size: AtomicUsize,
    maxSize: usize
}

struct MutexCondition<O>
{
    mutex: Mutex<O>,
    condition: Condvar
}

pub struct MostlyNonBlockingPortionQueue<E>
{
    sizes: CachePadded<Size>,
    
    notFull: CachePadded<MutexCondition<bool>>,
    notEmpty: CachePadded<MutexCondition<bool>>,
    empty: CachePadded<MutexCondition<bool>>,
    
    aProducerIsWaiting: CachePadded<AtomicBool>,
    aConsumerIsWaiting: CachePadded<AtomicBool>,
    
    nonBlockingQueue: CachePadded<Box<dyn NonBlockingQueue<E>>>
}

impl<E: Send + Sync + 'static> MostlyNonBlockingPortionQueue<E>
{
    pub fn createConcurrentBlownQueue(maxSize: usize) -> Self
    {
        let nonBlockingQueue: Box<dyn NonBlockingQueue<E>> = Box::new(ConcurrentPortionQueue::<E>::new(maxSize));
        MostlyNonBlockingPortionQueue::new(maxSize, nonBlockingQueue)
    }
    
    pub fn createAsynMPMC_BlownQueue(maxSize: usize) -> Self
    {
        let nonBlockingQueue: Box<dyn NonBlockingQueue<E>> = Box::new(AsyncMPMC_PortionQueue::<E>::new(maxSize));
        MostlyNonBlockingPortionQueue::new(maxSize, nonBlockingQueue)
    }

    
    pub fn new(maxSize: usize, nonBlockingQueue: Box<dyn NonBlockingQueue<E>>) -> Self
    {
        Self
        {
            sizes: CachePadded::new(
                Size
                {
                    size: AtomicUsize::new(0),
                    maxSize: maxSize
                }),
            notFull: CachePadded::new(
                MutexCondition::<bool>
                {
                    mutex: Mutex::new(false),
                    condition: Condvar::new()
                }),
            notEmpty: CachePadded::new(
                MutexCondition::<bool>
                {
                    mutex: Mutex::new(false),
                    condition: Condvar::new()
                }),
            empty: CachePadded::new(
                MutexCondition::<bool>
                {
                    mutex: Mutex::new(false),
                    condition: Condvar::new()
                }),
            aProducerIsWaiting: CachePadded::new(AtomicBool::new(false)),
            aConsumerIsWaiting: CachePadded::new(AtomicBool::new(false)),
            nonBlockingQueue: CachePadded::new(nonBlockingQueue)
        }
    }
}

impl<E> MPMC_PortionQueue<E> for MostlyNonBlockingPortionQueue<E>
{
    fn addPortion(&self, mut portion: E)
    {
        loop
        {
            if self.sizes.size.load(Ordering::Acquire) >= self.sizes.maxSize
            {
                let mut lock = self.notFull.mutex.lock().unwrap();
                while self.sizes.size.load(Ordering::Acquire) >= self.sizes.maxSize
                {
                    self.aProducerIsWaiting.store(true, Ordering::Release);
                    lock = self.notFull.condition.wait(lock).unwrap();
                }
            }
            
            match self.nonBlockingQueue.tryEnqueue(portion)
            {
                Ok(_) => break,
                Err(error) => portion = error
            }
        }
        self.sizes.size.fetch_add(1, Ordering::Release);
        
        if self.aConsumerIsWaiting.compare_exchange(true, false, Ordering::AcqRel, Ordering::Relaxed).is_ok()
        {
            let _lock = self.notEmpty.mutex.lock().unwrap();
            self.notEmpty.condition.notify_all();
        }
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        let mut portion: Option<E> = self.nonBlockingQueue.tryDequeue();
        
        if portion.is_none()
        {
            let mut workDone = self.notEmpty.mutex.lock().unwrap();
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
                workDone = self.notEmpty.condition.wait(workDone).unwrap();
            }
        }
        
        let newSize: usize = self.sizes.size.fetch_sub(1, Ordering::AcqRel) - 1;
        
        if self.aProducerIsWaiting.compare_exchange(true, false, Ordering::AcqRel, Ordering::Relaxed).is_ok()
        {
            let _lock = self.notFull.mutex.lock().unwrap();
            self.notFull.condition.notify_all();
        }
        
        if newSize == 0
        {
            let _lock = self.empty.mutex.lock().unwrap();
            self.empty.condition.notify_one();
        }
        
        portion
    }
    
    fn ensureAllPortionsAreRetrieved(&self)
    {
        {
            let _lock = self.notEmpty.mutex.lock().unwrap();
            self.notEmpty.condition.notify_all();
        }

        {
            let mut lock = self.empty.mutex.lock().unwrap();
            while self.sizes.size.load(Ordering::Acquire) > 0
            {
                lock = self.empty.condition.wait(lock).unwrap();
            }
        }        
    }
    
    fn stopConsumers(&self, _finalConsumerCount: usize)
    {
        let mut workDone = self.notEmpty.mutex.lock().unwrap();
        *workDone = true;
        self.notEmpty.condition.notify_all();
    }
    
    fn getSize(&self) -> usize
    {
        self.sizes.size.load(Ordering::Relaxed)
    }
    
    fn getMaxSize(&self) -> usize
    {
        self.sizes.maxSize
    }
}
