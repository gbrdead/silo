use core::option::Option;
use std::result::Result;
use concurrent_queue::ConcurrentQueue;
use concurrent_queue::PushError;
use concurrent_queue::PopError;
use std::sync::mpmc::channel;
use std::sync::mpmc::Sender;
use std::sync::mpmc::Receiver;
use std::sync::mpsc::TrySendError;
use std::sync::mpsc::TryRecvError;



pub trait NonBlockingQueue<E> : Send + Sync
{
    fn setMaxSize(&mut self, _maxSize: usize)
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
    fn setMaxSize(&mut self, maxSize: usize)
    {
        self.queue = Some(ConcurrentQueue::<E>::bounded(maxSize));
    }

    fn tryEnqueue(&self, portion: E) -> Result<(), E>
    {
        match self.queue.as_ref().unwrap().push(portion)
        {
            Ok(_) => Ok(()),
            Err(PushError::Full(portion)) => Err(portion),
            Err(PushError::Closed(_)) => panic!()
        }
    }
    
    fn tryDequeue(&self) -> Option<E>
    {
        match self.queue.as_ref().unwrap().pop()
        {
            Ok(portion) => Some(portion),
            Err(PopError::Empty) => None,
            Err(PopError::Closed) => panic!()
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
    fn tryEnqueue(&self, portion: E) -> Result<(), E>
    {
        match self.sender.try_send(portion)
        {
            Ok(_) => Ok(()),
            Err(TrySendError::Full(portion)) => Err(portion),
            Err(TrySendError::Disconnected(_)) => panic!()
        }
    }
    
    fn tryDequeue(&self) -> Option<E>
    {
        match self.receiver.try_recv()
        {
            Ok(portion) => Some(portion),
            Err(TryRecvError::Empty) => None,
            Err(TryRecvError::Disconnected) => panic!()
        }
    }
}
