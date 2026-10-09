/* ps3core.h - C API of ps3core.dll (consumed by the C# GUI through P/Invoke). */
#ifndef PS3CORE_H
#define PS3CORE_H
#ifdef _WIN32
  #ifdef PS3CORE_BUILD
    #define PS3CORE_API __declspec(dllexport)
  #else
    #define PS3CORE_API __declspec(dllimport)
  #endif
#else
  #define PS3CORE_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1. Bump when the API changes. */
PS3CORE_API int ps3_core_version(void);

/* Phase 1: create <root_dir>/<game_name>/{input,codebase,output} and copy the ELF to input/.
 * Writes the project directory into project_dir. Returns 0 on success. */
PS3CORE_API int ps3_create_project(const char* root_dir, const char* game_name, const char* elf_path,
                                   char* project_dir, int project_dir_len, char* err, int err_len);

/* Phase 2/3: parse the ELF in input/, write generated C++ to codebase/. Returns 0 on success; log = summary. */
PS3CORE_API int ps3_lift_project(const char* project_dir, char* log, int log_len);

/* Phase 6: copy runtime + codebase into output/, generate build.ninja and compile game.exe + ps3rt.dll.
 * runtime_dir = folder with ps3rt.cpp etc, compilers_dir = Compilers-files. Returns 0 on success. */
PS3CORE_API int ps3_build_project(const char* project_dir, const char* runtime_dir,
                                  const char* compilers_dir, char* log, int log_len);

#ifdef __cplusplus
}
#endif
#endif
