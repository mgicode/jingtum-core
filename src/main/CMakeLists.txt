set (TARGET_NAME skywelld)

aux_source_directory(. DIR_SRCS)
add_executable(${TARGET_NAME} ${DIR_SRCS})

# Add boost lib
set (BOOST_LIBS coroutine context date_time filesystem program_options regex system thread)
if (NOT "$ENV{BOOST_ROOT}" STREQUAL "")
	# Boost is in global
	include_directories ($ENV{BOOST_ROOT})
else ()
	message (FATAL_ERROR "Failed to find required dependency: boost")
endif ()


#target_link_libraries(${TARGET_NAME} database nodestore network protocol validators misc transaction ledger service transaction ledger consensus crypto base core misc json consensus shamap) 

target_link_libraries(${TARGET_NAME} database nodestore network protocol validators misc transaction ledger service transaction ledger consensus crypto base core misc json consensus shamap beast_net beast_insight beast_utility beast_asio beast_chrono beast_hash beast_module beast_strings beast_http beast_threads )  

target_link_libraries(${TARGET_NAME} libboost_coroutine.a)
target_link_libraries(${TARGET_NAME} libboost_context.a)
target_link_libraries(${TARGET_NAME} libboost_date_time.a)
target_link_libraries(${TARGET_NAME} libboost_filesystem.a)
target_link_libraries(${TARGET_NAME} libboost_program_options.a)
target_link_libraries(${TARGET_NAME} libboost_regex.a)
target_link_libraries(${TARGET_NAME} libboost_system.a)
target_link_libraries(${TARGET_NAME} libboost_thread.a)

target_link_libraries(${TARGET_NAME} libssl.a)
target_link_libraries(${TARGET_NAME} liblz4.a)
target_link_libraries(${TARGET_NAME} libprotobuf.a)

target_link_libraries(${TARGET_NAME} libsoci_core.a)
target_link_libraries(${TARGET_NAME} libsoci_mysql.a)
target_link_libraries(${TARGET_NAME} libsoci_sqlite3.a)
target_link_libraries(${TARGET_NAME} libmysqlclient.a)
target_link_libraries(${TARGET_NAME} libsqlite3.a)
target_link_libraries(${TARGET_NAME} librocksdb.a)

target_link_libraries(${TARGET_NAME} -lz)
target_link_libraries(${TARGET_NAME} -lpthread)
target_link_libraries(${TARGET_NAME} -lrt)
target_link_libraries(${TARGET_NAME} -lcrypto)
target_link_libraries(${TARGET_NAME} -ldl)

