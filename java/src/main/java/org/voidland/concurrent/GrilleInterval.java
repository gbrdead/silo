package org.voidland.concurrent;


public class GrilleInterval
{
    private Grille next;
    boolean preincremented;
    private long begin;
    private long nextOrdinal;
    private long end;
    
    
    public GrilleInterval(int halfSideLength, long begin, long end)
    {
        this.next = new Grille(halfSideLength, begin);
        this.preincremented = true;
        this.begin = begin;
        this.nextOrdinal = begin;
        this.end = end;
    }
    
    public Grille cloneNext()
    {
        if (this.nextOrdinal >= this.end)
        {
            return null;
        }
        
        if (!this.preincremented)
        {
            this.next.increment();
        }
        
        Grille current = new Grille(this.next);
        
        this.next.increment();;
        this.preincremented = true;

        this.nextOrdinal++;
        return current;
    }
    
    public Grille getNext()
    {
        if (this.nextOrdinal >= this.end)
        {
            return null;
        }

        if (!this.preincremented)
        {
            this.next.increment();
        }
        this.preincremented = false;

        this.nextOrdinal++;
        return this.next;
    }
    
    public float calculateCompletion()
    {
    	return (this.nextOrdinal - this.begin) * 100.0f / (this.end - this.begin);
    }
}
