package org.voidland.concurrent;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;


public class TurningGrilleCrackerProducerConsumer
        extends TurningGrilleCracker
{
    private int initialConsumerCount;
    private int producerCount;
    private MPMC_PortionQueue<Grille> portionQueue;
    
    private AtomicInteger consumerCount;
    private ConcurrentLinkedQueue<Thread> consumerThreads;
    private List<Thread> producerThreads;

    private AtomicBoolean shutdownOneConsumer;
    private int improving;
    private boolean addingThreads;
    long prevGrillesPerSecond;
    int bestConsumerCount;
    long bestGrillesPerSecond;

    
    public TurningGrilleCrackerProducerConsumer(String cipherText, int initialConsumerCount, int producerCount, MPMC_PortionQueue<Grille> portionQueue)
        throws IOException
    {
        super(cipherText);
        this.initialConsumerCount = initialConsumerCount;
        this.producerCount = producerCount;
        this.consumerCount = new AtomicInteger(0);
        this.portionQueue = portionQueue;
        this.shutdownOneConsumer = new AtomicBoolean(false);
        this.addingThreads = true;
        this.prevGrillesPerSecond = 0;
        this.bestGrillesPerSecond = this.prevGrillesPerSecond;
        this.bestConsumerCount = 0;
        this.producerThreads = new ArrayList<>(this.producerCount);
        this.consumerThreads = new ConcurrentLinkedQueue<>();
        this.improving = 0;
    }

    @Override
    protected void doBruteForce()
    {
        this.startProducerThreads();
        this.startInitialConsumerThreads();
        
        try
        {
            for (Thread producerThread : this.producerThreads)
            {
                producerThread.join();
            }
            
            this.portionQueue.ensureAllPortionsAreRetrieved();
            while (this.grilleCountSoFar.get() < this.grilleCount);   // Ensures all work is done and no more consumers will be started or stopped.
            this.portionQueue.stopConsumers(this.consumerThreads);
            
            Thread consumerThread;
            while ((consumerThread = this.consumerThreads.poll()) != null)
            {
                consumerThread.join();
            }
        }
        catch (InterruptedException e)
        {
            throw new RuntimeException(e);
        }
    }
    
    @Override
    protected String milestone(long grillesPerSecond)
    {
        int queueSize = this.portionQueue.getSize();

        if (grillesPerSecond > this.bestGrillesPerSecond)
        {
            this.bestGrillesPerSecond = grillesPerSecond;
            this.bestConsumerCount = this.consumerCount.get();
        }

        if (this.grilleCountSoFar.get() < this.grilleCount)
        {
            if (grillesPerSecond < this.prevGrillesPerSecond)
            {
                this.improving--;
            }
            else if (grillesPerSecond > this.prevGrillesPerSecond)
            {
                this.improving++;
            }
    
            if (this.improving >= 1 || this.improving <= -2)
            {
                if (this.improving < 0)
                {
                    this.addingThreads = !this.addingThreads;
                }
                this.improving = 0;
    
                if (this.addingThreads)
                {
                    this.consumerThreads.add(this.startConsumerThread());
                }
                else
                {
                    this.shutdownOneConsumer.set(true);
                }
            }
    
            this.prevGrillesPerSecond = grillesPerSecond;
        }

        if (Silo.VERBOSE)
        {
            return "consumer threads: " + this.consumerCount.get() + "; queue size: " + queueSize + " / " + this.portionQueue.getMaxSize();
        }
        return "";
    }
    
    @Override
    protected String milestonesSummary()
    {
        return "best consumer threads: " + this.bestConsumerCount;
    }

    private void startProducerThreads()
    {
        long nextIntervalBegin = 0;
        long intervalLength = Math.round((double)this.grilleCount / this.producerCount);
        for (int i = 0; i < this.producerCount; i++)
        {
            GrilleInterval grilleInterval = new GrilleInterval(this.sideLength / 2, nextIntervalBegin,
                    (i < this.producerCount - 1) ? (nextIntervalBegin + intervalLength) : this.grilleCount);
            
            Thread producerThread = new Thread(() ->
            {
                Grille grille;
                while ((grille = grilleInterval.cloneNext()) != null)
                {
                    this.portionQueue.addPortion(grille);
                }
            });
            this.producerThreads.add(producerThread);
            producerThread.start();
            
            nextIntervalBegin += intervalLength;
        }
    }
    
    private void startInitialConsumerThreads()
    {
        for (int i = 0; i < this.initialConsumerCount; i++)
        {
            this.consumerThreads.add(this.startConsumerThread());
        }
    }
    
    private Thread startConsumerThread()
    {
        this.consumerCount.incrementAndGet();
        Thread consumerThread = new Thread(() ->
        {
            Grille grille;
            while ((grille = portionQueue.retrievePortion()) != null)
            {
                this.applyGrille(grille);
                boolean shutdown = this.shutdownOneConsumer.getAndSet(false);
                if (shutdown && this.consumerCount.get() > 1)
                {
                    break;
                }
            }
            this.consumerCount.decrementAndGet();
        });
        consumerThread.start();
        return consumerThread;
    }
}
