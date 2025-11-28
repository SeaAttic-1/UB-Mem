build_lib(
  LIBNAME hbm
  SOURCE_FILES
    model/hbm-bank.cc
    model/hbm-controller.cc
    model/hbm-bank-group.cc
    model/hbm-pseudo-channel.cc
    model/hbm-stack.cc
    helper/hbm-helper.cc
  HEADER_FILES
    model/hbm-bank.h
    model/hbm-controller.h
    model/hbm-controller.h
    model/hbm-bank-group.h
    model/hbm-pseudo-channel.h
    model/hbm-stack.h
    model/hbm-macro.h
    helper/hbm-helper.h
  LIBRARIES_TO_LINK
    ${libcore}
)

