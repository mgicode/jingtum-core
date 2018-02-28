# network
aux_source_directory(./peerfinder/impl DIR_PEERFINDER_IMPL_SRCS)
aux_source_directory(./overlay/impl DIR_OVERLAY_IMPL_SRCS)
aux_source_directory(./peers DIR_PEERS_IMPL_SRCS)
aux_source_directory(./resource/impl DIR_RESOURCE_IMPL_SRCS)
aux_source_directory(. DIR_NETWORK_SRCS)
add_library(network ${DIR_NETWORK_SRCS} ${DIR_PEERFINDER_IMPL_SRCS} ${DIR_OVERLAY_IMPL_SRCS} ${DIR_PEERS_IMPL_SRCS} ${DIR_RESOURCE_IMPL_SRCS})

