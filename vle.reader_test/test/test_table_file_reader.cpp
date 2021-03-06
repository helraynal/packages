/*
 * This file is part of VLE, a framework for multi-modeling, simulation
 * and analysis of complex dynamical systems.
 * http://www.vle-project.org
 *
 * Copyright (c) 2014-2014 INRA http://www.inra.fr
 *
 * See the AUTHORS or Authors.txt file for copyright owners and
 * contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//@@tagtest@@
//@@tagdepends: vle.reader @@endtagdepends

#include <vle/utils/unit-test.hpp>

#include <vle/value/Matrix.hpp>
#include <vle/value/Map.hpp>
#include <vle/value/Set.hpp>
#include <vle/utils/Package.hpp>
#include <vle/utils/Context.hpp>

#include <iostream>
#include <vle/reader/table_file_reader.hpp>


void test_table_file_reader()
{
    auto ctx = vle::utils::make_context();

    vle::utils::Package pkg(ctx, "vle.reader_test");
    {
        vle::value::Map params;
        params.addString("sep"," ");
        vle::value::Set& columns = params.addSet("columns");
        columns.addString("double");
        columns.addString("double");
        columns.addString("double");
        vle::reader::TableFileReader tfr(pkg.getDataFile("data.txt"));
        tfr.setParams(params);
        vle::value::Matrix mat;
        tfr.readFile(mat);
        EnsuresApproximatelyEqual(mat.getDouble(2,1), 10.3, 10e-5);
        vle::reader::TableFileReader tfr2(pkg.getDataFile("data.txt"));
        tfr2.setParams(params);
        vle::value::Set set;
        tfr2.readLine(set);
        EnsuresApproximatelyEqual(set.getDouble(2), 6, 10e-5);
        tfr2.readLine(set);
        EnsuresApproximatelyEqual(set.getDouble(2), 10.3, 10e-5);
    }
    {
        vle::value::Map params;
        params.addString("sep",",");
        vle::value::Set& columns = params.addSet("columns");
        columns.addString("double");
        columns.addString("double");
        columns.addString("double");
        vle::reader::TableFileReader tfr(
                pkg.getDataFile("dataWithCommaSep.txt"));
        tfr.setParams(params);
        vle::value::Matrix mat;
        tfr.readFile(mat);
        EnsuresApproximatelyEqual(mat.getDouble(2,1), 10.3, 10e-5);
        vle::reader::TableFileReader tfr2(
                pkg.getDataFile("dataWithCommaSep.txt"));
        tfr2.setParams(params);
        vle::value::Set set;
        tfr2.readLine(set);
        EnsuresApproximatelyEqual(set.getDouble(2), 25, 10e-5);
        tfr2.readLine(set);
        EnsuresApproximatelyEqual(set.getDouble(2), 10.3, 10e-5);
    }
    {
        vle::reader::TableFileReader tfr(
                pkg.getDataFile("dataWithHeader.txt"));
        vle::value::Set header;
        tfr.readLine(header, " ");
        Ensures((header.size() == 3));
        Ensures((header.getString(2) == "V3"));
    }
    {//readLineUndo
        vle::value::Map params;
        params.addString("sep"," ");
        vle::value::Set& columns = params.addSet("columns");
        columns.addString("double");
        columns.addString("double");
        columns.addString("double");
        vle::reader::TableFileReader tfr(pkg.getDataFile("data.txt"));
        tfr.setParams(params);
        vle::value::Set set;
        tfr.readLine(set);
        std::cout << " 1: " << set << std::endl;
        EnsuresApproximatelyEqual(set.getDouble(2), 6, 10e-5);
        tfr.readLineUndo();
        tfr.readLine(set);
        std::cout << " 2: " << set << std::endl;
        EnsuresApproximatelyEqual(set.getDouble(2), 6, 10e-5);
        tfr.readLine(set);
        std::cout << " 3: " << set << std::endl;
        EnsuresApproximatelyEqual(set.getDouble(2), 10.3, 10e-5);
        tfr.readLineUndo();
        tfr.readLine(set);
        std::cout << " 4: " << set << std::endl;
        EnsuresApproximatelyEqual(set.getDouble(2), 10.3, 10e-5);
    }
}

int main()
{
    test_table_file_reader();

    return unit_test::report_errors();
}
