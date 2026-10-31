/* Force rsx.dll import library generation */
#if defined(_WIN32)
__declspec(dllexport) int ppsx33_rsx_module_version(void) { return 1; }
#endif
