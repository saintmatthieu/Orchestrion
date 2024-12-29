// The DSPFilters library includes the following code in its Common.h file:
// #ifdef _MSC_VER
// namespace tr1 = std::tr1; // But this line, from 2010, now fails to compile.
// #else
// namespace tr1 = std;
// #endif
//
// We alias std to std::tr1 and force the build system to include this file (see DSPFilters.cmake)
#if defined(_MSC_VER) && !defined(USE_TR1)
namespace std
{
namespace tr1 = std;
}
#endif
