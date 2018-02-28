# consensus

add_subdirectory(validators)
aux_source_directory(. DIR_CONSENSUS_SRCS)
add_library(consensus ${DIR_CONSENSUS_SRCS})
