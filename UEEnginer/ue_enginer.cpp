#include "ue_enginer.h"
#include <functional>
#include <string>
#include <vector>
#include <assert.h>

#include "engine.h"
#include "wrappers.h"

struct UEEnginer
{
  uint8_t* image;
  size_t size;
  mem_reader_t mm_reader;
  std::vector<std::string> find_pakcages_name;
  std::vector<UE_UPackage> pakcages;
};



void* ue_enginer_init(const char* gameName, void* image, size_t size, mem_reader_t mmReader)
{
  STATUS status = EngineInit(gameName, image);
  if(status != STATUS::SUCCESS)
    return nullptr;
    
  UEEnginer* enginer = new UEEnginer();
  if(enginer == nullptr)
    return nullptr;
  enginer->image = (uint8_t*)image;
  enginer->size = size;
  enginer->mm_reader = mmReader;
  return enginer;
}

bool ue_enginer_process(void* enginer)
{
  UEEnginer* _enginer = (UEEnginer*)enginer;
  std::unordered_map<uint8*, std::vector<UE_UObject>> packages_map;
  std::function<void(UE_UObject)> callback = [_enginer, &packages_map](UE_UObject object) {
    auto isFunction = object.IsA<UE_UFunction>();
    if (isFunction || object.IsA<UE_UStruct>() || object.IsA<UE_UEnum>()) 
    {

      auto packageObj = object.GetPackageObject();

      auto iter_begin = std::begin(_enginer->find_pakcages_name);
      auto iter_end = std::end(_enginer->find_pakcages_name);
      auto iter = std::find(iter_begin, iter_end, packageObj.GetName());
      if (iter != iter_end)
        packages_map[packageObj].push_back(object);
    }
  };


  ObjObjects.Dump(callback);
  if (!packages_map.size())
    return false;
  
  for (UE_UPackage package : packages_map)
  {
    package.Process();
    _enginer->pakcages.emplace_back(std::move(package));
  }
  return true;
}

void ue_enginer_add_find_package(void* enginer, const char* name)
{
  UEEnginer* _enginer = (UEEnginer*)enginer;
  _enginer->find_pakcages_name.emplace_back(name);
}

static bool ue_enginer_convert_structure(UE_UPackage::Struct* structure, int& field_index, int& func_index, UE_Structure* out_structure)
{
  out_structure->size = structure->Size;

  if (!structure->Members.empty())
  {
    for (auto& m : structure->Members)
    {
      UE_Field* f = &out_structure->fields[field_index++];
      f->name = strdup(m.Name.data());
      f->offset = m.Offset;
      f->size = m.Size;
    }
  }

  if (!structure->Functions.empty())
  {
    for (auto& f : structure->Functions)
    {
      UE_Function* f2 = &out_structure->functions[func_index++];
      f2->name = strdup(f.CppName.data());
      f2->address = f.Func;
    }
  }

  return true;
}

static bool ue_enginer_convert_structure_all(UE_UPackage::Struct* structure, UE_Structure* out_structure)
{
  out_structure->size = structure->Size;

  for (auto iter = structure->supers.rbegin(); iter != structure->supers.rend(); ++iter)
  {
    out_structure->field_count += (int)iter->Members.size();
    out_structure->function_count += (int)iter->Functions.size();
  }
  out_structure->field_count += (int)structure->Members.size();
  out_structure->function_count += (int)structure->Functions.size();

  out_structure->fields = (UE_Field*)malloc(out_structure->field_count * sizeof(UE_Field));
  if(out_structure->fields == NULL)
    return false;

  out_structure->functions = (UE_Function*)malloc(out_structure->function_count * sizeof(UE_Function));
  if (out_structure->functions == NULL)
  {
    free(out_structure->fields);
    return false;
  }

  int field_index = 0;
  int func_index = 0;
  for (auto iter = structure->supers.rbegin(); iter != structure->supers.rend(); ++iter)
  {
    ue_enginer_convert_structure(&*iter, field_index, func_index, out_structure);
  }

  ue_enginer_convert_structure(structure, field_index, func_index, out_structure);

  assert(field_index == out_structure->field_count);
  assert(func_index == out_structure->function_count);
  return true;
}

bool ue_enginer_find_struct(void* enginer, const char* name, UE_Structure* out_structure)
{
  UEEnginer* _enginer = (UEEnginer*)enginer;

  memset(out_structure, 0, sizeof(UE_Structure));
  auto& packages = _enginer->pakcages;
  for (UE_UPackage& package : packages)
  {
    auto& classes  = package.GetStructures() ;
    auto iter_begin = std::begin(classes);
    auto iter_end = std::end(classes);
    
    auto iter = std::find_if(iter_begin, iter_end, [name](const UE_UPackage::Struct& strucate) {
      return strucate.CppName == name;
    });

    if (iter != iter_end)
    {
      return ue_enginer_convert_structure_all(&*iter, out_structure);
    }
  }
  return false;
}

bool ue_enginer_struct_get_field(UE_Structure* structure, const char* name, UE_Field* out_field)
{
  for (int i = 0; i < structure->field_count; ++i)
  {
    UE_Field* f = &structure->fields[i];
    if (f && strcmp(f->name, name) == 0)
    {
      out_field = f;
      return true;
    }
  }
  return false;
}

bool ue_enginer_struct_get_function(UE_Structure* structure, const char* name, UE_Function* out_func)
{
  for (int i = 0; i < structure->function_count; ++i)
  {
    UE_Function* f = &structure->functions[i];
    if (f && strcmp(f->name, name) == 0)
    {
      out_func = f;
      return true;
    }
  }
  return false;
}

void ue_enginer_free_struct(UE_Structure* strucature)
{
  for (int i = 0; i < strucature->field_count; ++i)
  {
    UE_Field* f = &strucature->fields[i];
    free(f->name);
  }

  for (int i = 0; i < strucature->field_count; ++i)
  {
    UE_Function* f = &strucature->functions[i];
    free(f->name);
  }

  free(strucature->fields);
  free(strucature->functions);
}

bool ue_enginer_is_instance(void* enginer, const char* class_name, const void* object)
{
  UEEnginer* _enginer = (UEEnginer*)enginer;
  UE_UObject obj((void*)object);
  return obj.GetClass().GetCppName() == class_name;
}

bool ue_enginer_is_inherit(void* enginer, const char* baseclass_name, const void* object)
{
  UEEnginer* _enginer = (UEEnginer*)enginer;
  UE_UObject obj((void*)object);

  for (auto c = obj.GetClass().Cast<UE_UStruct>(); c; c = c.GetSuper())
  {
    if (c.GetCppName() == baseclass_name)
    {
      return true;
    }

  }
  return false;
}

void ue_enginer_destorty(void* enginer)
{
  UEEnginer* _enginer = (UEEnginer*)enginer;
  delete _enginer;
}
