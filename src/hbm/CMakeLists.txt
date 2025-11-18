build_lib(
  LIBNAME hbm
  SOURCE_FILES
    model/hbm-bank.cc
    model/hbm-controller.cc
    helper/hbm-helper.cc
  HEADER_FILES
    model/hbm-bank.h
    model/hbm-controller.h
    helper/hbm-helper.h
  LIBRARIES_TO_LINK
    ${libcore}
)

