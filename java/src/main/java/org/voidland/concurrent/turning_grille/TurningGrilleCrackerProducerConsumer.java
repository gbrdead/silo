package org.voidland.concurrent.turning_grille;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

import org.voidland.concurrent.Silo;
import org.voidland.concurrent.queue.MPMC_PortionQueue;


public class TurningGrilleCrackerProducerConsumer
        implements TurningGrilleCrackerImplDetails
{
    private int initialConsumerCount;
    private int producerCount;
    private MPMC_PortionQueue<Grille> portionQueue;
    
    private AtomicInteger consumerCount;
    private ConcurrentLinkedQueue<Thread> consumerThreads;
    private AtomicInteger shutdownNConsumers;
    
    private int improving;
    private boolean addingThreads;
    long prevGrillesPerSecond;
    int bestConsumerCount;
    long bestGrillesPerSecond;

    
    public TurningGrilleCrackerProducerConsumer(int initialConsumerCount, int producerCount, MPMC_PortionQueue<Grille> portionQueue)
        throws IOException
    {
        this.initialConsumerCount = initialConsumerCount;
        this.producerCount = producerCount;
        this.consumerCount = new AtomicInteger(0);
        this.portionQueue = portionQueue;
        this.shutdownNConsumers = new AtomicInteger(0);
        this.addingThreads = true;
        this.prevGrillesPerSecond = 0;
        this.bestGrillesPerSecond = this.prevGrillesPerSecond;
        this.bestConsumerCount = 0;
        this.consumerThreads = new ConcurrentLinkedQueue<>();
        this.improving = 0;
    }

    @Override
    public void bruteForce(TurningGrilleCracker cracker)
    {
    	List<Thread> producerThreads = this.startProducerThreads(cracker);
        this.startInitialConsumerThreads(cracker);
        
        try
        {
            for (Thread producerThread : producerThreads)
            {
                producerThread.join();
            }
            
            this.portionQueue.ensureAllPortionsAreRetrieved();
            while (cracker.grilleCountSoFar.get() < cracker.grilleCount);   // Ensures all work is done and no more consumers will be started or stopped.
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
    public String milestone(TurningGrilleCracker cracker, long grillesPerSecond)
    {
        int queueSize = this.portionQueue.getSize();

        if (grillesPerSecond > this.bestGrillesPerSecond)
        {
            this.bestGrillesPerSecond = grillesPerSecond;
            this.bestConsumerCount = this.consumerCount.get();
        }

        if (cracker.grilleCountSoFar.get() < cracker.grilleCount)
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
                    this.consumerThreads.add(this.startConsumerThread(cracker));
                }
                else
                {
                    this.shutdownNConsumers.getAndIncrement();
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
    public String milestonesSummary(TurningGrilleCracker cracker)
    {
        return "best consumer threads: " + this.bestConsumerCount;
    }

    private List<Thread> startProducerThreads(TurningGrilleCracker cracker)
    {
    	List<Thread> producerThreads = new ArrayList<>(this.producerCount);
    	
        long nextIntervalBegin = 0;
        long intervalLength = Math.round((double)cracker.grilleCount / this.producerCount);
        for (int i = 0; i < this.producerCount; i++)
        {
            GrilleInterval grilleInterval = new GrilleInterval(cracker.sideLength / 2, nextIntervalBegin,
                    (i < this.producerCount - 1) ? (nextIntervalBegin + intervalLength) : cracker.grilleCount);
            
            Thread producerThread = new Thread(() ->
            {
                while (true)
                {
                	Grille grille = grilleInterval.cloneNext();
                	if (grille == null)
                	{
                		break;
                	}
                	this.portionQueue.addPortion(grille);
                }
            });
            producerThreads.add(producerThread);
            producerThread.start();
            
            nextIntervalBegin += intervalLength;
        }
        
        return producerThreads;
    }
    
    private void startInitialConsumerThreads(TurningGrilleCracker cracker)
    {
        for (int i = 0; i < this.initialConsumerCount; i++)
        {
            this.consumerThreads.add(this.startConsumerThread(cracker));
        }
    }
    
    private Thread startConsumerThread(TurningGrilleCracker cracker)
    {
        this.consumerCount.getAndIncrement();
        Thread consumerThread = new Thread(() ->
        {
            while (true)
            {
            	Grille grille = portionQueue.retrievePortion();
            	if (grille == null)
            	{
            		this.consumerCount.getAndDecrement();
            		break;
            	}
        
            	long grilleCountSoFar = cracker.applyGrille(grille);
                cracker.registerOneAppliedGrill(grilleCountSoFar);
                
                if (this.shutdownNConsumers.get() > 0)
                {
	              	if (this.shutdownNConsumers.getAndDecrement() > 0)
	                {
	                	if (this.consumerCount.getAndDecrement() > 1)	// There should be at least one consumer running.
						{
							return;
						}
						else
						{
							this.consumerCount.getAndIncrement();
						}
	                }
	                else
	                {
	                	this.shutdownNConsumers.getAndIncrement();
	                }
                }
            }
        });
        consumerThread.start();
        return consumerThread;
    }
}
