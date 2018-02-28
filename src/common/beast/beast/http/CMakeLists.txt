#http

aux_source_directory(./impl DIR_BEAST_HTTP_SRCS)
aux_source_directory(./impl/http-parser DIR_BEAST_HTTP_P_SRCS)
aux_source_directory(./impl/http-parser/contrib DIR_BEAST_HTTP_P_C_SRCS)
add_library(beast_http ${DIR_BEAST_HTTP_SRCS} ${DIR_BEAST_HTTP_P_SRCS} ${DIR_BEAST_HTTP_P_C_SRCS})
