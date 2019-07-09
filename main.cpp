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

static size_t maxWordCount(size_t dimension)
{
    if(dimension < 2)
        return 1;
    return (dimension + 1) / 3;
}

class Crosswords: public Script
{
    public:
        Crosswords(const SizeOptions &opt, size_t width, size_t height, bool fancyBorders, const std::vector<int> &orderedMandatory = std::vector<int>()):
            Script(opt),
            width(width),
            height(height),

            letters(*this, width * height, 'a', 'z'+1), // Letters go from 'a' to 'z', and black tile is 'z'+1 ('{')

            ind1H(*this, height, dictionary.FirstIndexOfLength(2), dictionary.LastIndexOfLength(width)),
            ind2H(*this, height, MIN_INDEX, dictionary.LastIndexOfLength(width-3)),
            ind1V(*this, width, dictionary.FirstIndexOfLength(2), dictionary.LastIndexOfLength(height)),
            ind2V(*this, width, MIN_INDEX, dictionary.LastIndexOfLength(height-3)),
            
            wordPos1H(*this, height, 0, 2),
            wordPos2H(*this, height, 3, width+1),
            wordPos1V(*this, width, 0, 2),
            wordPos2V(*this, width, 3, height+1),

            wordLen1H(*this, height, 2, width),
            wordLen1V(*this, width, 2, height)
        {
            // Fewer black tiles than X
            count(*this, letters, 'z'+1, IRT_LQ, 10); // <= 10

            auto allIndices = ind1H+ind2H+ind1V+ind2V;

            distinct(*this, allIndices, MIN_INDEX);

            /* Borders */
            if(fancyBorders)
            {
                // First row
                rel(*this, wordPos1H[0], IRT_EQ, 0);
                rel(*this, wordLen1H[0], IRT_EQ, width);

                // Last row
                rel(*this, wordPos1H[height-1], IRT_EQ, 0);
                rel(*this, wordLen1H[height-1], IRT_EQ, width);

                // First column
                rel(*this, wordPos1V[0], IRT_EQ, 0);
                rel(*this, wordLen1V[0], IRT_EQ, height);

                // Last column
                rel(*this, wordPos1V[width-1], IRT_EQ, 0);
                rel(*this, wordLen1V[width-1], IRT_EQ, height);
            }

            // Impose mandatory words
            for(size_t i = 0; i < orderedMandatory.size(); ++i)
            {
                int index = orderedMandatory[i];
                if(index)
                    rel(*this, allIndices[i], IRT_EQ, index);
            }

            // Horizontal words
            for(size_t y = 0; y < height; ++y)
            {
                // First words
                IntVarArgs row = letters.slice(y*width, 1, width);
                extensional(*this, wordPos1H[y] + row + ind1H[y] + wordLen1H[y], *dfa_firstH);

                // Second words
                IntVarArgs reducedRow = letters.slice(y*width+3, 1, width-3);
                extensional(*this, wordPos2H[y] + reducedRow + ind2H[y], *dfa_secondH);

                // wp2H[y] == wp1H[y] + wl1H[y] + 1
                // wp2H[y] * 1 + wp1H[y] * (-1) + wl1H[y] * (-1) == 1
                linear(*this, std::vector<int> {1, -1, -1}, std::vector<IntVar> {wordPos2H[y], wordPos1H[y], wordLen1H[y]}, IRT_EQ, 1);
            }
            
            // Vertical words
            for(size_t x = 0; x < width; ++x)
            {
                // First words
                IntVarArgs col = letters.slice(x, width, height);
                extensional(*this, wordPos1V[x] + col + ind1V[x] + wordLen1V[x], *dfa_firstV);

                // Second words
                IntVarArgs reducedCol = letters.slice(x+3*width, width, height-3);
                extensional(*this, wordPos2V[x] + reducedCol + ind2V[x], *dfa_secondV);

                // wp2V[x] == wp1V[x] + wl1V[x] + 1
                // wp2V[y] * 1 + wp1V[x] * (-1) + wl1V[x] * (-1) == 1
                linear(*this, std::vector<int> {1, -1, -1}, std::vector<IntVar> {wordPos2V[x], wordPos1V[x], wordLen1V[x]}, IRT_EQ, 1);
            }

            auto seed = std::time(nullptr);
            branch(*this, ind1H+ind1V, INT_VAR_SIZE_MIN(), INT_VAL_RND(seed));
            branch(*this, ind2H+ind2V, INT_VAR_NONE(), INT_VAL_RND(seed));

            branch(*this, wordPos1H+wordPos1V, INT_VAR_NONE(), INT_VAL_MIN());
        }

        Crosswords(Crosswords &crosswords):
            Script(crosswords),
            width(crosswords.width),
            height(crosswords.height)
        {
            letters.update(*this, crosswords.letters);

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

            std::vector<std::string> words;
            wordlist(words);
            for(const auto word : words)
                std::cout << word << std::endl;
        }
        
        virtual void wordlist(std::vector<std::string> &words) const
        {
            words.clear();

            for(size_t y = 0; y < height; ++y)
            {
                std::string tmp = "";
                for(size_t x = 0; x < width; ++x)
                {
                    auto var = letters[x+y*width];
                    if(var.min() != var.max())
                        tmp += '?';
                    else if(var.val() != 'z'+1)
                        tmp += var.val();
                    else
                    {
                        if(tmp.size() >= 2)
                            words.push_back(tmp);
                        tmp = "";
                    }
                }
                if(tmp.size() >= 2)
                    words.push_back(tmp);
            }

            for(size_t x = 0; x < width; ++x)
            {
                std::string tmp = "";
                for(size_t y = 0; y < height; ++y)
                {
                    auto var = letters[x+y*width];
                    if(var.min() != var.max())
                        tmp += '?';
                    else if(var.val() != 'z'+1)
                        tmp += var.val();
                    else
                    {
                        if(tmp.size() >= 2)
                            words.push_back(tmp);
                        tmp = "";
                    }
                }

                if(tmp.size() >= 2)
                    words.push_back(tmp);
            }
        }

    protected:
        size_t width;
        size_t height;
        IntVarArray letters;

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

bool permutation_valid(size_t width, size_t height, bool fancyBorders, const std::vector<int> &indices)
{
    size_t wordCountH = maxWordCount(width);

    // Assumes order: ind1H(H) + ind2H(H) + ...
    //              + ind1V(W) + ind2V(W) + ...;

    std::cout << indices.size() << " indices" << std::endl;

    for(size_t i = 0; i < indices.size(); ++i)
    {
        int index = indices[i];
        if(index)
        {
            std::string word = dictionary.GetWord(index);
            size_t size = word.size();

            if(fancyBorders)
            {
                if((i == 0 || i == height-1) && size != width)
                    return false;
                if((i == wordCountH*height || i == wordCountH*height+width-1) && size != height)
                    return false;
            }

            if(i < wordCountH*height) // indXH
            {
                size_t x = i / wordCountH;
                for(size_t j = i - height; x--; j -= height)
                {
                    int other = indices[j];
                    if(other)
                        size += dictionary.GetWord(other).size() + 1;
                }

                if(size > width)
                    return false;
            }
            else // indXV
            {
                size_t x = (i - wordCountH*height) / width;

                size += 3*x;
                for(size_t j = i - height; x--; j -= height)
                {
                    int other = indices[j];
                    if(other)
                        size += dictionary.GetWord(other).size() + 1;
                }
                if(size > height)
                    return false;
            }
        }
    }
    
    return true;
}

void run_single(size_t nthreads, bool fancyBorders, std::vector<int> indices = std::vector<int>())
{

    SizeOptions opt("Crosswords");
    opt.solutions(0);

    Crosswords model(opt, WIDTH, HEIGHT, fancyBorders, indices);
    Search::Options o;
    Search::Cutoff *c = Search::Cutoff::constant(70000);
    o.cutoff = c;
    o.threads = nthreads;
    RBS<Crosswords, DFS> e(&model, o);
    if(auto *p = e.next())
    {
        cout_mutex.lock();
        p->print(std::cout);
        cout_mutex.unlock();
    }
}

void run_single_mandatory(bool fancyBorders, const std::vector<int> &indices)
{
    if(!permutation_valid(WIDTH, HEIGHT, fancyBorders, indices))
        return;
    run_single(1, fancyBorders, indices);
}

void run_concurrently(bool fancyBorders, std::vector<int> indices, size_t nthreads, size_t id)
{
    size_t i = 0;
    do
    {
        if((i%nthreads) != id)
        {
            ++i;
            continue;
        }

        run_single_mandatory(fancyBorders, indices);
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
    const bool FANCY_BORDERS = false;

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
        size_t wordCountH = maxWordCount(WIDTH);
        size_t wordCountV = maxWordCount(HEIGHT);

        const size_t wordCount = wordCountH*HEIGHT + wordCountV*WIDTH;
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
            pool.emplace_back(run_concurrently, FANCY_BORDERS, indices, 4, i);
        for(auto &thread : pool)
            thread.join();
    }
    else
        run_single(4, FANCY_BORDERS);

    delete dfa_borderH;
    delete dfa_borderV;
    delete dfa_firstH;
    delete dfa_firstV;
    delete dfa_secondH;
    delete dfa_secondV;

    return EXIT_SUCCESS;
}

