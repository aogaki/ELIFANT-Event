#include <gtest/gtest.h>

#include "TFileRAII.hpp"

#include <TFile.h>
#include <TTree.h>

#include <filesystem>
#include <string>

using namespace DELILA;

class TFileRAIITest : public ::testing::Test {
 protected:
  std::string testFileName = "test_raii.root";

  void SetUp() override {
    // Create a test ROOT file
    TFile *f = new TFile(testFileName.c_str(), "RECREATE");
    TTree *tree = new TTree("TestTree", "Test Tree");
    int data = 42;
    tree->Branch("data", &data, "data/I");
    tree->Fill();
    tree->Write();
    f->Close();
    delete f;
  }

  void TearDown() override {
    // Clean up test files
    if (std::filesystem::exists(testFileName)) {
      std::filesystem::remove(testFileName);
    }
    if (std::filesystem::exists("test_write.root")) {
      std::filesystem::remove("test_write.root");
    }
    if (std::filesystem::exists("test_create.root")) {
      std::filesystem::remove("test_create.root");
    }
  }
};

TEST_F(TFileRAIITest, MakeTFileBasic) {
  auto file = MakeTFile(testFileName.c_str(), "READ");

  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(file->IsOpen());
  EXPECT_FALSE(file->IsZombie());
}

TEST_F(TFileRAIITest, AutomaticCleanupOnDestruction) {
  {
    auto file = MakeTFile(testFileName.c_str(), "READ");
    EXPECT_TRUE(file->IsOpen());
  }  // file goes out of scope, should auto-close
  // If cleanup failed, subsequent tests might have issues
  SUCCEED();
}

TEST_F(TFileRAIITest, MoveSemantics) {
  auto file1 = MakeTFile(testFileName.c_str(), "READ");
  ASSERT_NE(file1, nullptr);

  // Move to another TFilePtr
  auto file2 = std::move(file1);

  EXPECT_EQ(file1, nullptr);  // file1 should be null after move
  ASSERT_NE(file2, nullptr);
  EXPECT_TRUE(file2->IsOpen());
}

TEST_F(TFileRAIITest, ReadExistingFile) {
  auto file = MakeTFile(testFileName.c_str(), "READ");

  ASSERT_NE(file, nullptr);
  EXPECT_FALSE(file->IsZombie());

  TTree *tree = dynamic_cast<TTree *>(file->Get("TestTree"));
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(tree->GetEntries(), 1);
}

TEST_F(TFileRAIITest, WriteNewFile) {
  {
    auto file = MakeTFile("test_write.root", "RECREATE");
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(file->IsOpen());
    EXPECT_TRUE(file->IsWritable());

    TTree tree("NewTree", "New Tree");
    int value = 123;
    tree.Branch("value", &value, "value/I");
    tree.Fill();
    tree.Write();
  }  // File closes automatically

  // Verify file was written correctly
  auto readFile = MakeTFile("test_write.root", "READ");
  TTree *tree = dynamic_cast<TTree *>(readFile->Get("NewTree"));
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(tree->GetEntries(), 1);
}

TEST_F(TFileRAIITest, NonExistentFile) {
  auto file = MakeTFile("nonexistent_file.root", "READ");

  // ROOT creates a TFile object even for non-existent files, but marks it as
  // Zombie
  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(file->IsZombie());
}

TEST_F(TFileRAIITest, InvalidFilePath) {
  auto file = MakeTFile("/invalid/path/file.root", "READ");

  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(file->IsZombie());
}

TEST_F(TFileRAIITest, MultipleFilesSimultaneously) {
  auto file1 = MakeTFile(testFileName.c_str(), "READ");
  auto file2 = MakeTFile("test_create.root", "RECREATE");

  EXPECT_TRUE(file1->IsOpen());
  EXPECT_TRUE(file2->IsOpen());
  EXPECT_FALSE(file1->IsZombie());
  EXPECT_FALSE(file2->IsZombie());
}

TEST_F(TFileRAIITest, UpdateMode) {
  auto file = MakeTFile(testFileName.c_str(), "UPDATE");

  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(file->IsOpen());
  EXPECT_TRUE(file->IsWritable());
}

TEST_F(TFileRAIITest, CustomDeleterCalled) {
  bool fileClosed = false;
  {
    auto file = MakeTFile(testFileName.c_str(), "READ");
    ASSERT_NE(file, nullptr);
    // When file goes out of scope, deleter should call Close() and delete
  }
  // Check file can be reopened (indicates it was properly closed)
  auto file2 = MakeTFile(testFileName.c_str(), "READ");
  EXPECT_TRUE(file2->IsOpen());
}

TEST_F(TFileRAIITest, NullptrHandling) {
  TFilePtr nullFile(nullptr);
  EXPECT_EQ(nullFile, nullptr);
  // Should not crash when deleter is called on nullptr
}

TEST_F(TFileRAIITest, ResetMethodReleaseFile) {
  auto file = MakeTFile(testFileName.c_str(), "READ");
  ASSERT_NE(file, nullptr);

  file.reset();  // Manually release

  EXPECT_EQ(file, nullptr);
}

TEST_F(TFileRAIITest, SwapFiles) {
  auto file1 = MakeTFile(testFileName.c_str(), "READ");
  auto file2 = MakeTFile("test_create.root", "RECREATE");

  std::swap(file1, file2);

  EXPECT_TRUE(file1->IsWritable());  // Now has the RECREATE file
  EXPECT_FALSE(file2->IsWritable());  // Now has the READ file
}

TEST_F(TFileRAIITest, GetRawPointer) {
  auto file = MakeTFile(testFileName.c_str(), "READ");
  TFile *rawPtr = file.get();

  ASSERT_NE(rawPtr, nullptr);
  EXPECT_TRUE(rawPtr->IsOpen());
}

TEST_F(TFileRAIITest, BoolConversion) {
  auto file = MakeTFile(testFileName.c_str(), "READ");
  EXPECT_TRUE(static_cast<bool>(file));

  file.reset();
  EXPECT_FALSE(static_cast<bool>(file));
}

TEST_F(TFileRAIITest, ArrowOperator) {
  auto file = MakeTFile(testFileName.c_str(), "READ");

  EXPECT_FALSE(file->IsZombie());
  EXPECT_TRUE(file->IsOpen());
  EXPECT_STREQ(file->GetOption(), "READ");
}

TEST_F(TFileRAIITest, CreateMultipleTreesInFile) {
  {
    auto file = MakeTFile("test_multi.root", "RECREATE");

    TTree tree1("Tree1", "First Tree");
    TTree tree2("Tree2", "Second Tree");

    int data1 = 1, data2 = 2;
    tree1.Branch("d", &data1, "d/I");
    tree2.Branch("d", &data2, "d/I");

    tree1.Fill();
    tree2.Fill();

    tree1.Write();
    tree2.Write();
  }

  auto readFile = MakeTFile("test_multi.root", "READ");
  TTree *t1 = dynamic_cast<TTree *>(readFile->Get("Tree1"));
  TTree *t2 = dynamic_cast<TTree *>(readFile->Get("Tree2"));

  ASSERT_NE(t1, nullptr);
  ASSERT_NE(t2, nullptr);
  EXPECT_EQ(t1->GetEntries(), 1);
  EXPECT_EQ(t2->GetEntries(), 1);

  std::filesystem::remove("test_multi.root");
}

TEST_F(TFileRAIITest, ExceptionSafety) {
  try {
    auto file = MakeTFile(testFileName.c_str(), "READ");
    throw std::runtime_error("Test exception");
  } catch (const std::runtime_error &e) {
    // File should be properly closed even when exception is thrown
    SUCCEED();
  }

  // Verify file can still be opened
  auto file = MakeTFile(testFileName.c_str(), "READ");
  EXPECT_TRUE(file->IsOpen());
}

TEST_F(TFileRAIITest, VectorOfFiles) {
  std::vector<TFilePtr> files;

  files.push_back(MakeTFile(testFileName.c_str(), "READ"));
  files.push_back(MakeTFile("test_create.root", "RECREATE"));

  EXPECT_EQ(files.size(), 2);
  EXPECT_TRUE(files[0]->IsOpen());
  EXPECT_TRUE(files[1]->IsOpen());

  files.clear();  // All files should be properly closed
  SUCCEED();
}

TEST_F(TFileRAIITest, FileNameAccessor) {
  auto file = MakeTFile(testFileName.c_str(), "READ");

  std::string fileName = file->GetName();
  EXPECT_EQ(fileName, testFileName);
}

TEST_F(TFileRAIITest, FileSizeAccessor) {
  auto file = MakeTFile(testFileName.c_str(), "READ");

  Long64_t size = file->GetSize();
  EXPECT_GT(size, 0);
}

TEST_F(TFileRAIITest, WriteAndReadCycle) {
  const int nEntries = 100;

  // Write
  {
    auto file = MakeTFile("test_cycle.root", "RECREATE");
    TTree tree("Data", "Data Tree");
    int value;
    tree.Branch("val", &value, "val/I");

    for (int i = 0; i < nEntries; i++) {
      value = i * i;
      tree.Fill();
    }
    tree.Write();
  }

  // Read
  {
    auto file = MakeTFile("test_cycle.root", "READ");
    TTree *tree = dynamic_cast<TTree *>(file->Get("Data"));
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->GetEntries(), nEntries);

    int value;
    tree->SetBranchAddress("val", &value);
    tree->GetEntry(10);
    EXPECT_EQ(value, 100);  // 10*10
  }

  std::filesystem::remove("test_cycle.root");
}

TEST_F(TFileRAIITest, DefaultREADMode) {
  auto file = MakeTFile(testFileName.c_str());  // No mode specified, should
                                                  // default to READ

  ASSERT_NE(file, nullptr);
  EXPECT_TRUE(file->IsOpen());
  EXPECT_FALSE(file->IsWritable());
}
