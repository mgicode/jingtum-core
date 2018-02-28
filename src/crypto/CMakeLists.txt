aux_source_directory(./impl DIR_CRYPTO_IMPL_SRCS)
aux_source_directory(./ed25519-donna DIR_CRYPTO_DONNA_SRCS)
add_library(crypto ${DIR_CRYPTO_IMPL_SRCS} ${DIR_CRYPTO_DONNA_SRCS})

