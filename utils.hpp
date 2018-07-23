#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <string>

#include <gecode/minimodel.hh>
#include <gecode/int.hh>

Gecode::IntSharedArray string2array(const std::string &str)
{
    Gecode::IntSharedArray ret(str.size());
    for(size_t i = 0; i < str.size(); ++i)
        ret[i] = str[i];
    return ret;
}

void words_not_overlapping(const Gecode::Home &home, const Gecode::IntVarArgs &positions, const Gecode::IntVarArgs &lengths)
{
    // Assert: positions.size() == lengths.size()
    
    size_t size = positions.size();
    for(size_t i = 1; i < size; ++i)
    {
        // Symmetry breaking: positions[i-1] < positions[i]

        // Then, as positions[i-1] < positions[i], !colliding <=> positions[i-1]+lengths[i-1] < positions[i]. Inequality is strict since we want at least one black tile as separator.
        Gecode::rel(home, (lengths[i] > 2) == (positions[i-1] + lengths[i-1] < positions[i]));
    }
}

void word_no_overflow(const Gecode::Home &home, const Gecode::IntVar &position, const Gecode::IntVar &length, size_t width)
{
    Gecode::rel(home, position + length < width);
}

void length_of_word(const Gecode::Home &home, const Gecode::IntVar &index, const Gecode::IntVar &length)
{
    // Index == -1 <=> length == 0
    Gecode::rel(home, (index == -1) == (length == 0));

    // Todo: use dictionary for other length
}

void letters_of_word(const Gecode::Home &home, const Gecode::IntVarArgs &letters, const Gecode::IntVar &index, const Gecode::IntVar &position)
{
    (void) index;
    Gecode::element(home, letters, position, index);
}

#endif

