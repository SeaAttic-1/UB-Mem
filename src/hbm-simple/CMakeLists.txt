build_lib(
  LIBNAME hbm-simple
  SOURCE_FILES
    model/hbm-bank-simple.cc
    model/hbm-controller-simple.cc
    helper/hbm-helper-simple.cc
  HEADER_FILES
    model/hbm-bank-simple.h
    model/hbm-controller-simple.h
    helper/hbm-helper-simple.h
  LIBRARIES_TO_LINK
    ${libcore}
)

