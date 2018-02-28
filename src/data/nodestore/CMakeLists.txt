# data/nodestore

aux_source_directory(./backend DIR_DATA_NODE_BACKEND_SRCS)
aux_source_directory(./impl DIR_DATA_NODE_IMPL_SRCS)
add_library(nodestore ${DIR_DATA_NODE_BACKEND_SRCS} ${DIR_DATA_NODE_IMPL_SRCS})
