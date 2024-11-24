#pragma once
#include <stdint.h>

typedef void*(*mem_reader_t)(uintptr_t address, size_t size, void* buffer);

typedef struct _UE_Field {
  char* name;
  uint32_t offset;
  uint32_t size;
}UE_Field;

typedef struct _UE_Function {
  char* name;
  uintptr_t address;
}UE_Function;

typedef struct _UE_Structure {
  uint32_t size;
  int field_count;
  UE_Field* fields;
  int function_count;
  UE_Function* functions;
}UE_Structure;


void* ue_enginer_init(const char* gameName, void* image, size_t size, mem_reader_t mmReader);

bool ue_enginer_process(void* enginer);

void ue_enginer_add_find_package(void* enginer, const char* name);

bool ue_enginer_find_struct(void* enginer, const char* name, UE_Structure* out_structure);

bool ue_enginer_struct_get_field(UE_Structure* structure, const char* name, UE_Field* out_field);

bool ue_enginer_struct_get_function( UE_Structure* structure, const char* name, UE_Function* out_func);

void ue_enginer_free_struct(UE_Structure* strucature);

bool ue_enginer_is_instance(void* enginer, const char* class_name, const void* object);

bool ue_enginer_is_inherit(void* enginer, const char* baseclass_name, const void* object);

void ue_enginer_destorty(void* enginer);