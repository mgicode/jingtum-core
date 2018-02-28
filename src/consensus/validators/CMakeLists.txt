# consensus/validators

aux_source_directory(. DIR_CONSENSUS_SRCS)
aux_source_directory(./impl DIR_CONSENSUS_VALIDATORS)
add_library(validators ${DIR_CONSENSUS_SRCS} ${DIR_CONSENSUS_VALIDATORS})
