use super::MPMC_PortionQueue;
use super::NonBlockingQueue;

use std::sync::atomic::AtomicUsize;
use std::sync::atomic::Ordering;
use std::sync::Mutex;
use std::sync::Condvar;


pub struct MostlyNonBlockingPortionQueue<E>
{
    nonBlockingQueue: Box<dyn NonBlockingQueue<E>>,
    
    maxSize: usize,
    size: AtomicUsize,
    
    notFullMutex: Mutex<bool>,
    notEmptyMutex: Mutex<bool>,
    emptyMutex: Mutex<bool>,
    notFullCondition: Condvar,
    notEmptyCondition: Condvar,
    emptyCondition: Condvar,
    blockedProducers: AtomicUsize,
    blockedConsumers: AtomicUsize
}

impl<E> MostlyNonBlockingPortionQueue<E>
{
    pub fn new(initialConsumerCount: usize, producerCount: usize, nonBlockingQueue: Box<dyn NonBlockingQueue<E>>) -> Self
    {
        let mut ret: Self = Self
        {
            nonBlockingQueue: nonBlockingQueue,
            maxSize: initialConsumerCount * producerCount * 1000,
            size: AtomicUsize::new(0),
            notFullMutex: Mutex::new(false),
            notEmptyMutex: Mutex::new(false),
            emptyMutex: Mutex::new(false),
            notFullCondition: Condvar::new(),
            notEmptyCondition: Condvar::new(),
            emptyCondition: Condvar::new(),
            blockedProducers: AtomicUsize::new(0),
            blockedConsumers: AtomicUsize::new(0)
        };
        ret.nonBlockingQueue.setSizeParameters(producerCount, ret.maxSize);
        ret
    }
}

impl<E> MPMC_PortionQueue<E> for MostlyNonBlockingPortionQueue<E>
{
    fn addPortion(&self, mut portion: E)
    {
        let newSize: usize = self.size.fetch_add(1, Ordering::SeqCst) + 1;
        
        loop
        {
            if self.size.load(Ordering::SeqCst) >= self.maxSize
            {
                self.blockedProducers.fetch_add(1, Ordering::Relaxed);
                let mut lock = self.notFullMutex.lock().unwrap();
                while self.size.load(Ordering::SeqCst) >= self.maxSize
                {
                    lock = self.notFullCondition.wait(lock).unwrap();
                }
                self.blockedProducers.fetch_sub(1, Ordering::Relaxed);
            }
            
            match self.nonBlockingQueue.tryEnqueue(portion)
            {
                Ok(_) => break,
                Err(error) => portion = error
            }
        }
        
        if newSize == self.maxSize * 1 / 4
        {
            self.blockedProducers.fetch_add(1, Ordering::Relaxed);
            let _lock = self.notEmptyMutex.lock().unwrap();
            self.notEmptyCondition.notify_all();
            self.blockedProducers.fetch_sub(1, Ordering::Relaxed);
        }
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        let mut portion: Option<E> = self.nonBlockingQueue.tryDequeue();
        
        if portion.is_none()
        {
            self.blockedConsumers.fetch_add(1, Ordering::Relaxed);
            let mut workDone = self.notEmptyMutex.lock().unwrap();
            loop
            {
                if *workDone
                {
                    self.blockedConsumers.fetch_sub(1, Ordering::Relaxed);
                    return None;
                }
                portion = self.nonBlockingQueue.tryDequeue();
                if portion.is_some()
                {
                    break;
                }
                workDone = self.notEmptyCondition.wait(workDone).unwrap();
            }
            self.blockedConsumers.fetch_sub(1, Ordering::Relaxed);
        }
        
        let newSize: usize = self.size.fetch_sub(1, Ordering::SeqCst) - 1;
        if newSize == self.maxSize * 3 / 4
        {
            self.blockedConsumers.fetch_add(1, Ordering::Relaxed);
            let _lock = self.notFullMutex.lock().unwrap();
            self.notFullCondition.notify_all();
            self.blockedConsumers.fetch_sub(1, Ordering::Relaxed);
        }
        if newSize == 0
        {
            self.blockedConsumers.fetch_add(1, Ordering::Relaxed);
            let _lock = self.emptyMutex.lock().unwrap();
            self.emptyCondition.notify_one();
            self.blockedConsumers.fetch_sub(1, Ordering::Relaxed);
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
            while self.size.load(Ordering::SeqCst) > 0
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

    fn getBlockedProducers(&self) -> usize
    {
        self.blockedProducers.load(Ordering::Relaxed)
    }
    
    fn getBlockedConsumers(&self) -> usize
    {
        self.blockedConsumers.load(Ordering::Relaxed)
    }
}
