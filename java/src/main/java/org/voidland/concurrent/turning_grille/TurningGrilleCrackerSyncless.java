package org.voidland.concurrent.turning_grille;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.voidland.concurrent.Silo;


public class TurningGrilleCrackerSyncless
        implements TurningGrilleCrackerImplDetails
{
	private AtomicInteger workersCount;
	private List<Pair<AtomicLong, Long>> grilleIntervalsCompletion;
	
	
    public TurningGrilleCrackerSyncless()
        throws IOException
    {
        this.workersCount = new AtomicInteger(0);
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
                    cracker.applyGrille(grille);
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
    
    @Override
    public String milestone(TurningGrilleCracker cracker, long grillesPerSecond)
    {
        if (Silo.VERBOSE)
        {
            StringBuilder status = new StringBuilder();
            
            status.append("worker threads: ");
            status.append(this.workersCount.get());
            status.append("; completion per thread: ");
            
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
            
            return status.toString();
        }
        return "";
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
