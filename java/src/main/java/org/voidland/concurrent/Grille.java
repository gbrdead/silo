package org.voidland.concurrent;


public class Grille
{
    private int halfSideLength;
    private byte[] holes;
    
    
    public Grille(int halfSideLength, long ordinal)
    {
        this.halfSideLength = halfSideLength;
        this.holes = new byte[halfSideLength * halfSideLength];
        for (int i = 0; (ordinal != 0) && (i < this.holes.length); i++)
        {
            holes[i] = (byte)(ordinal & 0b11);
            ordinal >>= 2;
        }
    }
    
    public Grille(Grille other)
    {
        this.halfSideLength = other.halfSideLength;
        this.holes = other.holes.clone();
    }
    
    public void increment()
    {
        for (int i = 0; i < this.holes.length; i++)
        {
            if (this.holes[i] < 3)
            {
                this.holes[i]++;
                break;
            }
            this.holes[i] = 0;
        }
    }
    
    public boolean isHole(int x, int y, int rotation)
    {
        int sideLength = this.halfSideLength * 2;
        
        int origX, origY;
        switch (rotation)
        {
            case 0:
            {
                origX = x;
                origY = y;
                break;
            }
            case 1:
            {
                origX = y;
                origY = sideLength - 1 - x;
                break;
            }
            case 2:
            {
                origX = sideLength - 1 - x;
                origY = sideLength - 1 - y;
                break;
            }
            case 3:
            {
                origX = sideLength - 1 - y;
                origY = x;
                break;
            }
            default:
            {
                throw new IllegalArgumentException();
            }
        }
        
        byte quadrant;
        int holeX, holeY;
        if (origX < this.halfSideLength)
        {
            if (origY < this.halfSideLength)
            {
                quadrant = 0;
                holeX = origX;
                holeY = origY;
            }
            else
            {
                quadrant = 3;
                holeX = sideLength - 1 - origY;
                holeY = origX;
            }
        }
        else
        {
            if (origY < this.halfSideLength)
            {
                quadrant = 1;
                holeX = origY;
                holeY = sideLength - 1 - origX;
            }
            else
            {
                quadrant = 2;
                holeX = sideLength - 1 - origX;
                holeY = sideLength - 1 - origY;
            }
        }
        
        return this.holes[holeX * this.halfSideLength + holeY] == quadrant;
    }
}
