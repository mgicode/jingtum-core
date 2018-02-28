# transcation
aux_source_directory(book/impl DIR_BOOK_SRCS)
aux_source_directory(paths DIR_PATH_MAIN_SRCS)
aux_source_directory(paths/cursor DIR_PATH_CURSOR_SRCS)
aux_source_directory(transactors DIR_TRANSACTOR_SRCS)
aux_source_directory(tx DIR_TX_SRCS)

add_library(transaction ${DIR_BOOK_SRCS} ${DIR_PATH_MAIN_SRCS} ${DIR_PATH_CURSOR_SRCS} ${DIR_TRANSACTOR_SRCS} ${DIR_TX_SRCS})
