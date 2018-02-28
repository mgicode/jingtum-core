#crypto/donna

aux_source_directory(. DIR_CRYPTO_DONNA_SRCS)
aux_source_directory(./fuzz DIR_CRYPTO_DONNA_FUZZ_SRCS)
add_library(donna ${DIR_CRYPTO_DONNA_SRCS} ${DIR_CRYPTO_DONNA_FUZZ_SRCS})
