/*
 * Copyright (c) 2014-2014 INRA http://www.inra.fr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


/*
 * @@tagdynamic@@
 * @@tagdepends: vle.discrete-time @@endtagdepends
 */

#include <vle/DiscreteTime.hpp>

#include <iostream>




namespace vle {

namespace discrete_time {
namespace test {


class D1dyn : public DiscreteTimeDyn
{
public:
    D1dyn(const vle::devs::DynamicsInit& init, const vle::devs::InitEventList& events)
        : DiscreteTimeDyn(init, events)
    {
    }

    virtual ~D1dyn()
    {
    }

    void compute(const vle::devs::Time& t) override
    {
        VarMono* a = (VarMono*) getVariables().find("a")->second;
        VarMono* c = (VarMono*) getVariables().find("c")->second;
        VarMono* d = (VarMono*) getVariables().find("d")->second;

        //std::cout << " DBG compute D1dyn " << " d(0)=" << d->getVal(t, 0) << " a(0)=" <<a->getVal(t, 0) << " c(0)=" << c->getVal(t, 0) << std::endl;

        d->update(t, d->getVal(t, 0) + a->getVal(t, 0) + c->getVal(t, 0) + 1);
    }
};

}}}

DECLARE_DYNAMICS(vle::discrete_time::test::D1dyn)
