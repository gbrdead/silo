use super::MPMC_PortionQueue;

use std::collections::VecDeque;
use std::sync::Mutex;
use std::sync::Condvar;


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
    notFullCondition: Condvar
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
            notFullCondition: Condvar::new()
        }
    }
}

impl<E: Send + Sync> MPMC_PortionQueue<E> for TextbookPortionQueue<E>
{
    fn addPortion(&self, portion: E)
    {
        let mut selfUnsync = self.mutex.lock().unwrap();
        
        while selfUnsync.queue.len() >= self.maxSize
        {
            selfUnsync = self.notFullCondition.wait(selfUnsync).unwrap();
        }
        
        selfUnsync.queue.push_back(portion);
        
        self.notEmptyCondition.notify_one();
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        let mut selfUnsync = self.mutex.lock().unwrap();
        
        while selfUnsync.queue.is_empty()
        {
            if selfUnsync.workDone
            {
                return None;
            }
            selfUnsync = self.notEmptyCondition.wait(selfUnsync).unwrap();
        }
        
        let portion: Option<E> = selfUnsync.queue.pop_front();
        
        self.notFullCondition.notify_one();
        
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
}
