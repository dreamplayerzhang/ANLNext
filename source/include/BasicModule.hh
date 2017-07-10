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

#ifndef ANL_BasicModule_H
#define ANL_BasicModule_H 1

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <algorithm>
#include <iterator>
#include <memory>

#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include "ANLStatus.hh"
#include "ModuleParameter.hh"
#include "ANLException.hh"
#include "ModuleAccess.hh"
#include "ANLMacro.hh"

#ifdef ANL_USE_TVECTOR
#include "TVector2.h"
#include "TVector3.h"
#endif /* ANL_USE_TVECTOR */

#ifdef ANL_USE_HEPVECTOR
#include "CLHEP/Vector/TwoVector.h"
#include "CLHEP/Vector/ThreeVector.h"
#endif /* ANL_USE_HEPVECTOR */

namespace anl
{

class EvsManager;

/**
 * A basic class for an ANL Next module.
 *
 * @author Hirokazu Odaka
 * @date 2010-06-xx
 * @date 2010-09-18
 * @date 2013-05-22
 * @date 2014-12-18
 * @date 2015-11-10 | review parameter setter/getter methods
 * @date 2017-07-02 | do not own ModuleAccess, always fully accessible
 * @date 2017-07-07 | new model (mod-methods are renamed)
 */
class BasicModule
{
private:
  virtual std::string __module_name__() const
  { return "BasicModule"; }
  virtual std::string __module_version__() const
  { return "0.0"; }
  virtual std::unique_ptr<BasicModule> __clone__()
  { return nullptr; }

public:
  BasicModule();
  virtual ~BasicModule();

  BasicModule(BasicModule&& r) = delete;
  BasicModule& operator=(const BasicModule& r) = delete;
  BasicModule& operator=(BasicModule&& r) = delete;

  virtual std::unique_ptr<BasicModule> clone()
  { return __clone__(); }

protected:
  BasicModule(const BasicModule& r);

public:
  std::string module_name() const { return __module_name__(); }
  std::string module_version() const { return __module_version__(); }
  
  void set_module_id(const std::string& v);
  std::string module_id() const { return (this->*moduleIDMethod_)(); }

  int copy_id() const { return copyID_; }
  bool is_master() const { return (copyID_ == 0); }
  
  virtual ANLStatus mod_define()         { return AS_OK; }
  virtual ANLStatus mod_pre_initialize() { return AS_OK; }
  virtual ANLStatus mod_initialize()     { return AS_OK; }
  virtual ANLStatus mod_begin_run()      { return AS_OK; }
  virtual ANLStatus mod_analyze()        { return AS_OK; }
  virtual ANLStatus mod_end_run()        { return AS_OK; }
  virtual ANLStatus mod_finalize()       { return AS_OK; }

  virtual ANLStatus mod_reduce(const std::list<BasicModule*>& parallel_modules);
  virtual ANLStatus mod_merge(const BasicModule*) { return AS_OK; }

  virtual ANLStatus mod_communicate() { ask_parameters(); return AS_OK; }

  std::vector<std::pair<std::string, ModuleAccess::ConflictOption>> get_aliases() const { return aliases_; }
  std::vector<std::string> get_aliases_string() const;
  void add_alias(const std::string& name,
                 ModuleAccess::ConflictOption conflict=ModuleAccess::ConflictOption::error)
  {
    aliases_.emplace_back(name, conflict);
  }

  std::string module_description() const { return moduleDescription_; }
  void set_module_description(const std::string& v) { moduleDescription_ = v; }

  void set_evs_manager(EvsManager* man) { evsManager_ = man; }
  void set_module_access(const ModuleAccess* aa) { moduleAccess_ = aa; }

  /**
   * enable this module.
   */
  void on() { moduleOn_ = true; }
  
  /**
   * disable this module.
   */
  void off() { moduleOn_ = false; }

  /**
   * @return true if this module is on.
   */
  bool is_on() const { return moduleOn_; }

  /**
   * @return true if this module is off.
   */
  bool is_off() const { return !moduleOn_; }

  void set_loop_index(long int index) { loopIndex_ = index; }
  long int get_loop_index() const { return loopIndex_; }
  
  /**
   * expose a module parameter specified by "name" and set it as the current parameter.
   * @param name module parameter name
   */
  void expose_parameter(const std::string& name)
  { hide_parameter(name, false); }
  
  ModuleParamConstIter parameter_begin() const
  { return std::begin(moduleParameters_); }
  ModuleParamConstIter parameter_end() const
  { return std::end(moduleParameters_); }
  ModuleParamConstIter find_parameter(const std::string& name) const;
  const VModuleParameter* get_parameter(const std::string& name) const
  {
    return find_parameter(name)->get();
  }
  
  template<typename T>
  void set_parameter(const std::string& name, T val);
  void set_parameter(const std::string& name,
                     double x, double y);
  void set_parameter(const std::string& name,
                     double x, double y, double z);
  void clear_array(const std::string& name);
  void set_map_key(const std::string& key)
  {
    currentParameter_->set_map_key(key);
  }
  
  template <typename T>
  void set_value_element(const std::string& name, T val);

  void insert_to_container() { currentParameter_->insert_to_container(); }

  void print_parameters();
  void ask_parameters();

  boost::property_tree::ptree parameters_to_property_tree() const;

protected:
  template <typename T>
  void register_parameter(T* ptr, const std::string& name);

  template <typename T>
  void register_parameter(T* ptr, const std::string& name,
                          double unit, const std::string& unit_name);

  template <typename T>
  void register_parameter(T* ptr, const std::string& name,
                          const std::string& expression);

  template <typename T>
  void register_parameter(T* ptr, const std::string& name,
                          const std::string& expression,
                          const std::string& default_string);

  template <typename T>
  void register_parameter_map(T* ptr, const std::string& name,
                              const std::string& key_name,
                              const std::string& key_default);

  void set_parameter_question(const std::string& q)
  { currentParameter_->set_question(q); }
  void set_parameter_description(const std::string& q)
  { currentParameter_->set_description(q); }
  
  void unregister_parameter(const std::string& name);
  void hide_parameter(const std::string& name, bool hidden=true);
  void ask_parameter(const std::string& name, const std::string& question="");

  template <typename T>
  void add_value_element(T* ptr, const std::string& name);
  template <typename T>
  void add_value_element(T* ptr, const std::string& name,
                         double unit, const std::string& unit_name,
                         const std::string& question="");
  template <typename T>
  void add_value_element(T* ptr, const std::string& name,
                         const std::string& expression,
                         const std::string& question="");
  
  void enable_value_elements(int type, const std::vector<std::size_t>& enable)
  { currentParameter_->enable_value_elements(type, enable); }
  
  template <typename T>
  void get_module(const std::string& name, T* ptr)
  { *ptr = static_cast<T>(moduleAccess_->get_module(name)); }

  template <typename T>
  void get_module_NC(const std::string& name, T* ptr)
  { *ptr = static_cast<T>(moduleAccess_->get_module_NC(name)); }

  template <typename T>
  const T* get_module(const std::string& name)
  { return static_cast<const T*>(moduleAccess_->get_module(name)); }

  template <typename T>
  T* get_module_NC(const std::string& name)
  { return static_cast<T*>(moduleAccess_->get_module_NC(name)); }

  template <typename T>
  void get_module_IF(const std::string& name, T* ptr);

  template <typename T>
  void get_module_IFNC(const std::string& name, T* ptr);

  bool exist_module(const std::string& name)
  { return moduleAccess_->exist(name); }
  
  void define_evs(const std::string& key);
  void undefine_evs(const std::string& key);
  bool is_evs_defined(const std::string& key) const;
  bool evs(const std::string& key) const;
  void set_evs(const std::string& key);
  void reset_evs(const std::string& key);

protected:
  template <typename ModuleType>
  std::unique_ptr<BasicModule> make_clone(ModuleType*&& copied);

private:
  ModuleParamIter find_parameter(const std::string& name);
  std::string get_module_id() const { return moduleID_; }

private:
  std::string moduleID_;
  std::vector<std::pair<std::string, ModuleAccess::ConflictOption>> aliases_;
  std::string moduleDescription_;
  bool moduleOn_ = true;
  EvsManager* evsManager_ = nullptr;
  const ModuleAccess* moduleAccess_ = nullptr;
  ModuleParamList moduleParameters_;
  ModuleParam_sptr currentParameter_;
  long int loopIndex_ = -1;
    
  const int copyID_ = 0;
  int lastCopy_ = 0;

  std::string (BasicModule::*moduleIDMethod_)() const;
};

using AMIter = std::vector<BasicModule*>::iterator;
using AMConstIter = std::vector<BasicModule*>::const_iterator;

template<typename T>
void BasicModule::register_parameter(T* ptr, const std::string& name)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name));
  moduleParameters_.push_back(p);
  currentParameter_ = p;
}

template<typename T>
void BasicModule::register_parameter(T* ptr, const std::string& name,
                                     double unit, const std::string& unit_name)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name, unit, unit_name));
  moduleParameters_.push_back(p);
  currentParameter_ = p;
}

template<typename T>
void BasicModule::register_parameter(T* ptr, const std::string& name,
                                     const std::string& expression)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name, expression));
  moduleParameters_.push_back(p);
  currentParameter_ = p;
}

template<typename T>
void BasicModule::register_parameter(T* ptr, const std::string& name,
                                     const std::string& expression,
                                     const std::string& default_string)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name, expression, default_string));
  moduleParameters_.push_back(p);
  currentParameter_ = p;
}

template <typename T>
void BasicModule::register_parameter_map(T* ptr, const std::string& name,
                                         const std::string& key_name,
                                         const std::string& key_default)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name, key_name, key_default));
  moduleParameters_.push_back(p);
  currentParameter_ = p;
}

template <typename T>
void BasicModule::add_value_element(T* ptr, const std::string& name)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name));
  currentParameter_->add_value_element(p);
}

template <typename T>
void BasicModule::add_value_element(T* ptr, const std::string& name,
                                    double unit, const std::string& unit_name,
                                    const std::string& question)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name, unit, unit_name));
  p->set_question(question);
  currentParameter_->add_value_element(p);
}

template <typename T>
void BasicModule::add_value_element(T* ptr, const std::string& name,
                                    const std::string& expression,
                                    const std::string& question)
{
  ModuleParam_sptr p(new ModuleParameter<T>(ptr, name, expression));
  p->set_question(question);
  currentParameter_->add_value_element(p);
}

inline
ModuleParamIter BasicModule::find_parameter(const std::string& name)
{
  ModuleParamIter it = std::begin(moduleParameters_);
  for (; it!=std::end(moduleParameters_); ++it) {
    if ((*it)->name() == name) {
      return it;
    }
  }
  if (it == std::end(moduleParameters_)) {
    const std::string message
      = (boost::format("Parameter is not found: %s / %s") % module_id() % name).str();
    BOOST_THROW_EXCEPTION( ANLException(this, message) );
  }
  return it;
}

inline
ModuleParamConstIter BasicModule::find_parameter(const std::string& name) const
{
  ModuleParamConstIter it = std::begin(moduleParameters_);
  for (; it!=std::end(moduleParameters_); ++it) {
    if ((*it)->name() == name) {
      return it;
    }
  }
  if (it == std::end(moduleParameters_)) {
    const std::string message
      = (boost::format("Parameter is not found: %s / %s") % module_id() % name).str();
    BOOST_THROW_EXCEPTION( ANLException(this, message) );
  }
  return it;
}

template <typename T>
void BasicModule::set_parameter(const std::string& name, T val)
{
  ModuleParamIter it = find_parameter(name);
  (*it)->set_value(val);
}

inline
void BasicModule::clear_array(const std::string& name)
{
  ModuleParamIter it = find_parameter(name);
  (*it)->clear_array();
}

inline
void BasicModule::set_parameter(const std::string& name,
                                double x, double y)
{
  ModuleParamIter it = find_parameter(name);
  (*it)->set_value(x, y);
}

inline
void BasicModule::set_parameter(const std::string& name,
                                double x, double y, double z)
{
  ModuleParamIter it = find_parameter(name);
  (*it)->set_value(x, y, z);
}

template <typename T>
void BasicModule::set_value_element(const std::string& name, T val)
{
  try {
    currentParameter_->set_value_element(name, val);
  }
  catch (ANLException& e) {
    e.set_module_info(this);
    throw;
  }
}

template <typename T>
inline
void BasicModule::get_module_IF(const std::string& name, T *ptr)
{
  *ptr = dynamic_cast<T>(moduleAccess_->get_module(name));
  if (ptr==0) {
    const std::string message
      = (boost::format("Dynamic cast failed from ANL Module: %s") % name).str();
    BOOST_THROW_EXCEPTION( ANLException(this, message) );
  }
}

template <typename T>
inline
void BasicModule::get_module_IFNC(const std::string& name, T *ptr)
{
  *ptr = dynamic_cast<T>(moduleAccess_->get_module_NC(name));
  if (ptr==0) {
    const std::string message
      = (boost::format("Dynamic cast failed from ANL Module: %s") % name).str();
    BOOST_THROW_EXCEPTION( ANLException(this, message) );
  }
}

template <typename ModuleType>
std::unique_ptr<BasicModule> BasicModule::make_clone(ModuleType*&& copied)
{
  std::unique_ptr<BasicModule> m(copied);
  if (this->is_master()) {
    this->lastCopy_ += 1;
  }
  else {
    m.reset(nullptr);
  }
  return m;
}

} /* namespace anl */

#endif /* ANL_BasicModule_H */
