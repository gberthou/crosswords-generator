#ifndef PROP_MANDATORY_HPP
#define PROP_MANDATORY_HPP

#include <iostream>
#include <vector>

#include <gecode/int.hh>

typedef std::vector<Gecode::Int::IntView> VIntView;

class PropMandatory : public Gecode::Propagator
{
    public:
        PropMandatory(Gecode::Space &home, const std::vector<int> &aindices, const VIntView &avindices):
            Gecode::Propagator(home),
            indices(aindices),
            vindices(avindices)
        {
            for(auto &vi : vindices)
                vi.subscribe(home, *this, Gecode::Int::PC_INT_DOM);
        }

        PropMandatory(Gecode::Space &home, PropMandatory &p):
            Gecode::Propagator(home, p),
            indices(p.indices),
            vindices(p.vindices)
        {
            for(size_t i = 0; i < vindices.size(); ++i)
                vindices[i].update(home, p.vindices[i]);
        }

        virtual Gecode::ExecStatus propagate(Gecode::Space &home, const Gecode::ModEventDelta &)
        {
            // Not optimized af, but you know...
            
            // Too many memory operations
            std::vector<std::vector<Gecode::Int::IntView> > wordPresent(indices.size());

            for(auto &view : vindices)
            {
                if(view.assigned())
                {
                    int val = view.val();
                    for(size_t i = 0; i < indices.size(); ++i)
                        if(val == indices[i])
                        {
                            wordPresent[i].push_back(view);
                            break;
                        }
                }
                else
                {
                    for(Gecode::Int::ViewRanges<Gecode::Int::IntView> j(view); j(); ++j)
                    {
                        int min = j.min();
                        int max = j.max();

                        for(size_t i = 0; i < indices.size(); ++i)
                        {
                            int indice = indices[i];
                            if(indice >= min && indice <= max)
                                wordPresent[i].push_back(view);
                        }
                    }
                }
            }

            bool subsumed = true;
            for(size_t i = 0; i < indices.size(); ++i)
            {
                auto &wp = wordPresent[i];
                size_t size = wp.size();

                if(size == 0)
                    return Gecode::ES_FAILED;
                if(size == 1)
                    GECODE_ME_CHECK(wp[0].eq(home, indices[i]));
                else
                    subsumed = false;
            }

            return subsumed ? home.ES_SUBSUMED(*this) : Gecode::ES_FIX;
        }

        virtual size_t dispose(Gecode::Space &home)
        {
            for(auto &vi : vindices)
                vi.cancel(home, *this, Gecode::Int::PC_INT_DOM);
            (void) Gecode::Propagator::dispose(home);
            return sizeof(*this);
        }

        virtual Gecode::Propagator *copy(Gecode::Space &home)
        {
            return new (home) PropMandatory(home, *this);
        }

        virtual Gecode::PropCost cost(const Gecode::Space &, const Gecode::ModEventDelta &) const
        {
            return Gecode::PropCost::linear(Gecode::PropCost::LO, 1);
        }

        virtual void reschedule(Gecode::Space &home)
        {
            for(auto &vi : vindices)
                vi.reschedule(home, *this, Gecode::Int::PC_INT_DOM);
        }

        static Gecode::ExecStatus post(Gecode::Space &home, const std::vector<int> &indices, const Gecode::IntVarArgs &ind)
        {
            /* For now, assume that indices are not "same"
            if(indices.same())
                return Gecode::ES_FAILED;
            */

            VIntView vindices;
            for(auto &i : ind)
                vindices.push_back(i);

            (void) new (home) PropMandatory(home, indices, vindices);
            return Gecode::ES_OK;
        }

        static void propmandatory(Gecode::Space &home, const std::vector<int> &indices, const Gecode::IntVarArgs &ind)
        {
            if(PropMandatory::post(home, indices, ind) != Gecode::ES_OK)
                home.fail();
        }

    protected:
        const std::vector<int> &indices;
        VIntView vindices;
};

#endif

