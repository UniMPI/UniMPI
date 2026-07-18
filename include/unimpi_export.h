/* include/unimpi_export.h - API Export/Visibility Macros
 *
 * Defines UNIMPI_API for proper symbol visibility when building/using
 * unimpi as a shared library (DLL on Windows).
 */
#ifndef UNIMPI_EXPORT_H
#define UNIMPI_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* For static library builds, no special export attributes needed */
#ifndef UNIMPI_SHARED_BUILD
    #define UNIMPI_API
    #define UNIMPI_LOCAL
#else
    /* Define UNIMPI_API for Windows DLL export/import */
    #ifdef _WIN32
        #ifdef UNIMPI_BUILDING_DLL
            /* Building unimpi as DLL - export symbols */
            #define UNIMPI_API __declspec(dllexport)
        #else
            /* Using unimpi as DLL - import symbols */
            #define UNIMPI_API __declspec(dllimport)
        #endif
        #define UNIMPI_LOCAL
    #else
        /* Non-Windows platforms - use GCC visibility attributes */
        #if defined(__GNUC__) && __GNUC__ >= 4
            #define UNIMPI_API __attribute__((visibility("default")))
            #define UNIMPI_LOCAL __attribute__((visibility("hidden")))
        #else
            #define UNIMPI_API
            #define UNIMPI_LOCAL
        #endif
    #endif
#endif

/* Deprecated function marker */
#ifdef __GNUC__
    #define UNIMPI_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
    #define UNIMPI_DEPRECATED __declspec(deprecated)
#else
    #define UNIMPI_DEPRECATED
#endif

#ifdef __cplusplus
}
#endif

#endif /* UNIMPI_EXPORT_H */
