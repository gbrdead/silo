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
use std::sync::MutexGuard;


struct Size
{
    size: AtomicUsize,
    maxSize: usize
}

pub struct MostlyNonBlockingPortionQueue<E>
{
    sizes: CachePadded<Size>,
    
    mutex: CachePadded<Mutex<bool>>,
    notFullCondition: CachePadded<Condvar>,
    notEmptyCondition: CachePadded<Condvar>,
    emptyCondition: CachePadded<Condvar>,
    
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
            mutex: CachePadded::new(Mutex::new(false)),
            notFullCondition: CachePadded::new(Condvar::new()),
            notEmptyCondition: CachePadded::new(Condvar::new()),
            emptyCondition: CachePadded::new(Condvar::new()),
            aProducerIsWaiting: CachePadded::new(AtomicBool::new(false)),
            aConsumerIsWaiting: CachePadded::new(AtomicBool::new(false)),
            nonBlockingQueue: CachePadded::new(nonBlockingQueue)
        }
    }
    
    
    fn lockMutexIfNecessary<'a>(&'a self, lock: &mut Option<MutexGuard<'a, bool>>)
    {
        if lock.is_none()
        {
            let _ = lock.insert(self.mutex.lock().unwrap());
        }
    }
    
    fn waitOnCondition(cond: &Condvar, lock: &mut Option<MutexGuard<'_, bool>>)
    {
        let theLock : Option<MutexGuard<'_, bool>> = lock.take();
        let _ = lock.insert(cond.wait(theLock.unwrap()).unwrap());
    }
}

impl<E: Send + Sync + 'static> MPMC_PortionQueue<E> for MostlyNonBlockingPortionQueue<E>
{
    fn addPortion(&self, mut portion: E)
    {
        let mut lock: Option<MutexGuard<'_, bool>> = None;
        
        loop
        {
            if self.sizes.size.load(Ordering::Acquire) >= self.sizes.maxSize
            {
                self.lockMutexIfNecessary(&mut lock);
                
                while self.sizes.size.load(Ordering::Acquire) >= self.sizes.maxSize
                {
                    if self.aConsumerIsWaiting.compare_exchange(true, false, Ordering::AcqRel, Ordering::Relaxed).is_ok()
                    {
                        self.notEmptyCondition.notify_all();
                    }

                    self.aProducerIsWaiting.store(true, Ordering::Release);
                    MostlyNonBlockingPortionQueue::<E>::waitOnCondition(&self.notFullCondition, &mut lock);
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
            self.lockMutexIfNecessary(&mut lock);
            self.notEmptyCondition.notify_all();
        }
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        let mut lock: Option<MutexGuard<'_, bool>> = None;
        
        let mut portion: Option<E> = self.nonBlockingQueue.tryDequeue();
        if portion.is_none()
        {
            self.lockMutexIfNecessary(&mut lock);
            loop
            {
                portion = self.nonBlockingQueue.tryDequeue();
                if portion.is_some()
                {
                    break;
                }

                let workDone: bool = **lock.as_ref().unwrap();
                if workDone
                {
                    return None;
                }
                
                if self.aProducerIsWaiting.compare_exchange(true, false, Ordering::AcqRel, Ordering::Relaxed).is_ok()
                {
                    self.notFullCondition.notify_all();
                }
                
                self.aConsumerIsWaiting.store(true, Ordering::Release);
                MostlyNonBlockingPortionQueue::<E>::waitOnCondition(&self.notEmptyCondition, &mut lock);
            }
        }
        
        let newSize: usize = self.sizes.size.fetch_sub(1, Ordering::AcqRel) - 1;
        
        if self.aProducerIsWaiting.compare_exchange(true, false, Ordering::AcqRel, Ordering::Relaxed).is_ok()
        {
            self.lockMutexIfNecessary(&mut lock);
            self.notFullCondition.notify_all();
        }
        
        if newSize == 0
        {
            self.lockMutexIfNecessary(&mut lock);
            self.emptyCondition.notify_one();
        }
        
        portion
    }
    
    fn ensureAllPortionsAreRetrieved(&self)
    {
        let mut lock = self.mutex.lock().unwrap();
        self.notEmptyCondition.notify_all();
        while self.sizes.size.load(Ordering::Acquire) > 0
        {
            lock = self.emptyCondition.wait(lock).unwrap();
        }
    }
    
    fn stopConsumers(&self, _finalConsumerCount: usize)
    {
        let mut workDone = self.mutex.lock().unwrap();
        *workDone = true;
        self.notEmptyCondition.notify_all();
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
