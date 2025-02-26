use concurrent_queue::ConcurrentQueue;
use core::option::Option;
use std::result::Result;
use std::sync::mpmc::channel;
use std::sync::mpmc::Sender;
use std::sync::mpmc::Receiver;



pub trait NonBlockingQueue<E> : Send + Sync
{
    fn setSizeParameters(&mut self, _producerCount: usize, _maxSize: usize)
    {
    }

    fn tryEnqueue(&self, portion: E) -> Result<(), E>;
    fn tryDequeue(&self) -> Option<E>;
}



pub struct ConcurrentPortionQueue<E>
{
    queue: Option<ConcurrentQueue<E>>
}

impl<E> ConcurrentPortionQueue<E>
{
    pub fn new() -> Self
    {
        Self
        {
            queue: None
        }
    }
}

impl<E: Send + Sync> NonBlockingQueue<E> for ConcurrentPortionQueue<E>
{
    fn setSizeParameters(&mut self, _producerCount: usize, maxSize: usize)
    {
        self.queue = Some(ConcurrentQueue::<E>::bounded(maxSize));
    }

    fn tryEnqueue(&self, portion: E) -> Result<(), E>
    {
        match self.queue.as_ref().unwrap().push(portion)
        {
            Ok(_) => Ok(()),
            Err(error) => Err(error.into_inner())
        }
    }
    
    fn tryDequeue(&self) -> Option<E>
    {
        match self.queue.as_ref().unwrap().pop()
        {
            Ok(portion) => Some(portion),
            Err(_) => None
        }
    }
}



pub struct AsyncMpmcPortionQueue<E>
{
    sender: Sender<E>,
    receiver: Receiver<E>
}

impl<E> AsyncMpmcPortionQueue<E>
{
    pub fn new() -> Self
    {
        let (sender, receiver): (Sender<E>, Receiver<E>) = channel::<E>();
        Self
        {
            sender: sender,
            receiver: receiver
        }
    }
}

impl<E: Send + Sync> NonBlockingQueue<E> for AsyncMpmcPortionQueue<E>
{
    fn setSizeParameters(&mut self, _producerCount: usize, _maxSize: usize)
    {
    }

    fn tryEnqueue(&self, portion: E) -> Result<(), E>
    {
        let _ = self.sender.send(portion);
        Ok(())
    }
    
    fn tryDequeue(&self) -> Option<E>
    {
        match self.receiver.recv()
        {
            Ok(portion) => Some(portion),
            Err(_) => None
        }
    }
}
