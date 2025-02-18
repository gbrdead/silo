use std::vec::Vec;



pub struct Grille
{
    halfSideLength: usize,
    holes: Vec<u8>
}

impl Grille
{
    #[inline]
    pub fn new(halfSideLength: usize, ordinal: u64) -> Self
    {
        let mut ret: Self = Self
        {
            halfSideLength: halfSideLength,
            holes: vec![0; halfSideLength * halfSideLength]
        };
        
        let mut ord: u64 = ordinal;
        for i in ret.holes.iter_mut()
        {
            if ord == 0
            {
                break;
            }
            *i = (ord & 0b11) as u8;
            ord >>= 2;
        }
        
        ret
    }
    
    #[inline]
    pub fn increment(&mut self)
    {
        for i in self.holes.iter_mut()
        {
            if *i < 3
            {
                *i += 1;
                break;
            }
            *i = 0;
        }
    }

    #[inline]
    pub fn isHole(&self, x: usize, y: usize, rotation: usize) -> bool
    {
        let sideLength: usize = self.halfSideLength * 2;

        let origX: usize;
        let origY: usize;
        match rotation
        {
            0 =>
            {
                origX = x;
                origY = y;
            }
            1 =>
            {
                origX = y;
                origY = sideLength - 1 - x;
            }
            2 =>
            {
                origX = sideLength - 1 - x;
                origY = sideLength - 1 - y;
            }
            3 =>
            {
                origX = sideLength - 1 - y;
                origY = x;
            }
            _ =>
            {
                panic!();
            }
        }
        
        let quadrant: u8;
        let holeX: usize;
        let holeY: usize;
        if origX < self.halfSideLength
        {
            if origY < self.halfSideLength
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
            if origY < self.halfSideLength
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

        self.holes[holeX * self.halfSideLength + holeY] == quadrant
    }   
}

impl Clone for Grille
{
    #[inline]
    fn clone(&self) -> Self
    {
        Self
        {
            halfSideLength: self.halfSideLength,
            holes: self.holes.clone()
        }
    }
}


pub struct GrilleInterval
{
    next: Grille,
    preincremented: bool,
    nextOrdinal: u64,
    end: u64
}

impl GrilleInterval
{
    #[inline]
    pub fn new(halfSideLength: usize, begin: u64, end: u64) -> Self
    {
        Self
        {
            next: Grille::new(halfSideLength, begin),
            preincremented: true,
            nextOrdinal: begin,
            end: end
        }
    }
    
    #[inline]
    pub fn cloneNext(&mut self) -> Option<Grille>
    {
        if self.nextOrdinal >= self.end
        {
            return None;
        }

        if !self.preincremented
        {
            self.next.increment();
        }

        let current: Grille = self.next.clone();

        self.next.increment();
        self.preincremented = true;

        self.nextOrdinal += 1;
        Some(current)
    }
    
    #[inline]
    pub fn getNext(&mut self) -> Option<&Grille>
    {
        if self.nextOrdinal >= self.end
        {
            return None;
        }

        if !self.preincremented
        {
            self.next.increment();
        }
        self.preincremented = false;

        self.nextOrdinal += 1;
        Some(&self.next)
    }
}
