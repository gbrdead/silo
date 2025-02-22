use super::MPMC_PortionQueue;

use std::collections::VecDeque;
use std::sync::atomic::AtomicUsize;
use std::sync::Mutex;
use std::sync::Condvar;
use std::sync::atomic::Ordering;


struct TextbookPortionQueueUnsync<E>
{
    queue: VecDeque<E>,
    workDone: bool
}

pub struct TextbookPortionQueue<E>
{
    maxSize: usize,

    mutex: Mutex<TextbookPortionQueueUnsync<E>>,
    notEmptyCondition: Condvar,
    notFullCondition: Condvar,
    blockedProducers: AtomicUsize,
    blockedConsumers: AtomicUsize
}

impl<E> TextbookPortionQueue<E>
{
    pub fn new(initialConsumerCount: usize, producerCount: usize) -> Self
    {
        let maxSize: usize = initialConsumerCount * producerCount * 1000;
        Self
        {
            maxSize: maxSize,
            mutex: Mutex::new(TextbookPortionQueueUnsync
                {
                    queue: VecDeque::with_capacity(maxSize),
                    workDone: false
                }),
            notEmptyCondition: Condvar::new(),
            notFullCondition: Condvar::new(),
            blockedProducers: AtomicUsize::new(0),
            blockedConsumers: AtomicUsize::new(0)
        }
    }
}

impl<E: Send + Sync> MPMC_PortionQueue<E> for TextbookPortionQueue<E>
{
    fn addPortion(&self, portion: E)
    {
        self.blockedProducers.fetch_add(1, Ordering::Relaxed);
        let mut selfUnsync = self.mutex.lock().unwrap();
        
        while selfUnsync.queue.len() >= self.maxSize
        {
            selfUnsync = self.notFullCondition.wait(selfUnsync).unwrap();
        }
        self.blockedProducers.fetch_sub(1, Ordering::Relaxed);
        
        selfUnsync.queue.push_back(portion);
        
        self.blockedProducers.fetch_add(1, Ordering::Relaxed);
        self.notEmptyCondition.notify_one();
        self.blockedProducers.fetch_sub(1, Ordering::Relaxed);
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        self.blockedConsumers.fetch_add(1, Ordering::Relaxed);
        let mut selfUnsync = self.mutex.lock().unwrap();
        
        while selfUnsync.queue.is_empty()
        {
            if selfUnsync.workDone
            {
                self.blockedConsumers.fetch_sub(1, Ordering::Relaxed);
                return None;
            }
            selfUnsync = self.notEmptyCondition.wait(selfUnsync).unwrap();
        }
        self.blockedConsumers.fetch_sub(1, Ordering::Relaxed);
        
        let portion: Option<E> = selfUnsync.queue.pop_front();
        
        self.blockedConsumers.fetch_add(1, Ordering::Relaxed);
        self.notFullCondition.notify_one();
        self.blockedConsumers.fetch_sub(1, Ordering::Relaxed);
        
        portion
    }
    
    fn ensureAllPortionsAreRetrieved(&self)
    {
        let mut selfUnsync = self.mutex.lock().unwrap();
        
        while !selfUnsync.queue.is_empty()
        {
            selfUnsync = self.notFullCondition.wait(selfUnsync).unwrap();
        }
    }
    
    fn stopConsumers(&self, _finalConsumerCount: usize)
    {
        let mut selfUnsync = self.mutex.lock().unwrap();

        selfUnsync.workDone = true;

        self.notEmptyCondition.notify_all();
    }
    
    fn getSize(&self) -> usize
    {
        let selfUnsync = self.mutex.lock().unwrap();
        selfUnsync.queue.len()
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
