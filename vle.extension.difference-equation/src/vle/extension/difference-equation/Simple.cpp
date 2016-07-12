/*
 * @file vle/extension/difference-equation/Simple.cpp
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


#include <vle/extension/difference-equation/Simple.hpp>
#include <vle/value/Set.hpp>

namespace vle { namespace extension { namespace DifferenceEquation {

using namespace devs;
using namespace value;

Simple::Simple(const DynamicsInit& model,
               const InitEventList& events,
               bool control) :
    Base(model, events, control), mPortName("update"),
    mInitValue(false), mSize(DEFAULT_SIZE)
{
    if (events.exist("name")) {
        mVariableName = toString(events.get("name"));
    } else {
        if (getModel().existOutputPort("update") or
            getModel().getOutputPortNumber() == 0) {
            mVariableName = getModelName();
        } else {

            if (getModel().getOutputPortNumber() > 1) {
                throw utils::ModellingError(
                    vle::utils::format("[%s] DifferenceEquation::Simple: invalid "  \
                          "number of output port", getModelName().c_str()));
            }

            mVariableName = getModel().getOutputPortList().begin()->first;
            mPortName = mVariableName;
        }
    }

    if (events.exist("value")) {
        mInitialValue = toDouble(events.get("value"));
        mInitValue = true;
    }

    if (events.exist("init")) {
        const value::Set& init = events.getSet("init");
        unsigned int i;

        for (i = 0; i < init.size(); ++i) {
            mValues.push_front(toDouble(init.get(i)));
        }
    }
}

void Simple::addValue(double value, const std::string& /* name */)
{
    mValues.push_front(value);
    while (mSize > 0
        and mValues.size() > (unsigned int)mSize) {
        mValues.pop_back();
    }
    mSetValue = true;
}

void Simple::initValues(const vle::devs::Time& time)
{
    if (mInitValue) {
        addValue(mInitialValue, mVariableName);
    } else {
        addValue(initValue(time), mVariableName);
        mInitValue = true;
    }
}

void Simple::removeValue(const std::string& /* name */)
{
    mValues.pop_front();
}

void Simple::removeValues()
{
    mValues.pop_front();
}

void Simple::updateValues(const Time& time)
{
    mSetValue = false;
    addValue(compute(time), mVariableName);
}

void Simple::size(int size)
{
    if (size == 0) {
        throw utils::ModellingError(vle::utils::format(
                    "[%s] DifferenceEquation::size - not null size",
            getModelName().c_str()));
    }

    mSize = size;
}

double Simple::val() const
{
    if (not mSetValue) {
        throw utils::InternalError(vle::utils::format(
                "[%s] DifferenceEquation::getValue - forbidden to use " \
                "%s() before computing of %s",
                getModelName().c_str(), mVariableName.c_str(),
                mVariableName.c_str()));
    }

    return mValues.front();
}

double Simple::val(int shift) const
{
    if (shift > 0) {
        throw utils::ModellingError(vle::utils::format(
                    "[%s] DifferenceEquation::getValue - positive shift on %s",
                    getModelName().c_str(), mVariableName.c_str()));
    }

    if (shift == 0 and not mSetValue) {
        throw utils::InternalError(vle::utils::format(
                "[%s] DifferenceEquation::getValue - forbidden to use " \
                  "%s() before computing of %s",
                  getModelName().c_str(), mVariableName.c_str(),
                  mVariableName.c_str()));
    }

    ++shift;
    if (shift == 0) {
        return mValues.front();
    } else {
        if ((int)(mValues.size() - 1) < -shift) {
            throw utils::InternalError(vle::utils::format(
                        "[%s] - %s[%i] - shift too large",
                getModelName().c_str(), mVariableName.c_str(), shift));
        }

        return mValues[-shift];
    }
}

void Simple::output(Time /* time */,
                    ExternalEventList& output) const
{
    if (mState == SEND_INIT and not mValues.empty()) {
        output.emplace_back(mPortName);
        vle::value::Map& attrs = output.back().addMap();
        attrs.addString("name",mVariableName);
        Set& values = attrs.addSet("init");

        std::deque < double>::const_iterator it = mValues.begin();

        while (it != mValues.end()) {
            values.addDouble(*it++);
        }
    } else {
        if (mActive and (mState == PRE_INIT or mState == PRE_INIT2
                         or mState == POST or mState == POST2)) {
            output.emplace_back(mPortName);
            vle::value::Map& attrs = output.back().addMap();
            attrs.addString("name",mVariableName);
            attrs.addDouble("value",val());
        }
    }
}

std::unique_ptr<vle::value::Value>
Simple::observation(const ObservationEvent& event) const
{
    if (mState == INIT) {
        throw utils::InternalError(
            vle::utils::format("[%s] DifferenceEquation::Simple: model not initialized" \
                  " (perhaps, a cycle of synchronous variables)",
            getModelName().c_str()));
    }

    if (event.getPortName() != mVariableName) {
        throw utils::InternalError(vle::utils::format(
                    "[%s] DifferenceEquation::observation: invalid variable"
                    " name: %s",
                    getModelName().c_str(), event.getPortName().c_str()));
    }
    return Double::create(val());
}

}}} // namespace vle extension DifferenceEquation
