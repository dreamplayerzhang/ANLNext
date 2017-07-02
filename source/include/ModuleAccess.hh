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

#ifndef ANL_ModuleAccess_H
#define ANL_ModuleAccess_H 1

#include <string>
#include <map>
#include <iterator>
#include <boost/format.hpp>
#include "ANLException.hh"

namespace anl
{

class BasicModule;

/**
 * Interface to access other ANL modules.
 * 
 * @author Hirokazu Odaka
 * @date 2010-06-xx
 * @date 2016-08-19
 * @date 2017-07-29
 */
class ModuleAccess
{
public:
  enum class ConflictOption { error, yield, overwrite, remove };

public:
  ModuleAccess() = default;
  ~ModuleAccess();
  ModuleAccess(const ModuleAccess&) = default;
  ModuleAccess(ModuleAccess&&) = default;
  ModuleAccess& operator=(const ModuleAccess&) = default;
  ModuleAccess& operator=(ModuleAccess&&) = default;

  const BasicModule* getModule(const std::string& name) const
  { return getModuleNC(name); }
  
  BasicModule* getModuleNC(const std::string& name) const;

  void registerModule(const std::string& name, BasicModule* module,
                      ConflictOption conflict=ConflictOption::yield);

  bool exist(const std::string& name) const;

private:
  using ANLModuleMap = std::map<std::string, BasicModule*>;
  ANLModuleMap moduleMap_;
};

inline
BasicModule* ModuleAccess::getModuleNC(const std::string& name) const
{
  ANLModuleMap::const_iterator it = moduleMap_.find(name);
  if (it == std::end(moduleMap_)) {
    const std::string message
      = (boost::format("Module is not found: %s") % name).str();
    BOOST_THROW_EXCEPTION( ANLException(message) );
  }
  return it->second;
}

inline
bool ModuleAccess::exist(const std::string& name) const
{
  return (moduleMap_.find(name) != std::end(moduleMap_));
}

} /* namespace anl */

#endif /* ANL_ModuleAccess_H */
