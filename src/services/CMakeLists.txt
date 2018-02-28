# serverces

aux_source_directory(net/impl DIR_NET_SRCS)
aux_source_directory(rpc/handlers DIR_RPC_H_SRCS)
aux_source_directory(rpc/impl DIR_RPC_I_SRCS)
aux_source_directory(server/impl DIR_SERVER_I_SRCS)
aux_source_directory(websocket DIR_WEBSOCKET_SRCS)
add_library(service ${DIR_NET_SRCS} ${DIR_RPC_H_SRCS} ${DIR_RPC_I_SRCS} ${DIR_SERVER_I_SRCS} ${DIR_WEBSOCKET_SRCS})
