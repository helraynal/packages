
/*
 * Copyright (C) 2015 INRA
 *
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

#include <iostream>
#include <sstream>
#include <cmath>
#include <string>
#include <thread>

#include <vle/vpz/Vpz.hpp>
#include <vle/utils/Tools.hpp>
#include <vle/utils/Exception.hpp>
#include <vle/utils/Package.hpp>
#include <vle/utils/Spawn.hpp>
#include <vle/manager/Simulation.hpp>
#include <vle/manager/Manager.hpp>
#include <vle/value/Double.hpp>
#include <vle/value/Null.hpp>
#include <vle/value/Tuple.hpp>
#include <vle/value/Table.hpp>

#include <vle/reader/vle_results_text_reader.hpp>

#include "MetaManager.hpp"
#include "VleAPIfacilities.hpp"


namespace vle {
namespace recursive {



/***********VlePropagate*************/

VlePropagate::VlePropagate(const std::string& _cond, const std::string& _port):
                cond(_cond), port(_port)
{
    if (cond.empty() or port.empty()) {
        throw utils::ArgError(utils::format(
                "[MetaManager] : the propagate input has wrong form: '%s.%s'",
                cond.c_str(),  port.c_str()));
    }
}

VlePropagate::~VlePropagate()
{
}

std::string
VlePropagate::getName() const
{
    std::string ret = cond;
    ret.append(".");
    ret.append(port);
    return ret;
}

/***********VleInput*************/

VleInput::VleInput(const std::string& _cond, const std::string& _port,
        const value::Value& val, utils::Rand& rn):
                cond(_cond), port(_port), nbValues(0)
{
    if (cond.empty() or port.empty()) {
        throw utils::ArgError(utils::format(
                "[MetaManager] : the input has wrong form: '%s.%s'",
                cond.c_str(),  port.c_str()));
    }
    switch (val.getType()) {
    case value::Value::TUPLE:
        nbValues = val.toTuple().size();
        break;
    case value::Value::SET:
        nbValues = val.toSet().size();
        break;
    case value::Value::MAP: {
        mvalues = MetaManager::valuesFromDistrib(val.toMap(), rn);
        if (mvalues) {
            nbValues = mvalues->size();
        } else {
            nbValues = 1;
        }
        break;
    } default:
        nbValues = 1;
        break;
    }
}

VleInput::~VleInput()
{
}

std::string
VleInput::getName() const
{
    std::string ret = cond;
    ret.append(".");
    ret.append(port);
    return ret;
}

/***********VleReplicate*************/

VleReplicate::VleReplicate(const std::string& _cond, const std::string& _port,
        const value::Value& val, utils::Rand& rn):
                cond(_cond), port(_port), nbValues(0)
{
    bool err = false;
    if (cond.empty() or port.empty()) {
        throw utils::ArgError(utils::format(
                "[MetaManager] : the replicate has wrong form: '%s.%s'",
                cond.c_str(),  port.c_str()));
    }
    switch (val.getType()) {
    case value::Value::TUPLE:
        nbValues = val.toTuple().size();
        break;
    case value::Value::SET:
        nbValues = val.toSet().size();
        break;
    case value::Value::MAP: {
        mvalues = MetaManager::valuesFromDistrib(val.toMap(), rn);
        if (mvalues) {
            nbValues = mvalues->size();
        } else {
            err = true;
        }
        break;
    } default: {
        err = true;
        break;
    }}
    if (err) {
        throw utils::ArgError(utils::format(
                "[MetaManager] : error in configuration of "
                "'replicate_%s.%s', expect a value::Set or Tuple",
                cond.c_str(),  port.c_str()));
    }
}

VleReplicate::~VleReplicate()
{
}

std::string
VleReplicate::getName()
{
    std::string ret = cond;
    ret.append(".");
    ret.append(port);
    return ret;
}

/***********DelegateOutput********/
DelegateOut::DelegateOut(VleOutput& vleout, bool managedouble):
        vleOut(vleout), manageDouble(managedouble)
{
}
DelegateOut::~DelegateOut()
{
}

std::unique_ptr<value::Value>
DelegateOut::integrateReplicate(VleOutput& vleout, vle::value::Matrix& outMat)
{
    switch(vleout.integrationType) {
    case MAX: {
        double max = -9999;
        for (unsigned int i=1; i < outMat.rows(); i++) {
            double v = outMat.getDouble(vleout.colIndex, i);
            if (v > max) {
                max = v;
            }
        }
        return value::Double::create(max);
        break;
    } case LAST: {
        if (vleout.shared) {
            const std::unique_ptr<value::Value>& res =
                            outMat.get(vleout.colIndex, outMat.rows() - 1);
            return res->clone();
        } else {
            return std::move(outMat.give(vleout.colIndex, outMat.rows() - 1));
        }
        break;
    } case MSE: {
        double sum_square_error = 0;
        double nbVal = 0;
        for (unsigned int i=0; i< vleout.mse_times->size(); i++) {
            int t = std::floor(vleout.mse_times->at(i));
            if (t > 0 and t< (int) outMat.rows()) {
                sum_square_error += std::pow(
                        (outMat.getDouble(vleout.colIndex,t)
                                - vleout.mse_observations->at(i)), 2);
                nbVal++;
            }
        }
        return value::Double::create(sum_square_error/nbVal);
        break;
    } default:{
        //not possible
        break;
    }}
    return nullptr;
}

AccuMulti&
DelegateOut::getAccu(std::map<int, std::unique_ptr<AccuMulti>>& accus,
        unsigned int index, AccuStat s)
{
    std::map<int, std::unique_ptr<AccuMulti>>::iterator itf =
            accus.find(index);
    if (itf != accus.end()) {
        return *(itf->second);
    }
    std::unique_ptr<AccuMulti> ptr(new AccuMulti(s));
    AccuMulti& ref = *ptr;
    accus.insert(std::make_pair(index, std::move(ptr)));
    return ref;
}

AccuMono&
DelegateOut::getAccu(std::map<int, std::unique_ptr<AccuMono>>& accus,
        unsigned int index, AccuStat s)
{
    std::map<int, std::unique_ptr<AccuMono>>::iterator itf =
            accus.find(index);
    if (itf != accus.end()) {
        return *(itf->second);
    }
    std::unique_ptr<AccuMono> ptr(new AccuMono(s));
    AccuMono& ref = *ptr;
    accus.insert(std::make_pair(index, std::move(ptr)));
    return ref;
}


DelOutStd::DelOutStd(VleOutput& vleout): DelegateOut(vleout, true)
{
    minputAccu.reset(new AccuMono(vleOut.inputAggregationType));
}

std::unique_ptr<value::Value>
DelOutStd::insertReplicate(
            vle::value::Matrix& outMat, unsigned int currInput)
{
    //start insertion for double management only
    std::unique_ptr<value::Value> intVal = std::move(
            integrateReplicate(vleOut, outMat));
    if (vleOut.nbReplicates == 1) {
        minputAccu->insert(intVal->toDouble().value());
    } else {
        AccuMono& accuRepl = DelegateOut::getAccu(mreplicateAccu, currInput,
                vleOut.replicateAggregationType);
        accuRepl.insert(intVal->toDouble().value());
        //test if aggregating replicates is finished
        if (accuRepl.count() == vleOut.nbReplicates) {
            minputAccu->insert(accuRepl.getStat(
                    vleOut.replicateAggregationType));
            mreplicateAccu.erase(currInput);

        }
    }
    //test if aggregating inputs is finished
    if (minputAccu->count() == vleOut.nbInputs) {
        double res = minputAccu->getStat(vleOut.inputAggregationType);
        minputAccu.reset(nullptr);
        return value::Double::create(res);
    }
    return nullptr;
};


DelOutIntAggrALL::DelOutIntAggrALL(VleOutput& vleout, bool managedouble):
        DelegateOut(vleout, managedouble), mreplicateAccu(),
        minputAccu(nullptr), nbInputsFilled(0)
{

}
std::unique_ptr<value::Value>
DelOutIntAggrALL::insertReplicate(
            vle::value::Matrix& outMat, unsigned int currInput)
{
    if (not minputAccu) {
        if (manageDouble) {
            minputAccu.reset(new value::Table(vleOut.nbInputs,
                    outMat.rows()-1));
        } else {
            minputAccu.reset(new value::Matrix(vleOut.nbInputs,
                    outMat.rows()-1, 10, 10));
        }
    }
    if (vleOut.nbReplicates == 1){//one can put directly into results
        for (unsigned int i=1; i < outMat.rows(); i++) {
            if (manageDouble) {
                minputAccu->toTable().get(currInput, i-1) =
                        outMat.getDouble(vleOut.colIndex, i);
            } else if (vleOut.shared) {
                minputAccu->toMatrix().set(currInput, i-1,
                        outMat.get(vleOut.colIndex, i)->clone());
            } else {
                minputAccu->toMatrix().set(currInput, i-1,
                        std::move(outMat.give(vleOut.colIndex, i)));
            }
        }
        nbInputsFilled++;
    } else {
        AccuMulti& accuRepl = DelegateOut::getAccu(mreplicateAccu, currInput,
                vleOut.replicateAggregationType);
        accuRepl.insertColumn(outMat, vleOut.colIndex);
        if (accuRepl.count() == vleOut.nbReplicates) {
            accuRepl.fillStat(minputAccu->toTable(),
                    currInput, vleOut.replicateAggregationType);
            mreplicateAccu.erase(currInput);
            nbInputsFilled++;
        }
    }
    if (nbInputsFilled == vleOut.nbInputs) {
        return std::move(minputAccu);
    }
    return nullptr;
}

DelOutIntALL::DelOutIntALL(VleOutput& vleout): DelegateOut(vleout, true),
        mreplicateAccu(), minputAccu(nullptr)
{

}
std::unique_ptr<value::Value>
DelOutIntALL::insertReplicate(
            vle::value::Matrix& outMat, unsigned int currInput)
{
    if (not minputAccu) {
        minputAccu.reset(new AccuMulti(vleOut.inputAggregationType));
    }
    if (vleOut.nbReplicates == 1){//one can put directly into results
        minputAccu->insertColumn(outMat, vleOut.colIndex);
    } else {
        AccuMulti& accuRepl = DelegateOut::getAccu(mreplicateAccu, currInput,
                vleOut.replicateAggregationType);
        accuRepl.insertColumn(outMat, vleOut.colIndex);
        if (accuRepl.count() == vleOut.nbReplicates) {
            minputAccu->insertAccuStat(accuRepl,
                    vleOut.replicateAggregationType);
            mreplicateAccu.erase(currInput);
        }
    }
    if (minputAccu->count() == vleOut.nbInputs) {
        std::unique_ptr<value::Table> res(new value::Table(1,
                minputAccu->size()));
        minputAccu->fillStat(*res, 0, vleOut.inputAggregationType);
        return std::move(res);
    }
    return nullptr;
}

DelOutAggrALL::DelOutAggrALL(VleOutput& vleout, bool managedouble):
        DelegateOut(vleout, managedouble), mreplicateAccu(),
        minputAccu(nullptr), nbInputsFilled(0)
{
    if (manageDouble) {
        minputAccu.reset(new value::Table(vleOut.nbInputs,1));
    } else {
        minputAccu.reset(new value::Matrix(vleOut.nbInputs,1,10,10));
    }
}
std::unique_ptr<value::Value>
DelOutAggrALL::insertReplicate(
            vle::value::Matrix& outMat, unsigned int currInput)
{
    std::unique_ptr<value::Value> intVal = std::move(
            integrateReplicate(vleOut, outMat));

    if (vleOut.nbReplicates == 1){//one can put directly into results
        if (manageDouble) {
            minputAccu->toTable().get(currInput, 0) =
                    intVal->toDouble().value();
        } else {
            minputAccu->toMatrix().set(currInput, 0, std::move(intVal));

        }

        nbInputsFilled++;
    } else {
        AccuMono& accuRepl = DelegateOut::getAccu(mreplicateAccu, currInput,
                vleOut.replicateAggregationType);
        accuRepl.insert(intVal->toDouble().value());
        if (accuRepl.count() == vleOut.nbReplicates) {
            minputAccu->toTable().get(currInput, 0)=
                    accuRepl.getStat(vleOut.replicateAggregationType);
            mreplicateAccu.erase(currInput);
            nbInputsFilled++;
        }
    }
    if (nbInputsFilled == vleOut.nbInputs) {
        return std::move(minputAccu);
    }
    return nullptr;
}


/***********VleOutput*************/

VleOutput::VleOutput() :
   id(), view(), absolutePort(), colIndex(-1), shared(true),
   integrationType(LAST), replicateAggregationType(S_mean),
   inputAggregationType(S_at), nbInputs(0), nbReplicates(0), delegate(nullptr),
   mse_times(nullptr), mse_observations(nullptr)
{
}

VleOutput::VleOutput(const std::string& _id,
        const value::Value& val) :
   id(_id), view(), absolutePort(),  colIndex(-1), shared(true),
   integrationType(LAST), replicateAggregationType(S_mean),
   inputAggregationType(S_at), nbInputs(0), nbReplicates(0), delegate(nullptr),
   mse_times(nullptr), mse_observations(nullptr)
{
    std::string tmp;
    if (val.isString()) {
        if (not parsePath(val.toString().value())) {
            throw utils::ArgError(utils::format(
                    "[MetaManager] : error in configuration of the output "
                    "'output_%s' with a string; got: '%s'",
                    id.c_str(),  val.toString().value().c_str()));
        }
    } else if (val.isMap()) {
        const value::Map& m = val.toMap();
        bool error = false;
        if (m.exist("path")) {
            error = not parsePath(m.getString("path"));
        }
        if (m.exist("aggregation_replicate")) {
            tmp = m.getString("aggregation_replicate");
            if (tmp == "mean") {
                replicateAggregationType = S_mean;
            } else {
                error = true;
            }
        }
        if (m.exist("aggregation_input")) {
            tmp = m.getString("aggregation_input");
            if (tmp == "mean") {
                inputAggregationType = S_mean;
            } else if (tmp == "quantile"){
                inputAggregationType = S_quantile;
            } else if (tmp == "max"){
                inputAggregationType = S_max;
            } else if (tmp == "all"){
                inputAggregationType = S_at;
            } else {
                error = true;
            }
        }
        if (not error) {
            if (m.exist("integration")) {
                tmp = m.getString("integration");
                if (tmp == "last") {
                    integrationType = LAST;
                } else if(tmp == "max") {
                    integrationType = MAX;
                } else if(tmp == "mse") {
                    integrationType = MSE;
                } else if(tmp == "all") {
                    integrationType = ALL;
                } else {
                    error = true;
                }
            } else {
                integrationType = LAST;
            }
        }
        if (not error and integrationType == MSE) {
            if (not m.exist("mse_times") or not m.exist("mse_observations")) {
                error = true;
            } else {
                mse_times.reset(new value::Tuple(m.getTuple("mse_times")));
                mse_observations.reset(new value::Tuple(
                        m.getTuple("mse_observations")));
                error = mse_times->size() != mse_observations->size();
            }
        }
        if (error) {
            throw utils::ArgError(utils::format(
                    "[MetaManager] : error in configuration of the output "
                    " '%s%s' with a map",
                    "id_output_",  id.c_str()));
        }
    }
}

VleOutput::~VleOutput()
{
}

bool
VleOutput::parsePath(const std::string& path)
{
    std::vector<std::string> splvec;
    MetaManager::split(splvec, path, '/');
    if (splvec.size() == 2) {
        view.assign(splvec[0]);
        absolutePort.assign(splvec[1]);
        return true;
    } else {
        return false;
    }
}

std::unique_ptr<value::Value>
VleOutput::insertReplicate(value::Map& result, unsigned int currInput,
        unsigned int nbInputs, unsigned int nbReplicates)
{
    value::Map::iterator it = result.find(view);
    if (it == result.end()) {
        throw vu::ArgError(utils::format(
                "[MetaManager] view '%s' not found)",
                view.c_str()));
    }
    value::Matrix& outMat = value::toMatrixValue(*it->second);
    return insertReplicate(outMat, currInput, nbInputs, nbReplicates);
}

std::unique_ptr<value::Value>
VleOutput::insertReplicate(value::Matrix& outMat, unsigned int currInput,
        unsigned int nbIn, unsigned int nbRepl)
{
    if (not delegate){
        nbReplicates = nbRepl;
        nbInputs = nbIn;
        //performs some checks on output matrix
        if (outMat.rows() < 2){
            throw vu::ArgError("[MetaManager] expect at least 2 rows");
        }
        //get col index
        colIndex = 9999;
        for (unsigned int i=0; i < outMat.columns(); i++) {
            if (outMat.getString(i,0) == absolutePort) {
                colIndex = i;
            }
        }
        if (colIndex == 9999) {
            throw vu::ArgError(utils::format(
                    "[MetaManager] view.port '%s' not found)",
                    absolutePort.c_str()));
        }
        bool manageDouble = true;
        if (not outMat.get(colIndex,1)->isDouble()) {
            if (nbReplicates != 1 or
                (integrationType != ALL  and integrationType != LAST) or
                (inputAggregationType != S_at)){
                throw vu::ArgError(utils::format(
                        "[MetaManager] since data is not double no "
                        "aggregation is possible for output '%s'",
                        id.c_str()));
            }
            manageDouble = false;
        }
        if (integrationType == ALL) {
            if (inputAggregationType == S_at) {
                delegate.reset(new DelOutIntAggrALL(*this, manageDouble));
            } else {
                delegate.reset(new DelOutIntALL(*this));
            }
        } else {
            if (inputAggregationType == S_at) {
                delegate.reset(new DelOutAggrALL(*this, manageDouble));
            } else {
                delegate.reset(new DelOutStd(*this));
            }
        }
    }
    return delegate->insertReplicate(outMat, currInput);
}

/***********MetaManager*************/

unsigned int
MetaManager::inputsSize() const
{
    if (mInputs.size() > 0) {
        return mInputs[0]->nbValues;
    }
    return 1;
}


unsigned int
MetaManager::replicasSize() const
{
    if (not mReplicate) {
        return 1;
    }
    return mReplicate->nbValues;
}

void
MetaManager::readResultFile(const std::string& filePath, value::Matrix& mat)
{
    reader::VleResultsTextReader tfr(filePath);
    tfr.readFile(mat);
}



MetaManager::MetaManager(): mIdVpz(), mIdPackage(),
        mConfigParallelType(SINGLE), mRemoveSimulationFiles(true),
        mConfigParallelNbSlots(1), mConfigParallelMaxExpes(1),
        mexpe_debug(true), mrand(), mPropagate(), mInputs(),
        mReplicate(nullptr), mOutputs(), mWorkingDir(""),
        mCtx(utils::make_context())
{
    mCtx->set_log_priority(3);//erros only
}

MetaManager::~MetaManager()
{
    mCtx.reset();
    clear();
}

utils::Rand&
MetaManager::random_number_generator()
{
    return mrand;
}

/******************* Static public functions ****************************/

void
MetaManager::split(std::vector<std::string>& elems, const std::string &s,
        char delim)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

bool
MetaManager::parseInput(const std::string& conf,
        std::string& cond, std::string& port,
        const std::string& prefix)
{
    std::string varname;
    cond.clear();
    port.clear();
    if (prefix.size() > 0) {
        if (conf.compare(0, prefix.size(), prefix) != 0) {
            return false;
        }
        varname.assign(conf.substr(prefix.size(), conf.size()));
    } else {
        varname.assign(conf);
    }
    std::vector <std::string> splvec;
    MetaManager::split(splvec, varname, '.');
    if (splvec.size() != 2) {
        return false;
    }
    cond.assign(splvec[0]);
    port.assign(splvec[1]);
    return not cond.empty() and not port.empty();
}

bool
MetaManager::parseOutput(const std::string& conf, std::string& idout)
{
    idout.clear();
    std::string prefix = "output_";
    if (conf.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    idout.assign(conf.substr(prefix.size(), conf.size()));
    return not idout.empty();
}


std::unique_ptr<value::Tuple>
MetaManager::valuesFromDistrib(const value::Map& distrib, utils::Rand& rn)
{

    if (distrib.exist("distribution")
            and distrib.get("distribution")->isString()) {
        if (distrib.getString("distribution") == "uniform") {
            if (distrib.exist("nb") and distrib.exist("min")
                    and distrib.exist("max")
                    and distrib.get("nb")->isInteger()
                    and distrib.get("min")->isDouble()
                    and distrib.get("max")->isDouble()) {
                std::unique_ptr<value::Tuple> res(new value::Tuple(
                        distrib.getInt("nb")));
                double min = distrib.getDouble("min");
                double max = distrib.getDouble("max");
                std::vector<double>::iterator itb = res->value().begin();
                std::vector<double>::iterator ite = res->value().end();
                for (; itb != ite; itb++) {
                    *itb = rn.getDouble(min, max);
                }
                return std::move(res);
            }
        }
    }
    return nullptr;
}

void
MetaManager::clear()
{
    mPropagate.clear();
    mInputs.clear();
    mReplicate.reset(nullptr);
    mOutputs.clear();
    mOutputValues.clear();
}

}}//namespaces
