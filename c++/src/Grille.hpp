#ifndef __VOIDLAND_GRILLE_HPP__
#define __VOIDLAND_GRILLE_HPP__

#include <cstdint>
#include <vector>
#include <optional>


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
    Grille(unsigned halfSideLength, std::vector<uint8_t>&& holes);
    Grille(const Grille& other);
    Grille(Grille&& other);

    Grille& operator=(const Grille& other);
    Grille& operator=(Grille&& other);

    Grille& operator++();

    bool isHole(unsigned x, unsigned y, unsigned rotation) const;
};


class GrilleInterval
{
private:
    Grille next;
    bool preincremented;
    uint64_t begin;
    uint64_t nextOrdinal;
    uint64_t end;

public:
    GrilleInterval(unsigned halfSideLength, uint64_t begin, uint64_t end);

    std::optional<Grille> cloneNext();
    const Grille* getNext();

    float calculateCompletion() const;
};


}

#endif
