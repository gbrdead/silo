package org.voidland.concurrent.turning_grille;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.voidland.concurrent.Silo;


public class TurningGrilleCrackerSyncless
        implements TurningGrilleCrackerImplDetails
{
	private AtomicInteger workersCount;
	
    private Lock milestoneStateMutex;
	private List<Pair<AtomicLong, Long>> grilleIntervalsCompletion;
	
	
    public TurningGrilleCrackerSyncless()
        throws IOException
    {
        this.workersCount = new AtomicInteger(0);
        this.milestoneStateMutex = new ReentrantLock(false);
    }

    @Override
    public void bruteForce(TurningGrilleCracker cracker)
    {
        int cpuCount = Runtime.getRuntime().availableProcessors();
        
        List<Thread> workerThreads = this.startWorkerThreads(cracker, cpuCount);
        
        try
        {
            for (Thread workerThread : workerThreads)
            {
                workerThread.join();
            }
        }
        catch (InterruptedException e)
        {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void tryMilestone(TurningGrilleCracker cracker, long milestoneEnd, long grilleCountSoFar)
    {
    	if (this.milestoneStateMutex.tryLock())
    	{
    		try
    		{
    			String milestoneStatus = "";
		        if (Silo.VERBOSE)
		        {
		            StringBuilder status = new StringBuilder();
		            
		            status.append("workers: ");
		            status.append(this.workersCount.get());
		            status.append("; completion per worker: ");
		            
		            boolean first = true;
		            for (Pair<AtomicLong, Long> intervalCompletion : this.grilleIntervalsCompletion)
		            {
		                if (!first)
		                {
		                	status.append("/");
		                }
		                else
		                {
		                    first = false;
		                }
		
		                float completion = intervalCompletion.first.get() * 100.0f / intervalCompletion.second;
		                status.append(String.format("%.1f", completion));
		            }
		            status.append("% done");
		            
		            milestoneStatus = status.toString();
		        }
		        
		        cracker.milestone(milestoneEnd, grilleCountSoFar, milestoneStatus);
    		}
    		finally
    		{
    			this.milestoneStateMutex.unlock();
    		}
    	}
    }

    private List<Thread> startWorkerThreads(TurningGrilleCracker cracker, int workerCount)
    {
    	List<Thread> workerThreads = new ArrayList<>(workerCount);
        this.grilleIntervalsCompletion = new ArrayList<>(workerCount);
        
        long nextIntervalBegin = 0;
        long intervalLength = Math.round((double)cracker.grilleCount / workerCount);
        for (int i = 0; i < workerCount; i++)
        {
        	this.workersCount.getAndIncrement();
        	
        	long nextIntervalEnd = (i < workerCount - 1) ? (nextIntervalBegin + intervalLength) : cracker.grilleCount;
            GrilleInterval grilleInterval = new GrilleInterval(cracker.sideLength / 2, nextIntervalBegin, nextIntervalEnd);
            
            Pair<AtomicLong, Long> intervalCompletion = new Pair<AtomicLong, Long>(new AtomicLong(0), (nextIntervalEnd - nextIntervalBegin));
            this.grilleIntervalsCompletion.add(intervalCompletion);
            AtomicLong processedGrillsCount = intervalCompletion.first;
            
            Thread workerThread = new Thread(() ->
            {
            	while (true)
            	{
            		Grille grille = grilleInterval.getNext();
            		if (grille == null)
					{
            			break;
					}
            		long grilleCountSoFar = cracker.applyGrille(grille);
                    cracker.registerOneAppliedGrill(grilleCountSoFar);
                    
                    processedGrillsCount.getAndIncrement();
            	}
                this.workersCount.getAndDecrement();
            });
            workerThreads.add(workerThread);
            workerThread.start();
            
            nextIntervalBegin += intervalLength;
        }
        
        return workerThreads;
    }
    
    
    private static class Pair<F, S>
    {
    	public F first;
    	public S second;
    	
    	public Pair(F first, S second)
    	{
    		this.first = first;
    		this.second = second;
    	}
    }
}
