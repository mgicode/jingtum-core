aux_source_directory(. DIR_COMMON_MISC_MAIN_SRCS)
aux_source_directory(./impl DIR_COMMON_MISC_SRCS)
add_library(misc ${DIR_COMMON_MISC_MAIN_SRCS} ${DIR_COMMON_MISC_SRCS})
