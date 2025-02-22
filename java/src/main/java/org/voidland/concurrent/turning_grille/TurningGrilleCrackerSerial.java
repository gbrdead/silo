package org.voidland.concurrent.turning_grille;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import org.voidland.concurrent.Silo;


public class TurningGrilleCrackerSerial
        implements TurningGrilleCrackerImplDetails
{
    public TurningGrilleCrackerSerial()
        throws IOException
    {
    }

    @Override
    public void bruteForce(TurningGrilleCracker cracker)
    {
    	GrilleInterval grilleInterval = new GrilleInterval(cracker.sideLength / 2, 0, cracker.grilleCount);
    	long milestoneStart = System.nanoTime();
    	long grilleCountAtMilestoneStart = 0;
    	
    	while (true)
    	{
    		Grille grille = grilleInterval.getNext();
    		if (grille == null)
    		{
    			break;
    		}
    		long grilleCountSoFar = cracker.applyGrille(grille);

            if (grilleCountSoFar % (cracker.grilleCount / 1000) == 0)   // A milestone every 0.1%.
            {
            	long milestoneEnd = System.nanoTime();
                long elapsedTimeNs = milestoneEnd - milestoneStart;
                if (elapsedTimeNs > 0)
                {
                    long grilleCountForMilestone = grilleCountSoFar - grilleCountAtMilestoneStart;
                    long grillesPerSecond = grilleCountForMilestone * TimeUnit.SECONDS.toNanos(1) / elapsedTimeNs;
                    if (grillesPerSecond > cracker.bestGrillesPerSecond)
                    {
                        cracker.bestGrillesPerSecond = grillesPerSecond;
                    }
	
	    			if (Silo.VERBOSE)
	    			{
	    				float done = grilleCountSoFar * 100.0f / cracker.grilleCount;
	
                        System.err.print(String.format("%.1f", done) + "% done; ");
                        System.err.print("current speed: " + grillesPerSecond + " grilles/s; ");
                        System.err.print("best speed so far: " + cracker.bestGrillesPerSecond + " grilles/s");
                        System.err.println();
	    			}
	
	    			milestoneStart = milestoneEnd;
	    			grilleCountAtMilestoneStart = grilleCountSoFar;
                }
            }
    	}
    }
    
    @Override
    public void tryMilestone(TurningGrilleCracker cracker, long milestoneEnd, long grilleCountSoFar)
    {
    	cracker.milestone(milestoneEnd, grilleCountSoFar, "");
    }
}
