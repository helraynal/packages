/*
 * @file vle/examples/smartgardener/DifferenceEquationAdaptor.cpp
 *
 * This file is part of VLE, a framework for multi-modeling, simulation
 * and analysis of complex dynamical systems
 * http://www.vle-project.org
 *
 * Copyright (c) 2003-2007 Gauthier Quesnel <quesnel@users.sourceforge.net>
 * Copyright (c) 2003-2011 ULCO http://www.univ-littoral.fr
 * Copyright (c) 2007-2011 INRA http://www.inra.fr
 *
 * See the AUTHORS or Authors.txt file for copyright owners and contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * VLE Environment - the multimodeling and simulation environment
 * This file is a part of the VLE environment (http://vle.univ-littoral.fr)
 * Copyright (C) 2003 - 2008 The VLE Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @@tagdynamic@@
 */

#include <vle/utils/Tools.hpp>
#include <vle/devs/Dynamics.hpp>


namespace vle { namespace examples { namespace smartgardeners {

class DifferenceEquationAdaptor : public vle::devs::Dynamics
{
    typedef std::map < std::string, double > values;

public:
    DifferenceEquationAdaptor(const vle::devs::DynamicsInit& init,
                              const vle::devs::InitEventList& events) :
    vle::devs::Dynamics(init, events), mPhase(INIT), mSigma(0),
    mLastTime(0), mStepNumber(0)
    {
        mTimeStep = vle::value::toDouble(events.get("time-step"));
    }

    virtual ~DifferenceEquationAdaptor()
    { }

    virtual vle::devs::Time init(vle::devs::Time time) override
    {
        {
            const vle::vpz::ConnectionList& in_list =
                getModel().getInputPortList();

            for (vle::vpz::ConnectionList::const_iterator it =
                     in_list.begin(); it != in_list.end(); ++it) {
                if (not getModel().existOutputPort(it->first)) {
                    throw utils::InternalError(
                        vle::utils::format("[%s] DifferenceEquation adaptor - "   \
                                "%s output port is missing",
                        getModelName().c_str(), it->first.c_str()));
                }
                mValues[it->first] = 0.;
            }
        }

        {
            const vle::vpz::ConnectionList& out_list =
                getModel().getOutputPortList();

            for (vle::vpz::ConnectionList::const_iterator it =
                     out_list.begin(); it != out_list.end(); ++it) {
                if (not getModel().existInputPort(it->first))
                    throw utils::InternalError(
                        vle::utils::format("[%s] DifferenceEquation adaptor - "   \
                                "%s input port is missing or "         \
                                "%s output port is unused",
                        getModelName().c_str(), it->first.c_str(),
                        it->first.c_str()));
            }
        }

        mPhase = INIT;
        mSigma = 0;
        mLastTime = time;
        mStepNumber = 0;
        return vle::devs::infinity;
    }

    virtual void output(vle::devs::Time /* time */,
			vle::devs::ExternalEventList& output) const override
    {
        if (mPhase == SEND) {
            for (values::const_iterator it = mValues.begin();
                 it != mValues.end(); ++it) {
                output.emplace_back(it->first);
                value::Map& m = output.back().addMap();
                m.addString("name",it->first);
                m.addDouble("value", it->second);
            }
        }
    }

    virtual vle::devs::Time timeAdvance() const override
    {
        return mSigma;
    }

    virtual void internalTransition(vle::devs::Time time) override
    {
        if (mPhase == STEP) {
            ++mStepNumber;
            mPhase = SEND;
            mSigma = 0;
        } else if (mPhase == SEND) {
            mPhase = STEP;
            mSigma = mTimeStep;
        }
        mLastTime = time;
    }

    virtual void externalTransition(
            const vle::devs::ExternalEventList& events,
            vle::devs::Time time) override
    {
        vle::devs::ExternalEventList::const_iterator it = events.begin();

        while (it != events.end()) {
            if (mPhase == INIT or mPhase == STEP or mPhase == SEND) {
                std::string name = it->getPortName();
                values::iterator itv = mValues.find(name);
                const value::Map& attrs = it->attributes()->toMap();
                if (itv != mValues.end() and attrs.getString("name") == name) {
                    itv->second = attrs.getDouble("value");
                } else {
                    throw utils::InternalError(
                        vle::utils::format("[%s] DifferenceEquation adaptor - "     \
                                "wrong variable name on %s port: %s",
                        getModelName().c_str(), name.c_str(),
                        attrs.getString("name").c_str()));
                }

                if (mPhase == INIT) {
                    mPhase = SEND;
                    mSigma = 0;
                } else {
                    if (mSigma > 0) {
                        mSigma -= time - mLastTime;

                        double error = (time + mSigma) -
                            ((mStepNumber + 1) * mTimeStep);

                        if (error > 0 and error < 1e-12) {
                            mSigma -= 1.001 * error;
                        }
                    }
                }
            }
            ++it;
        }
        mLastTime = time;
    }

    virtual void confluentTransitions(
        vle::devs::Time time,
        const vle::devs::ExternalEventList& extEventlist) override
    {
        externalTransition(extEventlist, time);
        internalTransition(time);
    }

    virtual std::unique_ptr<vle::value::Value> observation(
        const vle::devs::ObservationEvent& event) const override
    {
        values::const_iterator it = mValues.find(event.getPortName());

        if (it != mValues.end()) {
            return vle::value::Double::create(it->second);
        } else {
            return 0;
        }
    }

    enum phase { INIT, STEP, SEND };

    // parameters
    double mTimeStep;

    // state
    phase mPhase;
    vle::devs::Time mSigma;
    vle::devs::Time mLastTime;
    unsigned long mStepNumber;

    values mValues;
};

}}} // namespaces

DECLARE_DYNAMICS(vle::examples::smartgardeners::DifferenceEquationAdaptor)
