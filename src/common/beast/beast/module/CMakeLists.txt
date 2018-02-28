#module

aux_source_directory(./asio            DIR_BEAST_M_ASIO_SRCS)
aux_source_directory(./core/diagnostic DIR_BEAST_M_CORE_D_SRCS)
aux_source_directory(./core/files      DIR_BEAST_M_CORE_F_SRCS)
aux_source_directory(./core/maths      DIR_BEAST_M_CORE_MA_SRCS)
aux_source_directory(./core/memory     DIR_BEAST_M_CORE_ME_SRCS)
aux_source_directory(./core/misc       DIR_BEAST_M_CORE_MI_SRCS)
aux_source_directory(./core/native     DIR_BEAST_M_CORE_NA_SRCS)
aux_source_directory(./core/streams    DIR_BEAST_M_CORE_ST_SRCS)
aux_source_directory(./core/system     DIR_BEAST_M_CORE_SY_SRCS)
aux_source_directory(./core/text       DIR_BEAST_M_CORE_TX_SRCS)
aux_source_directory(./core/thread     DIR_BEAST_M_CORE_TH_SRCS)
aux_source_directory(./core/time       DIR_BEAST_M_CORE_TI_SRCS)

add_library(beast_module 
${DIR_BEAST_M_ASIO_SRCS}
${DIR_BEAST_M_CORE_D_SRCS}
${DIR_BEAST_M_CORE_F_SRCS}
${DIR_BEAST_M_CORE_MA_SRCS}
${DIR_BEAST_M_CORE_ME_SRCS}
${DIR_BEAST_M_CORE_MI_SRCS}
${DIR_BEAST_M_CORE_NA_SRCS}
${DIR_BEAST_M_CORE_ST_SRCS}
${DIR_BEAST_M_CORE_SY_SRCS}
${DIR_BEAST_M_CORE_TX_SRCS}
${DIR_BEAST_M_CORE_TH_SRCS}
${DIR_BEAST_M_CORE_TI_SRCS}
)
