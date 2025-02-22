use concurrent_queue::ConcurrentQueue;
use core::option::Option;
use std::result::Result;


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
