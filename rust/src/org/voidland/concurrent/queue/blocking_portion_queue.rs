use super::MPMC_PortionQueue;

use std::collections::VecDeque;
use std::sync::mpmc::sync_channel;
use std::sync::mpmc::Sender;
use std::sync::mpmc::Receiver;
use std::thread::sleep;
use std::time::Duration;



struct TextbookPortionQueueUnsync<E>
{
    queue: VecDeque<E>,
    workDone: bool
}


pub struct StdTextbookPortionQueue<E>
{
    maxSize: usize,

    mutex: std::sync::Mutex<TextbookPortionQueueUnsync<E>>,
    notEmptyCondition: std::sync::Condvar,
    notFullCondition: std::sync::Condvar
}

impl<E> StdTextbookPortionQueue<E>
{
    pub fn new(initialConsumerCount: usize, producerCount: usize) -> Self
    {
        let maxSize: usize = initialConsumerCount * producerCount * 1000;
        Self
        {
            maxSize: maxSize,
            mutex: std::sync::Mutex::new(TextbookPortionQueueUnsync
                {
                    queue: VecDeque::with_capacity(maxSize),
                    workDone: false
                }),
            notEmptyCondition: std::sync::Condvar::new(),
            notFullCondition: std::sync::Condvar::new()
        }
    }
}

impl<E: Send + Sync> MPMC_PortionQueue<E> for StdTextbookPortionQueue<E>
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



pub struct ParkingLotTextbookPortionQueue<E>
{
    maxSize: usize,

    mutex: parking_lot::Mutex<TextbookPortionQueueUnsync<E>>,
    notEmptyCondition: parking_lot::Condvar,
    notFullCondition: parking_lot::Condvar
}

impl<E> ParkingLotTextbookPortionQueue<E>
{
    pub fn new(initialConsumerCount: usize, producerCount: usize) -> Self
    {
        let maxSize: usize = initialConsumerCount * producerCount * 1000;
        Self
        {
            maxSize: maxSize,
            mutex: parking_lot::Mutex::new(TextbookPortionQueueUnsync
                {
                    queue: VecDeque::with_capacity(maxSize),
                    workDone: false
                }),
            notEmptyCondition: parking_lot::Condvar::new(),
            notFullCondition: parking_lot::Condvar::new()
        }
    }
}

impl<E: Send + Sync> MPMC_PortionQueue<E> for ParkingLotTextbookPortionQueue<E>
{
    fn addPortion(&self, portion: E)
    {
        let mut selfUnsync = self.mutex.lock();
        
        while selfUnsync.queue.len() >= self.maxSize
        {
            self.notFullCondition.wait(&mut selfUnsync);
        }
        
        selfUnsync.queue.push_back(portion);
        
        self.notEmptyCondition.notify_one();
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        let mut selfUnsync = self.mutex.lock();
        
        while selfUnsync.queue.is_empty()
        {
            if selfUnsync.workDone
            {
                return None;
            }
            self.notEmptyCondition.wait(&mut selfUnsync);
        }
        
        let portion: Option<E> = selfUnsync.queue.pop_front();
        
        self.notFullCondition.notify_one();
        
        portion
    }
    
    fn ensureAllPortionsAreRetrieved(&self)
    {
        let mut selfUnsync = self.mutex.lock();
        
        while !selfUnsync.queue.is_empty()
        {
            self.notFullCondition.wait(&mut selfUnsync);
        }
    }
    
    fn stopConsumers(&self, _finalConsumerCount: usize)
    {
        let mut selfUnsync = self.mutex.lock();

        selfUnsync.workDone = true;

        self.notEmptyCondition.notify_all();
    }
    
    fn getSize(&self) -> usize
    {
        let selfUnsync = self.mutex.lock();
        selfUnsync.queue.len()
    }
    
    fn getMaxSize(&self) -> usize
    {
        self.maxSize
    }
}



pub struct SyncMpmcPortionQueue<E>
{
    maxSize: usize,
    sender: Sender<Option<E>>,
    receiver: Receiver<Option<E>>
}

impl<E> SyncMpmcPortionQueue<E>
{
    pub fn new(initialConsumerCount: usize, producerCount: usize) -> Self
    {
        let maxSize: usize = initialConsumerCount * producerCount * 1000;
        let (sender, receiver): (Sender<Option<E>>, Receiver<Option<E>>) = sync_channel::<Option<E>>(maxSize);
        Self
        {
            maxSize: maxSize,
            sender: sender,
            receiver: receiver
        }
    }
}

impl<E: Send + Sync> MPMC_PortionQueue<E> for SyncMpmcPortionQueue<E>
{
    fn addPortion(&self, portion: E)
    {
        let _ = self.sender.send(Some(portion));
    }
    
    fn retrievePortion(&self) -> Option<E>
    {
        self.receiver.recv().unwrap()
    }
    
    fn ensureAllPortionsAreRetrieved(&self)
    {
        while self.receiver.len() > 0
        {
            sleep(Duration::from_millis(1));
        }
    }
    
    fn stopConsumers(&self, finalConsumerCount: usize)
    {
        for _i in 0..finalConsumerCount
        {
            let _ = self.sender.send(None);
        }
    }
    
    fn getSize(&self) -> usize
    {
        self.receiver.len()
    }
    
    fn getMaxSize(&self) -> usize
    {
        self.maxSize
    }
}
