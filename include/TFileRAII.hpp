#ifndef TFileRAII_hpp
#define TFileRAII_hpp 1

#include <TFile.h>

#include <memory>

namespace DELILA
{

// Custom deleter for TFile that ensures Close() is called
struct TFileDeleter {
  void operator()(TFile *file) const
  {
    if (file) {
      file->Close();
      delete file;
    }
  }
};

// Type alias for RAII-managed TFile
using TFilePtr = std::unique_ptr<TFile, TFileDeleter>;

// Helper function to open a TFile with RAII
inline TFilePtr MakeTFile(const char *name, const char *option = "READ")
{
  return TFilePtr(new TFile(name, option));
}

}  // namespace DELILA

#endif
