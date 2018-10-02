#include <iostream>
#include <ctime>
#include <vector>
#include <set>
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

static DFA * dfa_noindex;

static std::mutex cout_mutex;

static bool redundant_word(const std::vector<std::string> &words)
{
    std::set<std::string> tmp;
    for(const auto word : words)
        if(!tmp.insert(word).second)
            return true;
    return false;
}

class Crosswords: public Script
{
    public:
        Crosswords(const SizeOptions &opt, size_t width, size_t height, const std::vector<DFA*> &mandatory = std::vector<DFA*>()):
            Script(opt),
            width(width),
            height(height),

            letters(*this, width * height, 'a', 'z'+1) // Letters go from 'a' to 'z', and black tile is 'z'+1 ('{')
        {
            // Fewer black tiles than X
            count(*this, letters, 'z'+1, IRT_LQ, 10); // <= 10

            IntVarArgs allwords;
            IntVar dummy(*this, 'z'+1, 'z'+1);
            for(size_t y = 0; y < height; ++y)
                allwords = allwords + letters.slice(y*width, 1, width) + dummy;
            for(size_t x = 0; x < width; ++x)
                allwords = allwords + letters.slice(x, width, height) + dummy;
            unshare(*this, allwords);

            for(const auto dfa : mandatory)
                extensional(*this, allwords, *dfa);

            for(size_t y = 0; y < height; ++y)
            {
                IntVarArgs row = letters.slice(y*width, 1, width);
                extensional(*this, row, *dfa_noindex);
            }

            for(size_t x = 0; x < width; ++x)
            {
                IntVarArgs col = letters.slice(x, width, height);
                extensional(*this, col, *dfa_noindex);
            }

            auto seed = std::time(nullptr);
            branch(*this, letters, INT_VAR_RND(seed), INT_VAL_RND(seed));
        }

        Crosswords(Crosswords &crosswords):
            Script(crosswords),
            width(crosswords.width),
            height(crosswords.height)
        {
            letters.update(*this, crosswords.letters);
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

            if(redundant_word(words))
                std::cout << std::endl << "Warning: Redundant words!"<< std::endl;
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
};

/*
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
*/

void run_single(size_t nthreads, std::vector<DFA*> dfas = std::vector<DFA*>())
{
    SizeOptions opt("Crosswords");
    opt.solutions(0);

    Crosswords model(opt, WIDTH, HEIGHT, dfas);
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

/*
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
*/

int main(void)
{
    std::vector<std::string> mandatoryWords;
    std::vector<DFA*> mandatoryDFAs;
    dictionary.AddMandatoryWords("mandatory", HEIGHT, mandatoryWords);

    DictionaryDFA dictDFA(dictionary, WIDTH, HEIGHT);

    std::cout << "DFA conversion start..." << std::endl;

    for(const auto word : mandatoryWords)
        mandatoryDFAs.push_back(DictionaryDFA::Mandatory(word));

    dfa_noindex = dictDFA.NoIndex();

    std::cout << "DFA conversion done!" << std::endl;

    /*
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
    */
    run_single(4, mandatoryDFAs);

    delete dfa_noindex;

    for(auto dfa : mandatoryDFAs)
        delete dfa;

    return EXIT_SUCCESS;
}

