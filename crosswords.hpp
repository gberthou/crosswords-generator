#ifndef CROSSWORDS_HPP
#define CROSSWORDS_HPP

#include <ctime>

#include <gecode/driver.hh>
#include <gecode/int.hh>

#include "dictionary.hpp"
#include "dfa.hpp"
#include "ui.hpp"

using namespace Gecode;

class Crosswords: public Script
{
    public:
        Crosswords(const SizeOptions &opt, size_t width, size_t height, const Dictionary &dictionary):
            Script(opt),
            width(width),
            height(height),

            letters(*this, width * height, 'a', 'z'+1), // Letters go from 'a' to 'z', and black tile is 'z'+1 ('{')

            indBH(*this, 2, dictionary.FirstIndexOfLength(width), dictionary.LastIndexOfLength(width)),
            indBV(*this, 2, dictionary.FirstIndexOfLength(height), dictionary.LastIndexOfLength(height)),

            ind1H(*this, height-2, dictionary.FirstIndexOfLength(2), dictionary.LastIndexOfLength(width)),
            ind2H(*this, height-2, MIN_INDEX, dictionary.LastIndexOfLength(width-3)),
            ind1V(*this, width-2, dictionary.FirstIndexOfLength(2), dictionary.LastIndexOfLength(height)),
            ind2V(*this, width-2, MIN_INDEX, dictionary.LastIndexOfLength(height-3)),
            
            wordPos1H(*this, height-2, 0, 2),
            wordPos2H(*this, height-2, 3, width+1),
            wordPos1V(*this, width-2, 0, 2),
            wordPos2V(*this, width-2, 3, height+1),

            wordLen1H(*this, height-2, 2, width),
            wordLen1V(*this, width-2, 2, height)
        {
            // DFA manipulations
            DictionaryDFA dictDFA(dictionary, width, height);
            DFA *dfa_borderH = dictDFA.BorderH();
            DFA *dfa_borderV = dictDFA.BorderV();
            DFA *dfa_firstH = dictDFA.FirstH();
            DFA *dfa_firstV = dictDFA.FirstV();
            DFA *dfa_secondH = dictDFA.SecondH();
            DFA *dfa_secondV = dictDFA.SecondV();

            // Fewer black tiles than X
            count(*this, letters, 'z'+1, IRT_LQ, 10); // <= 10

            auto allIndices = indBH+indBV+ind1H+ind2H+ind1V+ind2V;

            distinct(*this, allIndices, MIN_INDEX);

            /* Borders */
            // First row
            IntVarArgs topborderH = letters.slice(0, 1, width) + indBH[0];
            extensional(*this, topborderH, *dfa_borderH);

            // Last row
            IntVarArgs botborderH = letters.slice((height-1)*width, 1, width) + indBH[1];
            extensional(*this, botborderH, *dfa_borderH);

            // First column
            IntVarArgs leftborderV = letters.slice(0, width, height) + indBV[0];
            extensional(*this, leftborderV, *dfa_borderV);
            
            // Last column
            IntVarArgs rightborderV = letters.slice(width-1, width, height) + indBV[1];
            extensional(*this, rightborderV, *dfa_borderV);

            // Impose mandatory words
            //for(int indice : mandatoryIndices)
            //    count(*this, allIndices, indice, IRT_EQ, 1);

            // Horizontal words
            for(size_t y = 1; y < height-1; ++y)
            {
                // First words
                IntVarArgs row = letters.slice(y*width, 1, width);
                extensional(*this, wordPos1H[y-1] + row + ind1H[y-1] + wordLen1H[y-1], *dfa_firstH);

                // Second words
                IntVarArgs reducedRow = letters.slice(y*width+3, 1, width-3);
                extensional(*this, wordPos2H[y-1] + reducedRow + ind2H[y-1], *dfa_secondH);
                rel(*this, wordPos2H[y-1] == wordPos1H[y-1] + wordLen1H[y-1] + 1);
            }
            
            // Vertical words
            for(size_t x = 1; x < width-1; ++x)
            {
                // First words
                IntVarArgs col = letters.slice(x, width, height);
                extensional(*this, wordPos1V[x-1] + col + ind1V[x-1] + wordLen1V[x-1], *dfa_firstV);

                // Second words
                IntVarArgs reducedCol = letters.slice(x+3*width, width, height-3);
                extensional(*this, wordPos2V[x-1] + reducedCol + ind2V[x-1], *dfa_secondV);
                rel(*this, wordPos2V[x-1] == wordPos1V[x-1] + wordLen1V[x-1] + 1);
            }

            //if(!mandatoryIndices.size())
            {
                auto seed = std::time(nullptr);
                branch(*this, indBH+indBV+ind1H+ind1V, INT_VAR_SIZE_MIN(), INT_VAL_RND(seed));
                branch(*this, ind2H+ind2V, INT_VAR_NONE(), INT_VAL_RND(seed));
            }
#if 0
            else
            {
                Rnd rnd(std::time(nullptr));
                auto indexValBrancher = [rnd](const Space &, IntVar x, int) mutable {
                    // Principle: try mandatory words first if they belong to x's domain.
                    // Otherwise, use random selection within domain.

                    // Random generator gets altered regardless of mandatory words being selected or not.
                    unsigned int p = rnd(x.size());
                    int randomIndex;
                    bool randomAssigned = false;

                    for(Int::ViewRanges<Int::IntView> i(x); i(); ++i)
                    {
                        int min = i.min();
                        int max = i.max();

                        for(int index : mandatoryIndices)
                            if(index >= min && index <= max)
                                return index;

                        // (Copy-pasted from original Gecode ValSelRnd<View>::val(...))
                        if(randomAssigned)
                            continue;
                        if (i.width() > p)
                        {
                            randomIndex = i.min() + static_cast<int>(p);
                            randomAssigned = true;
                        }
                        else
                            p -= i.width();
                    }

                    // If program reaches this part, mandatory indices were unavailable (either none were specified, or are assigned to other words)
                    return randomIndex;
                };

                branch(*this, indBH+indBV+ind1H+ind1V, INT_VAR_SIZE_MIN(), INT_VAL(indexValBrancher));
                branch(*this, ind2H+ind2V, INT_VAR_NONE(), INT_VAL(indexValBrancher));
            }
#endif

            branch(*this, wordPos1H+wordPos1V, INT_VAR_NONE(), INT_VAL_MIN());

            delete dfa_borderH;
            delete dfa_borderV;
            delete dfa_firstH;
            delete dfa_firstV;
            delete dfa_secondH;
            delete dfa_secondV;
        }

        Crosswords(const SizeOptions &opt, size_t width, size_t height, const Dictionary &dictionary, const std::vector<int> &let):
            Crosswords(opt, width, height, dictionary)
        {
            size_t i = 0;
            for(size_t y = 0; y < height; ++y)
            {
                for(size_t x = 0; x < width; ++x)
                {
                    char c = let[i];
                    if(c >= 'a' && c <= 'z')
                        rel(*this, letters[i], IRT_EQ, c);
                    else if(c == '#')
                        rel(*this, letters[i], IRT_EQ, 'z'+1);
                    ++i;
                }
            }
        }

        Crosswords(Crosswords &crosswords):
            Script(crosswords),
            width(crosswords.width),
            height(crosswords.height)
        {
            letters.update(*this, crosswords.letters);

            indBH.update(*this, crosswords.indBH);
            indBV.update(*this, crosswords.indBV);

            ind1H.update(*this, crosswords.ind1H);
            ind2H.update(*this, crosswords.ind2H);
            ind1V.update(*this, crosswords.ind1V);
            ind2V.update(*this, crosswords.ind2V);

            wordPos1H.update(*this, crosswords.wordPos1H);
            wordPos2H.update(*this, crosswords.wordPos2H);
            wordPos1V.update(*this, crosswords.wordPos1V);
            wordPos2V.update(*this, crosswords.wordPos2V);

            wordLen1H.update(*this, crosswords.wordLen1H);
            wordLen1V.update(*this, crosswords.wordLen1V);
        }

        virtual Space *copy(void)
        {
            return new Crosswords(*this);
        }

        virtual void print(std::ostream &os) const
        {
            for(size_t y = 0; y < height; ++y)
            {
                for(size_t x = 0; x < width; ++x)
                {
                    auto var = letters[x+y*width];
                    if(var.min() != var.max())
                        os << '?';
                    else
                    {
                        char c = var.val();
                        os << c;
                    }
                }
                os << std::endl;
            }
            os << std::endl;
        }

        void extract(std::vector<int> &out) const
        {
            size_t i = 0;
            for(const auto &l : letters)
                out[i++] = l.val();
        }

        static bool solve(size_t width, size_t height, std::vector<int> &letters, const std::vector<std::string> &wordsToAdd)
        {
            size_t maxdim = (width > height ? width : height);

            Dictionary dictionary("dict", maxdim);
            dictionary.AddWords(wordsToAdd, maxdim);

            SizeOptions opt("Crosswords");
            opt.solutions(0);

            Crosswords model(opt, width, height, dictionary, letters);
            Search::Options o;
            Search::Cutoff *c = Search::Cutoff::constant(200000);
            o.cutoff = c;
            o.threads = 4;
            RBS<Crosswords, DFS> e(&model, o);
            bool ret = false;
            if(auto *p = e.next())
            {
                p->extract(letters);
                ret = true;
            }

            return ret;
        }

    protected:
        size_t width;
        size_t height;
        IntVarArray letters;

        IntVarArray indBH;
        IntVarArray indBV;

        IntVarArray ind1H;
        IntVarArray ind2H;
        IntVarArray ind1V;
        IntVarArray ind2V;

        IntVarArray wordPos1H;
        IntVarArray wordPos2H;
        IntVarArray wordPos1V;
        IntVarArray wordPos2V;

        IntVarArray wordLen1H;
        IntVarArray wordLen1V;
};

#endif

