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

#include <cstddef>
#include <tuple>
#include <array>
#include <memory>
#include <type_traits>
#include <boost/format.hpp>

namespace anl
{

struct value_category_scalar {};
struct value_category_pair {};
struct value_category_tuple {};

template <typename T>
struct size_of_value
{
  typedef value_category_scalar type;
  constexpr static std::size_t value = 1;
};

template <typename T1, typename T2>
struct size_of_value<std::pair<T1, T2>>
{
  typedef value_category_pair type;
  constexpr static std::size_t value = 2;
};

template <typename... Ts>
struct size_of_value<std::tuple<Ts...>>
{
  typedef value_category_tuple type;
  typedef std::tuple<Ts...> tuple_type;
  constexpr static std::size_t value = std::tuple_size<tuple_type>::value;
};


/**
 * A class template for an ANL module parameter. Partial specialization.
 * @author Hirokazu Odaka
 * @date 2011-07-12
 * @date 2012-12-12
 * @date 2014-12-09 | use variadic template.
 * @date 2015-11-10 | rename {set/get/output}_value<I>() to value_info_{set/get/output}().
 * @date 2016-08-19 | modify exceptions.
 */
template <typename T>
class ModuleParameter<std::map<std::string, T>> : public VModuleParameter
{
  typedef std::map<std::string, T> container_type;
  typedef std::string key_type;
  typedef T value_type;
  typedef typename container_type::iterator iter_type;
  typedef typename size_of_value<value_type>::type value_category;
 
  constexpr static std::size_t ValueSize = size_of_value<value_type>::value;
  typedef std::integral_constant<std::size_t, ValueSize> ValueEnd_t;

public:
  ModuleParameter(container_type* ptr, const std::string& name,
                  const std::string& key_name,
                  const std::string& key_default)
    : VModuleParameter(name), ptr_(ptr),
      key_name_(key_name), buffer_key_(key_default), first_input_(true)
  {
    set_default_string(key_default);
  }
  
  ModuleParameter(container_type* ptr, const std::string& name,
                  const std::string& key_name,
                  const std::string& key_default,
                  const std::vector<ModuleParam_sptr>& values_info)
    : VModuleParameter(name), ptr_(ptr),
      key_name_(key_name), buffer_key_(key_default), value_info_(values_info),
      first_input_(true)
  {
    set_default_string(key_default);
  }

  std::string type_name() const override
  { return "map"; }

  void output(std::ostream& os) const override
  {
    os << '\n';
    for (iter_type it=ptr_->begin(); it!=ptr_->end(); ++it) {
      os << "  " << key_name_ << ": " << it->first << '\n';
      value_type tmpValue = it->second;
      value_info_set<0>(&tmpValue, value_category());
      value_info_output<0>(os, value_category());
    }
    os.flush();
  }
  
  void input(std::istream&) override {}
  
  std::string map_key_name() const override
  { return key_name_; }
  void set_map_key(const std::string& key) override
  { buffer_key_ = key; }

  std::size_t num_value_elements() const override
  { return value_info_.size(); }
  std::shared_ptr<VModuleParameter const> value_element_info(std::size_t index) const override
  { return value_info_[index]; }
  std::string value_element_name(std::size_t index) const override
  { return value_info_[index]->name(); }

  void add_value_element(ModuleParam_sptr param) override
  { value_info_.push_back(param); }

  void enable_value_elements(int type, const std::vector<std::size_t>& enables) override
  {
    std::array<std::size_t, ValueSize> enableArray;
    for (std::size_t i=0; i<ValueSize; ++i) {
      enableArray[i] = 0;
    }
    for (std::size_t k=0; k<enables.size(); ++k) {
      const std::size_t ena = enables[k];
      if (ena < ValueSize) {
        enableArray[ena] = 1;
      }
    }
    value_enable_[type] = enableArray;
  }
  
  bool value_enable(std::size_t, value_category_scalar) const { return true; }
  bool value_enable(std::size_t, value_category_pair) const { return true; }
  bool value_enable(std::size_t i, value_category_tuple) const
  {
    if (i>0) {
      int type = 0;
      value_info_[0]->__get__(&type);
      typename decltype(value_enable_)::const_iterator it
        = value_enable_.find(type);
      if (it!=value_enable_.end() && (it->second)[i]==0) {
        return false;
      }
    }
    return true;
  }

  std::size_t size_of_container() const override
  { return ptr_->size(); }
  void clear_container() override
  { ptr_->clear(); }

  std::vector<std::string> map_key_list() const override
  {
    std::vector<std::string> keys;
    for (auto& pair: *ptr_) {
      keys.push_back(pair.first);
    }
    return keys;
  }
  
  void insert_to_container() override
  {
    value_type tmpValue;
    value_info_get<0>(&tmpValue, value_category());
    ptr_->insert(std::make_pair(buffer_key_, tmpValue));
    
    value_info_set<0>(&default_value_, value_category());
  }

  void retrieve_from_container(const std::string& key) const override
  {
    auto it = ptr_->find(key);
    if (it == ptr_->end()) {
      const std::string message
        = (boost::format("Map \"%s\" does not have key: %s") % name() % key).str();
      BOOST_THROW_EXCEPTION( ANLException(message) );
    }
    value_type value = (*it).second;
    value_info_set<0>(&value, value_category());
  }
  using VModuleParameter::retrieve_from_container;

  void set_value_element(const std::string& name, bool val) override
  { set_value_element_impl(name, val); }

  void set_value_element(const std::string& name, int val) override
  { set_value_element_impl(name, val); }

  void set_value_element(const std::string& name, double val) override
  { set_value_element_impl(name, val); }

  void set_value_element(const std::string& name, const std::string& val) override
  { set_value_element_impl(name, val); }

  bool get_value_element(const std::string& name, bool dummy) const override
  { return get_value_element_impl(name, dummy); }
  
  int get_value_element(const std::string& name, int dummy) const override
  { return get_value_element_impl(name, dummy); }

  double get_value_element(const std::string& name, double dummy) const override
  { return get_value_element_impl(name, dummy); }

  std::string get_value_element(const std::string& name, const std::string& dummy) const override
  { return get_value_element_impl(name, dummy); }
  
  bool ask() override
  {
    std::cout << "Define table of " << name() << ":" << std::endl;
    ModuleParameter<std::string> tmpKeyParam(&buffer_key_, key_name_);
    tmpKeyParam.set_question(name()+" (OK for exit)");
    
    ptr_->clear();
    if (first_input()) { initialize_default_value_elements(); }

    while (1) {
      tmpKeyParam.ask();
      if (buffer_key_=="ok" || buffer_key_=="OK") break;
      
      value_info_set<0>(&default_value_, value_category());
      
      const std::size_t NumValue = value_info_.size();
      for (std::size_t i=0; i<NumValue; ++i) {
        if (value_enable(i, value_category())) {
          value_info_[i]->ask();
        }
      }
      
      value_type tmpValue;
      value_info_get<0>(&tmpValue, value_category());
      ptr_->insert(std::make_pair(buffer_key_, tmpValue));
      buffer_key_ = "OK";
    }
    return true;
  }

  boost::property_tree::ptree to_property_tree() const override
  {
    using boost::property_tree::ptree;
    ptree pt;
    pt.put("name", name());
    pt.put("type", type_name());

    ptree pt_values;
    std::vector<std::string> key_list = map_key_list();
    for (auto& key: key_list) {
      ptree pt_value;
      retrieve_from_container(key);
      const std::size_t num_elements = num_value_elements();
      for (std::size_t j=0; j<num_elements; j++) {
        std::shared_ptr<VModuleParameter const> element = value_element_info(j);
        ptree pt_element = element->to_property_tree();
        pt_value.push_back(std::make_pair("", std::move(pt_element))); 
      }
      pt_values.push_back(std::make_pair(key, std::move(pt_value)));
    }
    pt.add_child("value", std::move(pt_values));
    return pt;
  }
  
private:
  bool first_input()
  {
    if (first_input_) {
      first_input_ = false;
      return true;
    }
    return false;
  }

  void initialize_default_value_elements()
  {
    value_info_get<0>(&default_value_, value_category());
  }

  template <std::size_t Index>
  void value_info_set(value_type* pval, value_category_scalar) const
  {
    value_info_[0]->__set__(pval);
  }

  template <std::size_t Index>
  void value_info_set(value_type* pval, value_category_pair) const
  {
    value_info_[0]->__set__(&(pval->first));
    value_info_[1]->__set__(&(pval->second));
  }
  
  template <std::size_t Index>
  void value_info_set(value_type* pval, value_category_tuple) const
  {
    typedef std::integral_constant<std::size_t, Index> Index_t;
    value_info_set<Index>(pval, value_category_tuple(), Index_t());
  }

  template <std::size_t Index>
  void value_info_set(value_type* pval, value_category_tuple,
                      std::integral_constant<std::size_t, Index>) const
  {
    value_info_[Index]->__set__(&(std::get<Index>(*pval)));
    typedef std::integral_constant<std::size_t, Index+1> NextIndex_t;
    value_info_set<Index+1>(pval, value_category_tuple(), NextIndex_t());
  }

  template <std::size_t Index>
  void value_info_set(value_type*, value_category_tuple,
                      ValueEnd_t) const
  {
  }

  template <std::size_t Index>
  void value_info_get(value_type* pval, value_category_scalar) const
  {
    value_info_[0]->__get__(pval);
  }

  template <std::size_t Index>
  void value_info_get(value_type* pval, value_category_pair) const
  {
    value_info_[0]->__get__(&(pval->first));
    value_info_[1]->__get__(&(pval->second));
  }
  
  template <std::size_t Index>
  void value_info_get(value_type* pval, value_category_tuple) const
  {
    typedef std::integral_constant<std::size_t, Index> Index_t;
    value_info_get<Index>(pval, value_category_tuple(), Index_t());
  }

  template <std::size_t Index>
  void value_info_get(value_type* pval, value_category_tuple,
                      std::integral_constant<std::size_t, Index>) const
  {
    value_info_[Index]->__get__(&(std::get<Index>(*pval)));
    typedef std::integral_constant<std::size_t, Index+1> NextIndex_t;
    value_info_get<Index+1>(pval, value_category_tuple(), NextIndex_t());
  }

  template <std::size_t Index>
  void value_info_get(value_type*, value_category_tuple,
                      ValueEnd_t) const
  {
  }

  template <std::size_t Index>
  void value_info_output(std::ostream& os, value_category_scalar) const
  {
    os << "    "; value_info_[0]->print(os); os << '\n';
  } 

  template <std::size_t Index>
  void value_info_output(std::ostream& os, value_category_pair) const
  {
    os << "    "; value_info_[0]->print(os); os << '\n';
    os << "    "; value_info_[1]->print(os); os << '\n';
  } 

  template <std::size_t Index>
  void value_info_output(std::ostream& os, value_category_tuple) const
  {
    typedef std::integral_constant<std::size_t, Index> Index_t;
    value_info_output<Index>(os, value_category_tuple(), Index_t());
  }

  template <std::size_t Index>
  void value_info_output(std::ostream& os, value_category_tuple,
                         std::integral_constant<std::size_t, Index>) const
  {
    if (value_enable(Index, value_category_tuple())) {
      os << "    "; value_info_[Index]->print(os); os << '\n';
    }
    typedef std::integral_constant<std::size_t, Index+1> NextIndex_t;
    value_info_output<Index+1>(os, value_category_tuple(), NextIndex_t());
  }

  template <std::size_t Index>
  void value_info_output(std::ostream&, value_category_tuple,
                         ValueEnd_t) const
  {
  }

  template <typename ValueT>
  void set_value_element_impl(const std::string& name, ValueT val)
  {
    if (first_input()) { initialize_default_value_elements(); }

    auto it = find_value_info(name);
    (*it)->set_value(val);
  }

  template <typename ValueT>
  ValueT get_value_element_impl(const std::string& name, ValueT dummy) const
  {
    auto it = find_value_info(name);
    return (*it)->get_value(dummy);
  }

  std::vector<ModuleParam_sptr>::iterator
  find_value_info(const std::string& name)
  {
    std::vector<ModuleParam_sptr>::iterator it = std::begin(value_info_);
    for (; it!=std::end(value_info_); ++it) {
      if ((*it)->name()==name) {
        return it;
      }
    }

    if (it == std::end(value_info_)) {
      const std::string message
        = (boost::format("Parameter is not found: %s") % name).str();
      BOOST_THROW_EXCEPTION( ANLException(message) );
    }
    return it;
  }

  std::vector<ModuleParam_sptr>::const_iterator
  find_value_info(const std::string& name) const
  {
    std::vector<ModuleParam_sptr>::const_iterator it = std::begin(value_info_);
    for (; it!=std::end(value_info_); ++it) {
      if ((*it)->name()==name) {
        return it;
      }
    }

    if (it == std::end(value_info_)) {
      const std::string message
        = (boost::format("Parameter is not found: %s") % name).str();
      BOOST_THROW_EXCEPTION( ANLException(message) );
    }
    return it;
  }

private:
  container_type* ptr_;
  std::string key_name_;
  std::string buffer_key_;
  value_type default_value_;
  std::vector<ModuleParam_sptr> value_info_;
  std::map<int, std::array<std::size_t, ValueSize>> value_enable_;
  // std::map<int, std::array<std::string, ValueSize>> value_question_;
  bool first_input_;
};

} /* namespace anl */
