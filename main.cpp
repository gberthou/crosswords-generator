#include <iostream>
#include <ctime>
#include <vector>
#include <set>
#include <algorithm>
#include <thread>
#include <mutex>
#include <cmath>
#include <random>

#include <gecode/driver.hh>
#include <gecode/int.hh>

#include "dictionary.hpp"
#include "dfa.hpp"

struct GridConstraint
{
    std::vector<char> grid; 

    GridConstraint()
    {
    }

    bool trySet(size_t index, char value)
    {
        char c = grid[index];
        if(c == 0)
        {
            grid[index] = value;
            return true;
        }
        else if(c == value)
            return true;
        return false;
    }
};

using namespace Gecode;

const size_t WIDTH = 9;
const size_t HEIGHT = 11;
const size_t COMBINATION_BASE = 4 * (WIDTH + HEIGHT);
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

static bool is_horizontal(size_t rowcol, size_t height)
{
    return rowcol < height;
}

class Crosswords: public Script
{
    public:
        Crosswords(const SizeOptions &opt, size_t width, size_t height, const GridConstraint &constraint = GridConstraint()):
            Script(opt),
            width(width),
            height(height),

            letters(*this, width * height, 'a', 'z'+1) // Letters go from 'a' to 'z', and black tile is 'z'+1 ('{')
        {
            // Fewer black tiles than X
            count(*this, letters, 'z'+1, IRT_LQ,9); // <= 10

            IntVarArgs allwords;
            IntVar dummy(*this, 'z'+1, 'z'+1);
            for(size_t y = 0; y < height; ++y)
                allwords = allwords + letters.slice(y*width, 1, width) + dummy;
            for(size_t x = 0; x < width; ++x)
                allwords = allwords + letters.slice(x, width, height) + dummy;
            unshare(*this, allwords);

            for(size_t i = 0; i < width*height; ++i)
            {
                char c = constraint.grid[i];
                if(c != 0)
                    rel(*this, letters[i], IRT_EQ, constraint.grid[i]);
            }

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
            for(size_t i = 0; i < 16; ++i)
                os << '#';
            os << std::endl;

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
                os << word << std::endl;

            if(redundant_word(words))
                os << std::endl << "Warning: Redundant words!"<< std::endl;
            os << std::endl;
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

static int local2actual(size_t localpos, size_t rowcollength, size_t wordlength)
{
    switch(localpos)
    {
        case 1:
            return 2;

        case 2:
            return rowcollength - wordlength;

        case 3:
            return rowcollength - 2 - wordlength;

        default:
            return 0;
    }
}

static bool combination_valid(size_t width, size_t height, size_t combination, const std::vector<std::string> &mandatory, GridConstraint &constraint)
{
    constraint.grid.clear();
    constraint.grid.resize(width * height, 0);

    for(size_t i = 0; i < mandatory.size(); ++i)
    {
        size_t encodedPosition = (combination % COMBINATION_BASE);
        size_t rowcol          = encodedPosition / 4;
        size_t localpos        = (encodedPosition % 4);
        bool horizontal        = is_horizontal(rowcol, height);
        int actualpos          = local2actual(localpos, horizontal?width:height, mandatory[i].size());
        int endpos             = actualpos + mandatory[i].size(); 

        // Words out of the grid are denied
        if(actualpos < 0 || endpos > static_cast<int>(horizontal?width:height))
            return false;

        size_t index = (horizontal ? rowcol * width + actualpos : rowcol - height + actualpos * width);

        if(actualpos > 0)
        {
            size_t previndex = index - (horizontal?1:width);
            if(!constraint.trySet(previndex, 'z'+1))
                return false;
        }

        // Here no letter can be outside the grid, so no need to test it
        for(size_t j = 0; j < mandatory[i].size(); ++j, index += (horizontal ? 1 : width))
        {
            if(!constraint.trySet(index, mandatory[i][j]))
                return false;
        }

        if(endpos < static_cast<int>(horizontal?width:height))
        {
            if(!constraint.trySet(index, 'z'+1))
                return false;
        }

        combination /= COMBINATION_BASE;
    }

    return true;
}

void run_single(size_t nthreads, GridConstraint constraint = GridConstraint())
{
    SizeOptions opt("Crosswords");
    opt.solutions(0);

    Crosswords model(opt, WIDTH, HEIGHT, constraint);
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

void run_single_mandatory(size_t combination, const std::vector<std::string> &mandatory)
{
    GridConstraint constraint;
    if(!combination_valid(WIDTH, HEIGHT, combination, mandatory, constraint))
        return;
    run_single(1, constraint);
}

void run_concurrently(const std::vector<std::string> &mandatory, const std::vector<size_t> &combinations, size_t nthreads, size_t id)
{
    for(size_t i = 0; i < combinations.size(); ++i)
        if((i%nthreads) == id)
            run_single_mandatory(combinations[i], mandatory);
}

int main(void)
{
    std::vector<std::string> mandatoryWords;
    dictionary.AddMandatoryWords("mandatory", HEIGHT, mandatoryWords);

    DictionaryDFA dictDFA(dictionary, WIDTH, HEIGHT);

    std::cout << "DFA conversion start..." << std::endl;
    dfa_noindex = dictDFA.NoIndex();
    std::cout << "DFA conversion done!" << std::endl;

    if(mandatoryWords.size())
    {
        size_t wordCount = 4 // Borders
                         + 2*(WIDTH+HEIGHT-4); // 2 words per col/row
        if(mandatoryWords.size() > wordCount)
        {
            // TODO Error
        }

        size_t combination_count = pow(COMBINATION_BASE, mandatoryWords.size());
        std::cout << combination_count << " combinations at most" << std::endl;

        std::vector<size_t> combinations(combination_count);
        for(size_t i = 0; i < combination_count; ++i)
            combinations[i] = i;

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(combinations.begin(), combinations.end(), g);

        std::vector<std::thread> pool;
        for(size_t i = 0; i < 4; ++i)
            pool.emplace_back(run_concurrently, mandatoryWords, combinations, 4, i);
        for(auto &thread : pool)
            thread.join();
    }
    else
        run_single(4);

    delete dfa_noindex;

    return EXIT_SUCCESS;
}

