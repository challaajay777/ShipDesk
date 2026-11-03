#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

#ifndef MINIZ_EXPORT
#  if defined(_WIN32) && defined(MINIZ_SHARED_LIB)
#    ifdef miniz_EXPORTS
#      define MINIZ_EXPORT __declspec(dllexport)
#    else
#      define MINIZ_EXPORT __declspec(dllimport)
#    endif
#  else
#    define MINIZ_EXPORT
#  endif
#endif

#endif // MINIZ_EXPORT_H
