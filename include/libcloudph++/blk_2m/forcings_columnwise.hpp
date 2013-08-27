/** @file
  * @copyright University of Warsaw
  * @brief Rain sedimentation representation for single-moment bulk microphysics
  *   using forcing terms based on the upstrem advection scheme 
  * @section LICENSE
  * GPLv3+ (see the COPYING file or http://www.gnu.org/licenses/)
  */

#pragma once

#include <algorithm>

#include <libcloudph++/common/detail/zip.hpp>
#include <libcloudph++/blk_2m/terminal_vel_formulae.hpp>  //TODO so far the same parametrisation as in blk_1m 

namespace libcloudphxx
{
  namespace blk_2m
  {
    // expects the arguments to be columns with begin() pointing to the lowest level
    // returns rain flux out of the domain
    template <typename real_t, class container_t>
    quantity<divide_typeof_helper<si::mass_density, si::time>::type, real_t> forcings_columnwise(
      const opts_t<real_t> &opt,
      const container_t rhod_cont,   
      const container_t rhod_rr_cont,
      const container_t rhod_nr_cont,
      container_t drhod_rr_cont,
      container_t drhod_nr_cont,
      real_t dz
    )   
    {
      using flux_rr = quantity<divide_typeof_helper<si::mass_density, si::time>::type, real_t>;

      if (!opt.sedi) return flux_rr(0);

      using flux_nr = quantity<divide_typeof_helper<si::frequency, si::volume>::type, real_t>;
     
      flux_rr flux_rr_in = 0 * si::kilograms / si::cubic_metres / si::seconds;
      flux_nr flux_nr_in = 0 / si::cubic_metres / si::seconds;


      real_t *drhod_rr = NULL;
      real_t *drhod_nr = NULL;
      const real_t zero = 0;
      const real_t *rhod, *rhod_rr, *rhod_nr = &zero;

      auto iter = zip(rhod_cont, rhod_rr_cont, rhod_nr_cont, drhod_rr_cont, drhod_nr_cont);
      for (auto tup_ptr = --iter.end(); tup_ptr != --iter.begin(); --tup_ptr)
      {
        const real_t
          *rhod_below     = &boost::get<0>(*tup_ptr),
          *rhod_rr_below  = &boost::get<1>(*tup_ptr),
          *rhod_nr_below  = &boost::get<2>(*tup_ptr);

        if (drhod_rr != NULL) // i.e. all but first (top) grid cell
        {
          // terminal velocities at grid-cell edge (to assure precip mass conservation)
          quantity<si::velocity, real_t> tmp_vel  = -.5 * ( // averaging + axis orientation
	    formulae::v_term(
              *rhod_rr_below * si::kilograms / si::cubic_metres, 
              *rhod_nr_below / si::cubic_metres 
            ) + 
	    formulae::v_term(
              *rhod_rr * si::kilograms / si::cubic_metres,
              *rhod_nr / si::cubic_metres
            )
	  ); 
          
          flux_rr flux_rr_out = tmp_vel * (*rhod_rr * si::kilograms / si::cubic_metres) / (dz * si::metres);
          flux_nr flux_nr_out = tmp_vel * (*rhod_nr / si::cubic_metres) / (dz * si::metres);

	  *drhod_rr -= (flux_rr_in - flux_rr_out) * si::seconds * si::cubic_metres / si::kilograms;
          flux_rr_in = flux_rr_out; // inflow = outflow from above
	  *drhod_nr -= (flux_nr_in - flux_nr_out) * si::seconds * si::cubic_metres;
          flux_nr_in = flux_nr_out; // inflow = outflow from above
        }

        drhod_rr = &boost::get<3>(*tup_ptr);
        drhod_nr = &boost::get<4>(*tup_ptr);
         rhod    = rhod_below;
         rhod_rr = rhod_rr_below;
         rhod_nr = rhod_nr_below;
      }
      // the bottom grid cell (with mid-cell vterm approximation)
      quantity<si::velocity, real_t> tmp_vel = - formulae::v_term(
	*rhod_rr * si::kilograms / si::cubic_metres,
	*rhod_nr / si::cubic_metres
      ); 

      flux_rr flux_rr_out = tmp_vel * (*rhod_rr * si::kilograms / si::cubic_metres) / (dz * si::metres);
      flux_nr flux_nr_out = tmp_vel * (*rhod_nr / si::cubic_metres) / (dz * si::metres);
      *drhod_rr -= (flux_rr_in - flux_rr_out) * si::seconds * si::cubic_metres / si::kilograms;
      *drhod_nr -= (flux_nr_in - flux_nr_out) * si::seconds * si::cubic_metres;

      // outflow from the domain
      return flux_rr_out;
    }    
  }
};