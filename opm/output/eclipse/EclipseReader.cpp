/*
  Copyright 2015 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM. If not, see <http://www.gnu.org/licenses/>.
*/

#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/EclKW.hpp>
#include <ert/util/ert_unique_ptr.hpp>

#include <opm/output/eclipse/EclipseReader.hpp>
#include <opm/output/Cells.hpp>
#include <opm/output/Wells.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <algorithm>

namespace Opm {
namespace {

    std::vector<double> double_vector( const ecl_kw_type * ecl_kw ) {
        size_t size = ecl_kw_get_size( ecl_kw );

        if (ecl_kw_get_type( ecl_kw ) == ECL_DOUBLE_TYPE ) {
            const double * ecl_data = ecl_kw_get_double_ptr( ecl_kw );
            return { ecl_data , ecl_data + size };
        } else {
            const float * ecl_data = ecl_kw_get_float_ptr( ecl_kw );
            return { ecl_data , ecl_data + size };
        }

    }

    inline data::Solution restoreSOLUTION( ecl_file_type* file,
                                           int numcells,
                                           const UnitSystem& units ) {

        for( const auto* key : { "PRESSURE", "TEMP", "SWAT", "SGAS" } ) {
            if( !ecl_file_has_kw( file, key ) )
                throw std::runtime_error("Read of restart file: "
                                         "File does not contain "
                                         + std::string( key )
                                         + " data" );
        }

        const ecl_kw_type * pres = ecl_file_iget_named_kw( file, "PRESSURE", 0 );
        const ecl_kw_type * temp = ecl_file_iget_named_kw( file, "TEMP", 0 );
        const ecl_kw_type * swat = ecl_file_iget_named_kw( file, "SWAT", 0 );
        const ecl_kw_type * sgas = ecl_file_iget_named_kw( file, "SGAS", 0 );

        for( const auto& kw : { pres, temp, swat, sgas } ) {
            if( ecl_kw_get_size(kw) != numcells)
                throw std::runtime_error("Restart file: Could not restore "
                                         + std::string( ecl_kw_get_header( kw ) )
                                         + ", mismatched number of cells" );
        }

        using ds = data::Solution::key;
        data::Solution sol;
        sol.insert( ds::PRESSURE, double_vector( pres ));
        sol.insert( ds::TEMP,     double_vector( temp ));
        sol.insert( ds::SWAT,     double_vector( swat ));
        sol.insert( ds::SGAS,     double_vector( sgas ));

        const auto apply_pressure = [&]( double x ) {
            return units.to_si( UnitSystem::measure::pressure, x );
        };

        const auto apply_temperature = [=]( double x ) {
            return units.to_si( UnitSystem::measure::temperature, x );
        };

        std::transform( sol[ ds::PRESSURE ].begin(), sol[ ds::PRESSURE ].end(),
                        sol[ ds::PRESSURE ].begin(), apply_pressure );
        std::transform( sol[ ds::TEMP ].begin(), sol[ ds::TEMP ].end(),
                        sol[ ds::TEMP ].begin(), apply_temperature );

        /* optional keywords */
        if( ecl_file_has_kw( file, "RS" ) ) {
            const ecl_kw_type * rs = ecl_file_iget_named_kw( file , "RS" , 0);
            sol.insert( ds::RS, double_vector( rs ) );
        }

        if( ecl_file_has_kw( file, "RV" ) ) {
            const ecl_kw_type * rv = ecl_file_iget_named_kw( file , "RV" , 0);
            sol.insert( ds::RV, double_vector( rv ) );
        }

        return sol;
    }

    inline data::Wells restoreOPM_XWEL( ecl_file_type* file,
                                        int num_wells,
                                        int num_phases ) {
        const char* keyword  = "OPM_XWEL";
        const char* keyword2 = "OPM_IWEL";

        ecl_kw_type* xwel = ecl_file_iget_named_kw( file, keyword, 0 );
        const double* xwel_data = ecl_kw_get_double_ptr(xwel);
        const double* xwel_end  = xwel_data + ecl_kw_get_size( xwel );

        const double* bhp_begin = xwel_data;
        const double* bhp_end = bhp_begin + num_wells;
        const double* temp_begin = bhp_end;
        const double* temp_end = temp_begin + num_wells;
        const double* wellrate_begin = temp_end;
        const double* wellrate_end = wellrate_begin + (num_wells * num_phases);

        const auto remaining = std::distance( wellrate_end, xwel_end );
        const auto perf_elems = remaining / (2 + num_phases);

        const double* perfpres_begin = wellrate_end;
        const double* perfpres_end = perfpres_begin + perf_elems;
        const double* perfrate_begin = perfpres_end;
        const double* perfrate_end = perfrate_begin + perf_elems;
        const double* perfphase_begin = perfrate_end;
        const double* perfphase_end = perfphase_begin + num_phases*perf_elems;

        ecl_kw_type* iwel = ecl_file_iget_named_kw( file, keyword2, 0 );
        const int* control_begin = ecl_kw_get_int_ptr(iwel);
        std::vector<int> well_control;
        if (control_begin)
          well_control = {control_begin, control_begin + num_wells };

        return { {},
            { bhp_begin, bhp_end },
            { temp_begin, temp_end },
            { wellrate_begin, wellrate_end },
            { perfpres_begin, perfpres_end },
            { perfrate_begin, perfrate_end },
            { perfphase_begin, perfphase_end },
            well_control
        };
    }
}

std::pair< data::Solution, data::Wells >
init_from_restart_file( const EclipseState& es, int numcells ) {

    const InitConfig& initConfig         = es.getInitConfig();
    IOConfigConstPtr ioConfig            = es.getIOConfig();
    int restart_step                     = initConfig.getRestartStep();
    const std::string& restart_file_root = initConfig.getRestartRootName();
    bool output                          = false;
    const std::string filename           = ioConfig->getRestartFileName(
                                                        restart_file_root,
                                                        restart_step,
                                                        output);
    const bool unified                   = ioConfig->getUNIFIN();
    const int num_wells = es.getSchedule()->numWells( restart_step );
    const int num_phases = es.getTableManager().getNumPhases();

    using ft = ERT::ert_unique_ptr< ecl_file_type, ecl_file_close >;
    ft file( ecl_file_open( filename.c_str(), 0 ) );

    if( !file )
        throw std::runtime_error( "Restart file " + filename + " not found!" );

    if( unified &&
        !ecl_file_select_rstblock_report_step( file.get(), restart_step ) ) {
        throw std::runtime_error( "Restart file " + filename
                + " does not contain data for report step "
                + std::to_string( restart_step ) + "!" );
    }

    return {
        restoreSOLUTION( file.get(), numcells, es.getUnits() ),
        restoreOPM_XWEL( file.get(), num_wells, num_phases )
    };
}

} // namespace Opm
