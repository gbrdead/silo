#include "Grille.hpp"

#include <stdexcept>


namespace org::voidland::concurrent::turning_grille
{


Grille::Grille() :
    halfSideLength(0)
{
}

Grille::Grille(unsigned halfSideLength, uint64_t ordinal) :
    halfSideLength(halfSideLength),
    holes(halfSideLength * halfSideLength, 0)
{
    for (std::vector<uint8_t>::iterator i = this->holes.begin(); (ordinal != 0) && (i != this->holes.end()); ++i)
    {
        *i = (ordinal & 0b11);
        ordinal >>= 2;
    }
}

Grille::Grille(const Grille& other) :
    halfSideLength(other.halfSideLength),
    holes(other.holes)
{
}

Grille::Grille(Grille&& other) :
    halfSideLength(other.halfSideLength),
    holes(std::move(other.holes))
{
    other.halfSideLength = 0;
}

Grille::Grille(unsigned halfSideLength, std::vector<uint8_t>&& holes) :
    halfSideLength(halfSideLength),
    holes(std::move(holes))
{
}

Grille& Grille::operator=(const Grille& other)
{
    this->halfSideLength = other.halfSideLength;
    this->holes = other.holes;
    return *this;
}

Grille& Grille::operator=(Grille&& other)
{
    this->halfSideLength = other.halfSideLength;
    other.halfSideLength = 0;
    this->holes = std::move(other.holes);
    return *this;
}

Grille& Grille::operator++()
{
	for (uint8_t& hole : this->holes)
    {
        if (hole < 3)
        {
        	hole++;
            break;
        }
        hole = 0;
    }

    return *this;
}

bool Grille::isHole(unsigned x, unsigned y, unsigned rotation) const
{
    unsigned sideLength = this->halfSideLength * 2;

    unsigned origX, origY;
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
            throw std::invalid_argument("");
        }
    }

    uint8_t quadrant;
    unsigned holeX, holeY;
    if (origX < this->halfSideLength)
    {
        if (origY < this->halfSideLength)
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
        if (origY < this->halfSideLength)
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

    return this->holes[holeX * this->halfSideLength + holeY] == quadrant;
}


GrilleInterval::GrilleInterval(unsigned halfSideLength, uint64_t begin, uint64_t end) :
    next(halfSideLength, begin),
    preincremented(true),
	begin(begin),
    nextOrdinal(begin),
    end(end)
{
}

std::optional<Grille> GrilleInterval::cloneNext()
{
    if (this->nextOrdinal >= this->end)
    {
        return std::nullopt;
    }

    if (!this->preincremented)
    {
        ++this->next;
    }

    Grille current(this->next);

    ++this->next;
    this->preincremented = true;

    this->nextOrdinal++;
    return current;
}

const Grille* GrilleInterval::getNext()
{
    if (this->nextOrdinal >= this->end)
    {
        return nullptr;
    }

    if (!this->preincremented)
    {
        ++this->next;
    }
    this->preincremented = false;

    this->nextOrdinal++;
    return &this->next;
}

float GrilleInterval::calculateCompletion() const
{
	return (this->nextOrdinal - this->begin) * 100.0f / (this->end - this->begin);
}


}
