mod mostly_non_blocking_portion_queue;
mod non_blocking_queue;
mod blocking_portion_queue;

pub use non_blocking_queue::NonBlockingQueue;
pub use non_blocking_queue::ConcurrentPortionQueue;
pub use non_blocking_queue::AsyncMPMC_PortionQueue;
pub use mostly_non_blocking_portion_queue::MostlyNonBlockingPortionQueue;
pub use blocking_portion_queue::StdTextbookPortionQueue;
pub use blocking_portion_queue::ParkingLotTextbookPortionQueue;
pub use blocking_portion_queue::SyncMpmcPortionQueue;



pub trait MPMC_PortionQueue<E> : Send + Sync
{
    fn addPortion(&self, portion: E);
    fn retrievePortion(&self) -> Option<E>;
    
    // The following two methods must be called in succession after the producer threads are done adding portions.
    // After that, the queue cannot be reused.
    fn ensureAllPortionsAreRetrieved(&self);
    fn stopConsumers(&self, finalConsumerCount: usize);
    
    fn getSize(&self) -> usize;
    fn getMaxSize(&self) -> usize;
}
