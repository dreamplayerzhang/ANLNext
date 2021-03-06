/**
 * SaveData
 *
 * @author Hirokazu Odaka
 * @date 2008-04-30
 */

#ifndef SaveData_H
#define SaveData_H 1

#include <anlnext/BasicModule.hh>
#include "TFile.h"


class SaveData : public anlnext::BasicModule
{
  DEFINE_ANL_MODULE(SaveData, 2.0);
public:
  SaveData();
  
  anlnext::ANLStatus mod_define() override;
  anlnext::ANLStatus mod_initialize() override;
  anlnext::ANLStatus mod_finalize() override;

  TDirectory* GetDirectory() { return m_RootFile->GetDirectory(0); }
  bool cd() { return m_RootFile->cd(); }
  std::string FileName() const { return m_FileName; }
private:
  std::string m_FileName;
  TFile* m_RootFile;
};

#endif /* SaveData_H */
