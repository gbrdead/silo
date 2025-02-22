#ifndef __VOIDLAND_GRILLE_HPP__
#define __VOIDLAND_GRILLE_HPP__

#include <cstdint>
#include <vector>
#include <optional>
#include <stdexcept>


namespace org::voidland::concurrent::turning_grille
{


class Grille
{
private:
    unsigned halfSideLength;
    std::vector<uint8_t> holes;

public:
    Grille();
    Grille(unsigned halfSideLength, uint64_t ordinal);
    Grille(const Grille& other);
    Grille(Grille&& other);

    Grille& operator=(const Grille& other);
    Grille& operator=(Grille&& other);

    Grille& operator++();

    bool isHole(unsigned x, unsigned y, unsigned rotation) const;
};

inline Grille::Grille() :
    halfSideLength(0)
{
}

inline Grille::Grille(unsigned halfSideLength, uint64_t ordinal) :
    halfSideLength(halfSideLength),
    holes(halfSideLength * halfSideLength, 0)
{
    for (std::vector<uint8_t>::iterator i = this->holes.begin(); (ordinal != 0) && (i != this->holes.end()); ++i)
    {
        *i = (ordinal & 0b11);
        ordinal >>= 2;
    }
}

inline Grille::Grille(const Grille& other) :
    halfSideLength(other.halfSideLength),
    holes(other.holes)
{
}

inline Grille::Grille(Grille&& other) :
    halfSideLength(other.halfSideLength),
    holes(std::move(other.holes))
{
    other.halfSideLength = 0;
}

inline Grille& Grille::operator=(const Grille& other)
{
    this->halfSideLength = other.halfSideLength;
    this->holes = other.holes;
    return *this;
}

inline Grille& Grille::operator=(Grille&& other)
{
    this->halfSideLength = other.halfSideLength;
    other.halfSideLength = 0;
    this->holes = std::move(other.holes);
    return *this;
}

inline Grille& Grille::operator++()
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

inline bool Grille::isHole(unsigned x, unsigned y, unsigned rotation) const
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


class GrilleInterval
{
private:
    Grille next;
    bool preincremented;
    uint64_t nextOrdinal;
    uint64_t end;

public:
    GrilleInterval(unsigned halfSideLength, uint64_t begin, uint64_t end);

    std::optional<Grille> cloneNext();
    const Grille* getNext();
};

inline GrilleInterval::GrilleInterval(unsigned halfSideLength, uint64_t begin, uint64_t end) :
    next(halfSideLength, begin),
    preincremented(true),
    nextOrdinal(begin),
    end(end)
{
}

inline std::optional<Grille> GrilleInterval::cloneNext()
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

inline const Grille* GrilleInterval::getNext()
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


}

#endif
