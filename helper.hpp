#ifndef HELPER_HPP
#define HELPER_HPP

#include <vector>

class FakeView
{
    public:
        explicit FakeView(const std::vector<size_t> &data):
            data(data),
            it(data.cbegin())
        {
        }

        bool operator()()
        {
            return it != data.end();
        }

        FakeView &operator++()
        {
            ++it;
            return *this;
        }

        int val()
        {
            return *it;
        }

    protected:
        const std::vector<size_t> &data;
        std::vector<size_t>::const_iterator it;
};

#endif

