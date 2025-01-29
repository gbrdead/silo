package org.voidland.concurrent;

import java.io.IOException;
import java.util.Locale;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;


public abstract class TurningGrilleCracker
{
    private static final String WORDS_FILE_PATH = "The_Oxford_3000.txt";
    private static final int MIN_DETECTED_WORD_COUNT = 16;	// Determined by gut feeling.
    

    protected int sideLength;
    protected long grilleCount;
    protected AtomicLong grilleCountSoFar;
    
    private char[] cipherText;
    private WordsTrie wordsTrie;
    
    private AtomicBoolean milestoneReportInProgress;
    private long start;
    
    private long grilleCountAtMilestoneStart;
    private long milestoneStart;
    private long bestGrillesPerSecond;

    
    public TurningGrilleCracker(String cipherText)
    	throws IOException
    {
    	cipherText = cipherText.toUpperCase(Locale.ENGLISH);
    	if (cipherText.matches("[^A-Z]"))
    	{
    		throw new IllegalArgumentException("The ciphertext must contain only English letters.");
    	}
    	this.cipherText = cipherText.toCharArray();
    	
        this.sideLength = (int)Math.sqrt(this.cipherText.length);
        if (this.sideLength % 2 != 0  ||  this.sideLength * this.sideLength != this.cipherText.length)
        {
            throw new IllegalArgumentException("The ciphertext length must be a square of an odd number.");
        }

        this.grilleCount = 1;
        for (int i = 0; i < (this.sideLength * this.sideLength) / 4; i++)
        {
            this.grilleCount *= 4;
        }
    	
    	this.wordsTrie = new WordsTrie(TurningGrilleCracker.WORDS_FILE_PATH);
    
        this.milestoneReportInProgress = new AtomicBoolean(false);
        this.grilleCountSoFar = new AtomicLong(0);
    }
    
    public void bruteForce()
    {
        this.milestoneStart = this.start = System.nanoTime();
        this.doBruteForce();
        long end = System.nanoTime();
        long elapsedTimeNs = end - this.start;
        long grillsPerSecond = this.grilleCount * TimeUnit.SECONDS.toNanos(1) / elapsedTimeNs;

        System.err.print("Average speed: " + grillsPerSecond + " grilles/s; best speed: " + this.bestGrillesPerSecond + " grilles/s");
        String summary = this.milestonesSummary();
        if (summary.length() > 0)
        {
            System.err.print("; " + summary);
        }
        System.err.println();

        if (this.grilleCountSoFar.get() != this.grilleCount)
        {
            throw new RuntimeException("Some grills got lost.");
        }        
    }
    
    protected abstract void doBruteForce();
    
    protected String milestone(long elapsedTimeNs)
    {
        return "";
    }
    
    protected String milestonesSummary()
    {
        return "";
    }
    
    protected void applyGrille(Grille grill)
    {
        StringBuilder candidateBuf = new StringBuilder(this.cipherText.length);
        
        for (int rotation = 0; rotation < 4; rotation++)
        {
            for (int y = 0; y < this.sideLength; y++)
            {
                for (int x = 0; x < this.sideLength; x++)
                {
                   if (grill.isHole(x, y, rotation))
                   {
                       candidateBuf.append(this.cipherText[y * this.sideLength + x]);
                   }
                }
            }
        }
        
        this.findWordsAndReport(candidateBuf);
        candidateBuf.reverse();
        this.findWordsAndReport(candidateBuf);
        
        long grilleCountSoFar = this.grilleCountSoFar.incrementAndGet();
        if (grilleCountSoFar % (this.grilleCount / 1000) == 0)   // A milestone every 0.1%.
        {
            long milestoneEnd = System.nanoTime();
            
            if (this.milestoneReportInProgress.compareAndSet(false, true))
            {
                long elapsedTimeNs = milestoneEnd - this.milestoneStart;
                long grilleCountForMilestone = grilleCountSoFar - this.grilleCountAtMilestoneStart;
                
                long grillesPerSecond = grilleCountForMilestone * TimeUnit.SECONDS.toNanos(1) / elapsedTimeNs;
    
                if (grillesPerSecond > this.bestGrillesPerSecond)
                {
                    this.bestGrillesPerSecond = grillesPerSecond;
                }
    
                String milestoneStatus = this.milestone(elapsedTimeNs);
                
                if (Silo.VERBOSE)
                {
                    float done = grilleCountSoFar * 100.0f / this.grilleCount;
        
                    System.err.print(String.format("%.1f", done) + "% done; ");
                    System.err.print("current speed: " + grillesPerSecond + " grilles/s; ");
                    System.err.print("best speed so far: " + this.bestGrillesPerSecond + " grilles/s");
                    if (milestoneStatus.length() > 0)
                    {
                        System.err.print("; " + milestoneStatus);
                    }
                    System.err.println();
                }
                
                this.milestoneReportInProgress.set(false);
            }

            this.milestoneStart = milestoneEnd;
            this.grilleCountAtMilestoneStart = grilleCountSoFar;
        }
    }
    
    private void findWordsAndReport(CharSequence candidate)
    {
        int wordsFound = this.wordsTrie.countWords(candidate);
        if (wordsFound >= TurningGrilleCracker.MIN_DETECTED_WORD_COUNT)
        {
            if (Silo.VERBOSE)
            {
                System.out.println(wordsFound + ": " + candidate);
            }
        }
    }
}
