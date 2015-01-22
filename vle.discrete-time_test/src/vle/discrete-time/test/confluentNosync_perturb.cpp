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
 * @@tagdepends: @@endtagdepends
 */

#include <vle/devs/DynamicsDbg.hpp>
#include <iostream>




namespace vle {
namespace discrete_time {
namespace test {

class confluentNosync_perturb : public vle::devs::Dynamics
{
public:
    enum State {BeforeOutput, Output, AfterOutput};
    confluentNosync_perturb(const vle::devs::DynamicsInit& model,
        const vle::devs::InitEventList& events):
         vle::devs::Dynamics(model,events)
    {
    }
    ~confluentNosync_perturb()
    {
    }
    virtual vle::devs::Time init(const vle::devs::Time& /* time */)
    {
        return 1.0;
    }

    vle::devs::Time timeAdvance() const
    {
        return vle::devs::infinity;
    }

    void output(const vle::devs::Time& /* time */,
        vle::devs::ExternalEventList& output) const
    {
        vle::devs::ExternalEvent* ev = new vle::devs::ExternalEvent("y_nosync");
        ev->putAttribute("value", new vle::value::String("y_nosync"));
        ev->putAttribute("value", new vle::value::Double(10));
        output.push_back(ev);
    }
};

DECLARE_DYNAMICS_DBG(confluentNosync_perturb)

}}}


