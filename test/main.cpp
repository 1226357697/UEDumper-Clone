#include <iostream>
#include "../UEEnginer/ue_enginer.h"
#include "../Dumper/defs.h"
#include "../Dumper/utils.h"
#include "../Dumper/memory.h"
#include <Windows.h>
#include <filesystem>
namespace fs = std::filesystem;

STATUS test(PVOID* pimage, uint64_t* psize)
{
  uint32_t pid = 0;

  {
    HWND hWnd = FindWindowA("UnrealWindow", nullptr);
    if (!hWnd) {
      return STATUS::WINDOW_NOT_FOUND;
    };
    GetWindowThreadProcessId(hWnd, (DWORD*)(&pid));
    if (!pid) {
      return STATUS::PROCESS_NOT_FOUND;
    };
  }

  if (!ReaderInit(pid)) {
    return STATUS::READER_ERROR;
  };

  fs::path processName;

  {
    wchar_t processPath[MAX_PATH]{};
    if (!GetProccessPath(pid, processPath, MAX_PATH)) { return STATUS::CANNOT_GET_PROCNAME; };
    processName = fs::path(processPath).filename();
    printf("Found UE4 game: %ls\n", processName.c_str());
  }

  {


    uint64_t size = GetImageSize();
    if (!size) { return STATUS::MODULE_NOT_FOUND; }

    PVOID Image = VirtualAlloc(0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!Read((void*)Base, Image, size)) {
      return STATUS::CANNOT_READ;
    }
    *pimage = Image;
    *psize = size;
  }

  return STATUS::SUCCESS;
}

int main()
{
  PVOID image = 0;
  uint64_t size =0 ;
  if (test(&image, &size) != STATUS::SUCCESS)
  {
    return 1;
  }

  void* enginer = ue_enginer_init("PAYDAY3Client-Win64-Shipping", image, size, [](uintptr_t address, size_t size, void* buffer)->auto{
    Read((void*)address, buffer, size);
    return buffer;
  });

  ue_enginer_add_find_package(enginer, "Engine");
  ue_enginer_process(enginer);

  //UE_Structure s;
  //ue_enginer_find_struct(enginer, "UWorld", &s);

  PVOID world = (PVOID)0x0000017F2E9C31A0;
  ue_enginer_is_instance(enginer, "UWorld", world);
  ue_enginer_is_inherit(enginer, "UObject", world);


  return 0;
}