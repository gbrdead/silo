package org.voidland.concurrent.queue;


public interface NonBlockingQueue<E>
{
    boolean tryEnqueue(E portion);
    E tryDequeue();
}
