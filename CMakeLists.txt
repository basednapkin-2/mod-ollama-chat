set(SOURCES
    srcmod-ollama-chat_api.cpp
    srcmod-ollama-chat_command.cpp
    srcmod-ollama-chat_config.cpp
    srcmod-ollama-chat_events.cpp
    srcmod-ollama-chat_handler.cpp
    srcmod-ollama-chat_httpclient.cpp
    srcmod-ollama-chat_main.cpp
    srcmod-ollama-chat_personality.cpp
    srcmod-ollama-chat_querymanager.cpp
    srcmod-ollama-chat_rag.cpp
    srcmod-ollama-chat_random.cpp
    srcmod-ollama-chat_sentiment.cpp
)

add_library(mod-ollama-chat_static STATIC ${SOURCES})
set_target_properties(mod-ollama-chat_static PROPERTIES FOLDER modules)

# --- REQUIRED INCLUDE DIRECTORIES ---
target_include_directories(mod-ollama-chat_static PUBLIC
    # Include the module's own source and dependency directories
    ${CMAKE_CURRENT_SOURCE_DIR}src
    ${CMAKE_CURRENT_SOURCE_DIR}deps # -- ADDED Automatically find Eigen here

    # Include core AzerothCore headers
    ${CMAKE_SOURCE_DIR}srcservergame
    ${CMAKE_SOURCE_DIR}srcservercommon
    ${CMAKE_SOURCE_DIR}srcserverdatabase
)

# This links your module against the compiled AzerothCore libraries.
target_link_libraries(mod-ollama-chat_static LINK_PRIVATE common world)
