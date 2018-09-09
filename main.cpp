#include <iostream>
#include <ctime>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>

#include <gecode/driver.hh>
#include <gecode/int.hh>

#include "dictionary.hpp"
#include "dfa.hpp"

using namespace Gecode;

const size_t WIDTH = 9;
const size_t HEIGHT = 11;
static Dictionary dictionary("dict", HEIGHT);

static DFA * dfa_borderH;
static DFA * dfa_borderV;
static DFA * dfa_firstH;
static DFA * dfa_firstV;
static DFA * dfa_secondH;
static DFA * dfa_secondV;

static std::mutex cout_mutex;

static size_t scrabble_french(char c)
{
    switch(c)
    {
        case 'a':
        case 'e':
        case 'i':
        case 'l':
        case 'n':
        case 'o':
        case 'r':
        case 's':
        case 't':
        case 'u':
            return 1;

        case 'd':
        case 'g':
        case 'm':
            return 2;

        case 'b':
        case 'c':
        case 'p':
            return 3;

        case 'f':
        case 'h':
        case 'v':
            return 4;

        case 'j':
        case 'q':
            return 8;

        case 'k':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
            return 10;

    }
    return 0;
}

class Crosswords: public Script
{
    public:
        Crosswords(const SizeOptions &opt, size_t width, size_t height, const std::vector<int> &orderedMandatory = std::vector<int>()):
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
            // Fewer black tiles than X
            //count(*this, letters, 'z'+1, IRT_LQ, 10); // <= 10

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
            for(size_t i = 0; i < orderedMandatory.size(); ++i)
            {
                int index = orderedMandatory[i];
                if(index)
                    rel(*this, allIndices[i], IRT_EQ, index);
            }

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

            auto seed = std::time(nullptr);
            branch(*this, indBH+indBV+ind1H+ind1V, INT_VAR_SIZE_MIN(), INT_VAL_RND(seed));
            branch(*this, ind2H+ind2V, INT_VAR_NONE(), INT_VAL_RND(seed));

            branch(*this, wordPos1H+wordPos1V, INT_VAR_NONE(), INT_VAL_MIN());
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

        virtual void constrain(const Space &_base)
        {
            const Crosswords &base = static_cast<const Crosswords&>(_base);
            size_t btc = base.black_tile_count();
            size_t score = base.scrabble_score();

            std::cout << "[" << btc << ", " << score << "]" << std::endl;
            
            count(*this, letters, 'z'+1, IRT_LE, btc);
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

        size_t black_tile_count() const
        {
            size_t count = 0;
            for(size_t i = 0; i < width * height; ++i)
                count += (letters[i].val() == 'z' + 1);
            return count;
        }

        size_t scrabble_score() const
        {
            size_t score = 0;
            for(size_t i = 0; i < width * height; ++i)
                score += scrabble_french(letters[i].val());
            return score;
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

bool permutation_valid(size_t width, size_t height, const std::vector<int> &indices)
{
    // Assumes order: indBH(2) + indBV(2) + ind1H(H-2) + ind2H(H-2)
    //              + ind1V(W-2) + ind2V(W-2);

    for(size_t i = 0; i < indices.size(); ++i)
    {
        int index = indices[i];
        if(index)
        {
            std::string word = dictionary.GetWord(index);
            size_t size = word.size();

            if(i < 2)
            {
                if(size != width)
                    return false;
            }
            else if(i < 4)
            {
                if(size != height)
                    return false;
            }
            else if(i < 2 + height)
            {
                if(size > width)
                    return false;
                
                int other = indices[i + height - 2];
                if(other && size+dictionary.GetWord(other).size()+1 > width)
                    return false;
            }
            else if(i < 2*height)
            {
                if(size + 3 > width)
                    return false;
            }
            else if(i < 2*height + width - 2)
            {
                if(size > height)
                    return false;
                
                int other = indices[i + width - 2];
                if(other && size+dictionary.GetWord(other).size()+1 > height)
                    return false;
            }
            else // i < 2*(width+height-2)
            {
                if(size + 3 > height)
                    return false;
            }
        }
    }
    
    return true;
}

void run_single(size_t nthreads, std::vector<int> indices = std::vector<int>())
{

    SizeOptions opt("Crosswords");
    opt.solutions(0);

    Crosswords model(opt, WIDTH, HEIGHT, indices);
    Search::Options o;
    //Search::Cutoff *c = Search::Cutoff::constant(70000);
    //o.cutoff = c;
    o.threads = nthreads;
    //RBS<Crosswords, DFS> e(&model, o);
    //RBS<Crosswords, BAB> e(&model, o);
    BAB<Crosswords> e(&model, o);
    if(auto *p = e.next())
    {
        cout_mutex.lock();
        p->print(std::cout);
        cout_mutex.unlock();
    }
}

void run_single_mandatory(const std::vector<int> &indices)
{
    if(!permutation_valid(WIDTH, HEIGHT, indices))
        return;
    run_single(1, indices);
}

void run_concurrently(std::vector<int> indices, size_t nthreads, size_t id)
{
    size_t i = 0;
    do
    {
        if((i%nthreads) != id)
        {
            ++i;
            continue;
        }

        run_single_mandatory(indices);
        ++i;
    } while(std::prev_permutation(indices.begin(), indices.end()));
}

size_t permutation_count(size_t n, size_t k)
{
    size_t a = n-k;

    size_t result = n--;
    for(; n > a; --n)
        result *= n;

    return result;
}

int main(void)
{
    std::vector<int> mandatoryIndices;
    dictionary.AddMandatoryWords("mandatory", HEIGHT, mandatoryIndices);

    DictionaryDFA dictDFA(dictionary, WIDTH, HEIGHT);

    std::cout << "DFA conversion start..." << std::endl;

    dfa_borderH = dictDFA.BorderH();
    dfa_borderV = dictDFA.BorderV();
    dfa_firstH = dictDFA.FirstH();
    dfa_firstV = dictDFA.FirstV();
    dfa_secondH = dictDFA.SecondH();
    dfa_secondV = dictDFA.SecondV();

    std::cout << "DFA conversion done!" << std::endl;

    if(mandatoryIndices.size())
    {
        size_t wordCount = 4 // Borders
                         + 2*(WIDTH+HEIGHT-4); // 2 words per col/row
        if(mandatoryIndices.size() > wordCount)
        {
            // TODO Error
        }

        std::cout << permutation_count(wordCount, mandatoryIndices.size()) << " permutations" << std::endl;

        std::vector<int> indices(wordCount, 0);
        std::copy(mandatoryIndices.begin(), mandatoryIndices.end(), indices.begin());
        std::sort(indices.begin(), indices.end(), std::greater<int>());

        std::vector<std::thread> pool;
        for(size_t i = 0; i < 4; ++i)
            pool.emplace_back(run_concurrently, indices, 4, i);
        for(auto &thread : pool)
            thread.join();
    }
    else
    {
        for(size_t i = 0; i < 10; ++i)
            run_single(4);
    }

    delete dfa_borderH;
    delete dfa_borderV;
    delete dfa_firstH;
    delete dfa_firstV;
    delete dfa_secondH;
    delete dfa_secondV;

    return EXIT_SUCCESS;
}

