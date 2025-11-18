build_lib_example(
  NAME ub-quick-example
  SOURCE_FILES hbm-ssim.cc
  LIBRARIES_TO_LINK ${libcore}
                    ${libconfig-store}
                    ${libpoint-to-point}
)
