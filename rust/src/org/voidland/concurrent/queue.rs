mod mostly_non_blocking_portion_queue;
mod non_blocking_queue;
mod blocking_portion_queue;

pub use non_blocking_queue::NonBlockingQueue;
pub use non_blocking_queue::ConcurrentPortionQueue;
pub use mostly_non_blocking_portion_queue::MostlyNonBlockingPortionQueue;
pub use blocking_portion_queue::TextbookPortionQueue;


pub trait MPMC_PortionQueue<E> : Send + Sync
{
    fn addPortion(&self, portion: E);
    fn retrievePortion(&self) -> Option<E>;
    fn ensureAllPortionsAreRetrieved(&self);
    fn stopConsumers(&self, consumerCount: usize);
    fn getSize(&self) -> usize;
    fn getMaxSize(&self) -> usize;
}
