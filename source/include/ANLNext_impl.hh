/*************************************************************************
 *                                                                       *
 * Copyright (c) 2011 Hirokazu Odaka                                     *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                       *
 *************************************************************************/

#ifndef ANL_ANLNext_impl_H
#define ANL_ANLNext_impl_H 1

#include "ANLNext.hh"
#include "ANLVModule.hh"
#include "ANLException.hh"

namespace anl
{

template<typename T>
ANLStatus ANLNext::routine_modfn(T func, const std::string& func_id)
{
  std::cout << "ANLNext: " << func_id << " routine started." << std::endl;

  ANLStatus status = AS_OK;
  AMIter const mod_end = m_Modules.end();
  for (AMIter mod = m_Modules.begin(); mod != mod_end; ++mod) {
    if ((*mod)->is_off()) continue;
    
    try {
      status = ((*mod)->*func)();
      if (status != AS_OK ) {
        std::cout << "ANLNext: " << func_id << " routine stopped.\n"
                  << (*mod)->module_name() << "::mod_" << func_id
                  << " return " << status << std::endl;
        break;
      }
    }
    catch (ANLException& ex) {
      ex << ANLErrModFnInfo( (*mod)->module_name() + "::mod_" + func_id );
      throw;
    }
  }
  return status;
}

}

#endif /* ANL_ANLNext_impl_H */
