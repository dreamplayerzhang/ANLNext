/**
 * MyVectorModule sample module of the ANL Next framework
 *
 * @author 
 * @date
 *
 */

#ifndef MyVectorModule_H
#define MyVectorModule_H 1

#include <anlnext/BasicModule.hh>
#include <vector>
#include <tuple>

class MyVectorModule : public anlnext::BasicModule
{
  DEFINE_ANL_MODULE(MyVectorModule, 1.0);
public:
  MyVectorModule();
  virtual ~MyVectorModule();
  
  anlnext::ANLStatus mod_define() override;
  anlnext::ANLStatus mod_analyze() override;

private:
  std::vector<std::tuple<int, std::string, double, double>> myVector_;
  int bufferID_ = 0;
  std::string bufferType_ = "pixel";
  double bufferX_ = 0.0;
  double bufferY_ = 0.0;
};

#endif /* MyVectorModule_H */
