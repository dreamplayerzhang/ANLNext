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

#ifndef ANL_ModuleParameter_H
#define ANL_ModuleParameter_H 1

#include "VModuleParameter.hh"

#include <ostream>
#include <istream>
#include <sstream>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <iomanip>
#include <type_traits>
#include <boost/call_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/icl/type_traits/is_container.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#ifdef ANL_USE_TVECTOR
#include "TVector2.h"
#include "TVector3.h"
#endif /* ANL_USE_TVECTOR */

#ifdef ANL_USE_HEPVECTOR
#include "CLHEP/Vector/TwoVector.h"
#include "CLHEP/Vector/ThreeVector.h"
#endif /* ANL_USE_HEPVECTOR */

#include "ANLException.hh"
#include "ANLType.hh"

namespace anl
{

/**
 * A class template for an ANL module parameter.
 * @author Hirokazu Odaka
 * @date 2011-07-12
 * @date 2012-12-12
 * @date 2014-12-09 | use variadic template.
 * @date 2015-11-10 | get_value() methods
 * @date 2017-07-03 | rename get/set to __get__/__set__
 */
template <typename T>
class ModuleParameter : public VModuleParameter
{
  typedef std::integral_constant<bool,
                                 boost::icl::is_container<T>::value
                                 && !std::is_same<T, std::string>::value> is_container_type;
  
  typedef std::integral_constant<bool,
                                 std::is_arithmetic<T>::value> is_arithmetic_type;
    
  typedef std::integral_constant<bool,
                                 std::is_floating_point<T>::value> is_floating_point_type;

  typedef std::integral_constant<bool,
                                 std::is_integral<T>::value> is_integer_type;

  typedef typename param_call_traits<T>::type call_type;
  
public:
  ModuleParameter(T* ptr, const std::string& name);
  
  ModuleParameter(T* ptr, const std::string& name,
                  double unit, const std::string& unit_name);
  
  ModuleParameter(T* ptr, const std::string& name,
                  const std::string& expression);
  
  ModuleParameter(T* ptr, const std::string& name,
                  const std::string& expression,
                  const std::string& default_string);

  std::string type_name() const override
  { return type_info<T>::name(); }

  bool ask() override;

  void set_value(call_type val) override
  {
    set_value_impl(val,
                   is_container_type(),
                   is_arithmetic_type(),
                   is_floating_point_type());
  }
  using VModuleParameter::set_value;

  void clear_array() override
  {
    clear_array_impl(is_container_type());
  }
  
  T get_value(call_type dummy) const override
  {
    return get_value_impl(dummy,
                          is_container_type(),
                          is_arithmetic_type(),
                          is_floating_point_type());
  }
  using VModuleParameter::get_value;
  
  void output(std::ostream& os) const override
  {
    output_impl(os,
                is_container_type(),
                is_arithmetic_type(),
                is_floating_point_type());
  }
  
  void input(std::istream& is) override
  {
    input_impl(is,
               is_container_type(),
               is_arithmetic_type(),
               is_floating_point_type());
  }

  void __get__(void* value_ptr) const override
  {
    *static_cast<T*>(value_ptr) = *ptr_;
  }
  
  void __set__(const void* value_ptr) override
  {
    *ptr_ = *static_cast<const T*>(value_ptr);
  }

  boost::property_tree::ptree to_property_tree() const override
  {
    boost::property_tree::ptree pt;
    pt.put("name", name());
    pt.put("type", type_name());
    put_unit_info_to_property_tree(pt);
    put_value_info_to_property_tree(pt);
    return pt;
  }
  
protected:
  template <bool b0>
  bool ask_sequential(const std::integral_constant<bool, b0>&)
  { return ask_base(); }

  bool ask_sequential(const std::true_type&);

  void put_unit_info_to_property_tree(boost::property_tree::ptree& pt) const
  {
    put_unit_info_to_property_tree_impl(pt, is_floating_point_type());
  }
  
  void put_value_info_to_property_tree(boost::property_tree::ptree& pt) const
  {
    put_value_info_to_property_tree_impl(pt, is_container_type());
  }
  
private:
  template <bool b0, bool b1, bool b2>
  void set_value_impl(call_type val,
                      const std::integral_constant<bool, b0>&,
                      const std::integral_constant<bool, b1>&,
                      const std::integral_constant<bool, b2>&)
  {
    // non-container, non-number
    *ptr_ = val;
  }
  
  template <bool b0, bool b2>
  void set_value_impl(call_type val,
                      const std::integral_constant<bool, b0>&,
                      const std::true_type&,
                      const std::integral_constant<bool, b2>&)
  {
    // non-container, number, int
    *ptr_ = val;
  }
  
  template <bool b0>
  void set_value_impl(call_type val,
                      const std::integral_constant<bool, b0>&,
                      const std::true_type&,
                      const std::true_type&)
  {
    // non-container, number, floating-point
    *ptr_ = val * unit();
  }
  
  template <bool b1, bool b2>
  void set_value_impl(call_type val,
                      const std::true_type&,
                      const std::integral_constant<bool, b1>&,
                      const std::integral_constant<bool, b2>&)
  {
    // container
    typedef typename T::iterator iter_type;
    typedef typename std::iterator_traits<iter_type>::iterator_category IterCategory;
    set_value_impl(val, IterCategory());
  }
  
  void set_value_impl(call_type val, std::random_access_iterator_tag);
  void set_value_impl(call_type val, std::forward_iterator_tag);
  
  template <bool b0>
  void clear_array_impl(const std::integral_constant<bool, b0>&)
  {
    // non-container
  }
  
  void clear_array_impl(const std::true_type&)
  {
    // container
    ptr_->clear();
  }
  
  template <bool b0, bool b1, bool b2>
  T get_value_impl(call_type,
                   const std::integral_constant<bool, b0>&,
                   const std::integral_constant<bool, b1>&,
                   const std::integral_constant<bool, b2>&) const
  {
    // non-container, non-number
    return *ptr_;
  }
  
  template <bool b0, bool b2>
  T get_value_impl(call_type,
                   const std::integral_constant<bool, b0>&,
                   const std::true_type&,
                   const std::integral_constant<bool, b2>&) const
  {
    // non-container, number, int
    return *ptr_;
  }
  
  template <bool b0>
  T get_value_impl(call_type,
                   const std::integral_constant<bool, b0>&,
                   const std::true_type&,
                   const std::true_type&) const
  {
    // non-container, number, floating-point
    return (*ptr_)/unit();
  }
  
  template <bool b1, bool b2>
  T get_value_impl(call_type dummy,
                   const std::true_type&,
                   const std::integral_constant<bool, b1>&,
                   const std::integral_constant<bool, b2>&) const
  {
    // container
    typedef typename T::iterator iter_type;
    typedef typename std::iterator_traits<iter_type>::iterator_category IterCategory;
    return get_value_impl(dummy, IterCategory());
  }
  
  T get_value_impl(call_type dummy, std::random_access_iterator_tag) const;
  T get_value_impl(call_type dummy, std::forward_iterator_tag) const;

  template <bool b0, bool b1, bool b2>
  void output_impl(std::ostream& os,
                   const std::integral_constant<bool, b0>&,
                   const std::integral_constant<bool, b1>&,
                   const std::integral_constant<bool, b2>&) const
  {
    // non-container, non-number
    os << *ptr_;
  }
  
  template <bool b0, bool b2>
  void output_impl(std::ostream& os,
                   const std::integral_constant<bool, b0>&,
                   const std::true_type&,
                   const std::integral_constant<bool, b2>&) const
  {
    // non-container, number, int
    using std::string;
    if (expression().find("hex")!=string::npos) os << std::hex;
    else if (expression().find("dec")!=string::npos) os << std::dec;
    os << *ptr_;
    os << std::dec;
  }

  template <bool b0>
  void output_impl(std::ostream& os,
                   const std::integral_constant<bool, b0>&,
                   const std::true_type&,
                   const std::true_type&) const
  {
    // non-container, number, floating-point
    os << *ptr_/unit();
  }
  
  template <bool b1, bool b2>
  void output_impl(std::ostream& os,
                   const std::true_type&,
                   const std::integral_constant<bool, b1>&,
                   const std::integral_constant<bool, b2>&) const
  {
    // container
    typedef typename T::iterator iter_type;
    typedef typename std::iterator_traits<iter_type>::iterator_category IterCategory;
    output_impl(os, IterCategory());
  }
  
  void output_impl(std::ostream& os, std::forward_iterator_tag) const;

  template <bool b0, bool b1, bool b2>
  void input_impl(std::istream& is,
                   const std::integral_constant<bool, b0>&,
                   const std::integral_constant<bool, b1>&,
                   const std::integral_constant<bool, b2>&)
  {
    // non-container, non-number
    is >> *ptr_;
  }
  
  template <bool b0, bool b2>
  void input_impl(std::istream& is,
                  const std::integral_constant<bool, b0>&,
                  const std::true_type&,
                  const std::integral_constant<bool, b2>&)
  {

    // non-container, number, int
    using std::string;
    if (expression().find("hex")!=string::npos) is >> std::hex;
    else if (expression().find("dec")!=string::npos) is >> std::dec;
    is >> *ptr_;
    is >> std::dec;
  }

  template <bool b0>
  void input_impl(std::istream& is,
                  const std::integral_constant<bool, b0>&,
                  const std::true_type&,
                  const std::true_type&)
  {
    // non-container, number, floating-point
    T val(0.0);
    is >> val;
    if (is) { *ptr_ = val * unit(); }
  }
  
  template <bool b1, bool b2>
  void input_impl(std::istream& is,
                  const std::true_type&,
                  const std::integral_constant<bool, b1>&,
                  const std::integral_constant<bool, b2>&)
  {
    // container
    typedef typename T::iterator iter_type;
    typedef typename std::iterator_traits<iter_type>::iterator_category IterCategory;
    input_impl(is, IterCategory());
  }
  
  void input_impl(std::istream& is, std::random_access_iterator_tag);
  void input_impl(std::istream& is, std::forward_iterator_tag);

  std::string special_message_to_ask();

  template <bool b> std::string
  special_message_to_ask_impl(const std::integral_constant<bool, b>&)
  { return VModuleParameter::special_message_to_ask(); }
  
  std::string  special_message_to_ask_impl(const std::true_type&)
  {
    std::string message("\n  vector length, values... \n");
    return message;
  }

  template <bool b0>
  void put_unit_info_to_property_tree_impl(boost::property_tree::ptree&,
                                           const std::integral_constant<bool, b0>&) const
  {
    // non-floating-point
  }

  void put_unit_info_to_property_tree_impl(boost::property_tree::ptree& pt,
                                           const std::true_type&) const
  {
    // floating-point
    pt.put("unit_name", unit_name());
    pt.put("unit", unit());
  }

  template <bool b0>
  void put_value_info_to_property_tree_impl(boost::property_tree::ptree& pt,
                                            const std::integral_constant<bool, b0>&) const
  {
    // non-container
    T dummy = T();
    pt.put("value", get_value(dummy));
  }

  void put_value_info_to_property_tree_impl(boost::property_tree::ptree& pt,
                                            const std::true_type&) const
  {
    // container
    boost::property_tree::ptree pt_values;
    T dummy = T();
    T values = get_value(dummy);
    for (const auto& v: values) {
      boost::property_tree::ptree pt_value;
      pt_value.put("", v);
      pt_values.push_back(std::make_pair("", pt_value));
    }
    pt.add_child("value", pt_values);
  }
  
private:
  T* ptr_;
};

} /* namespace anl */

#include "ModuleParameter_impl.hh"
#include "ModuleParameter_spec.hh"
#include "ModuleParameter_map.hh"
#include "ModuleParameter_vector.hh"

#endif /* ANL_ModuleParameter_H */
