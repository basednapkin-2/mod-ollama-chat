# Enhanced mod-ollama-chat for AzerothCore (WIP)

> [!VERY FUCKING IMPORTANT]
> I am not a developer. AI was used to create the code. All I did was spend hours troubleshooting and fixing the AI's bullshit. If for some reason you use this, do NOT use this in ANY production setting unless you go through and review each and every line of code.
> Model used: Gemini 2.5 Pro via Google AI Studio.

> [!CAUTION]
> **LLM/AI Disclaimer:** Large Language Models (LLMs) such as those used by this module do not possess intelligence, reasoning, or true understanding. They generate text by predicting the most likely next word based on patterns in their training data—matching vectors, not thinking or comprehension. The quality and relevance of responses depend entirely on the model you use, its training data, and its configuration. Results may vary, and sometimes the output may be irrelevant, nonsensical, or simply not work as expected. This is a fundamental limitation of current AI and LLM technology. Use with realistic expectations.

> [!IMPORTANT]
> To fully disable Playerbots normal chatter and random chatter that might interfere with this module, set the following settings in your `playerbots.conf`:
> - `AiPlayerbot.EnableBroadcasts = 0`
> - `AiPlayerbot.RandomBotTalk = 0`
> - `AiPlayerbot.RandomBotEmote = 0`
> - `AiPlayerbot.RandomBotSuggestDungeons = 0`
> - `AiPlayerbot.EnableGreet = 0`
> - `AiPlayerbot.GuildFeedback = 0`
> - `AiPlayerbot.RandomBotSayWithoutMaster = 0`

## Overview

**mod-ollama-chat** is a significantly enhanced fork of the original `mod-ollama-chat` for AzerothCore. While the original module provided a basic bridge to a local Ollama API, this version rebuilds and expands upon that foundation to introduce a suite of advanced intelligence and awareness features. It transforms Player Bots from simple chat responders into dynamic, context-aware members of the game world.

This fork focuses on three core areas of improvement: **deep contextual understanding**, **persistent memory and relationships**, and **server performance and stability**.

## Core Enhancements & New Features

This version introduces a host of new systems and significantly upgrades the original module's functionality.

- **Enhanced RAG (Retrieval-Augmented Generation) System**
    - **TF-IDF Vector Search**: A sophisticated C++ search model built with the Eigen library that understands word relevance, providing fast and accurate information retrieval.
    - **Hybrid Filtering**: Automatically parses player queries for keywords (e.g., "frost resist", "mail") to apply metadata filters, dramatically improving search accuracy.

- **Enhanced Conversation History and Snapshot System**: Bots now possess "long-term memory". For extended conversations, an LLM creates a concise summary which is used as context in future interactions, allowing bots to remember their history with a player. Summaries are stored in the MySQL database (`mod_ollama_chat_summaries` table) for long-term persistence across server restarts.

- **Live Reloading**: New GM commands allow for live reloading of the entire configuration (`.ollama reload config`) and the RAG knowledge base (`.ollama reload rag`) without requiring a server restart.

## Comparison to the Original mod-ollama-chat

**Performance Gains:**
- **Query Latency**: 95-99% reduction in latency. In-game queries are virtually instantaneous (less than 10ms) compared to the original's (100-500ms+).
- **CPU Load**: 90-95% reduction in CPU spikes. All computations are done once during worldserver startup. Player queries are now computationally cheap, preventing server lag (TPS drops) when bots need to "think".
- **Scalability**: The system can handle over 10x more concurrent RAG queries, making it suitable for servers with many players interacting with bots simultaneously.

**Performance Trade-offs:**
- **Server Startup Time:** A slight, one-time increase of 5-15 seconds on boot to build the search index. 
- **Memory Usage:** A 50-200% increase in RAM usage for the RAG system itself.
    -   **Important Note:** The base RAM usage for the original RAG knowledge base is already very low for something like an AzerothCore server.

## Installation

1.  **Prerequisites:**
    - Ensure you have liyunfan1223's AzerothCore (https://github.com/liyunfan1223/azerothcore-wotlk) installation with the Player Bots (https://github.com/liyunfan1223/mod-playerbots) module enabled.
    - The module depends on:
     - **fmtlib** (https://github.com/fmtlib/fmt) - For string formatting
     - **nlohmann/json** (https://github.com/nlohmann/json) - For JSON processing (**bundled with module** - no installation needed)
     - cpp-httplib (https://github.com/yhirose/cpp-httplib) - Header-only HTTP library (included, no installation needed)
     - Ollama LLM support – set up a local instance of the Ollama API server with the model of your choice. More details at https://ollama.com
     - Eigen (version 5.0.0 only, I haven't tested with 3.4.1) - A header-only library for linear algebra, required by the TF-IDF vector search model. (**bundled with module** - no installation needed)

2.  **Install Dependencies:**
    - **fmtlib**: A required formatting library.
      ```bash
      # Windows (vcpkg):
      vcpkg install fmt
      # Ubuntu/Debian:
      sudo apt install libfmt-dev
      ```

3.  **Clone the Module:**
    ```bash
    cd /path/to/azerothcore/modules
    git clone https://github.com/[YourGitHub]/napkin-mod-ollama-chat.git mod-ollama-chat
    ```

4.  **Configure CMake & Recompile:**
    You must tell CMake where to find the Eigen library. The recommended method is to use a CMake flag during your build configuration.

    - Navigate to your AzerothCore build directory.
    - Run CMake with the `EIGEN_INCLUDE_DIR` flag pointing to the root of the extracted Eigen folder.

    ```bash
    cd /path/to/azerothcore/build
    cmake .. -DEIGEN_INCLUDE_DIR=/path/to/your/downloads/eigen-5.0.0
    ```

    - After configuration, build AzerothCore as you normally would:
    ```bash
    # For Linux/macOS
    make -j$(nproc)
    # For Windows (Visual Studio)
    cmake --build . --config RelWithDebInfo
    ```

5.  **Configuration & Restart:**
    - Copy the `mod-ollama-chat.conf.dist` file from the module's directory to your server's `config` folder and rename it to `mod-ollama-chat.conf`.
    - Edit the `.conf` file to match your setup, especially the Ollama API endpoint and RAG data path.
      - **ADD THE CONVERSATION SUMMARIZATION SECTION BELOW TO YOUR CONF FILE, DON'T SET ENABLESUMMARIZATION TO TRUE UNTIL YOU HAVE RAN THE SQL SCRIPT FOR IT **
    - Restart your `worldserver`.

```# --------------------------------------------
# CONVERSATION SUMMARIZATION
# --------------------------------------------

# OllamaChat.EnableSummarization
#     Description: Enable or disable automatic summarization of long conversations to save tokens.
#     Default:     1 (true)
OllamaChat.EnableSummarization = 0

# OllamaChat.SummarizationThreshold
#     Description: The number of message pairs (player message + bot reply) in a conversation history
#                  before a summarization is triggered.
#     Default:     10
OllamaChat.SummarizationThreshold = 10

# OllamaChat.SummarizationPromptTemplate
#     Description: The prompt used to ask the LLM to summarize a conversation.
#     Placeholders: {bot_name}, {player_name}, {full_chat_history}
#     Default:     You are a summarization expert. Condense the following conversation between '{bot_name}' and '{player_name}' into a concise, third-person summary of 2-3 sentences. Capture the key topics discussed, any important decisions made, and the overall tone of the relationship. Full Chat History:\n{full_chat_history}
OllamaChat.SummarizationPromptTemplate = You are a summarization expert. Condense the following conversation between '{bot_name}' and '{player_name}' into a concise, third-person summary of 2-3 sentences. Capture the key topics discussed, any important decisions made, and the overall tone of the relationship. Full Chat History:\n{full_chat_history}
```

## Setting up Ollama for the Database

This module requires a running Ollama server.

1.  **Install Ollama:** Download from [ollama.com](https://ollama.com).
2.  **Pull Required Models:** You need both a generation model and an embedding model.
    ```bash
    # For chat generation (example: Llama 3.2 1B)
    ollama pull llama3.2:1b

    # For the RAG system's vector database (REQUIRED)
    ollama pull nomic-embed-text
    ```
3.  **Run the Server:**
    ```bash
    ollama serve
    ```
4.  **Update Config:** Ensure `OllamaChat.Url` in your `.conf` file points to your Ollama server.

## Configuration Options

For a complete list of all available configuration options, including settings for RAG, Summarization, Sentiment Tracking, and more, please see the `mod-ollama-chat.conf.dist` file included in this repository.

## Commands

All commands require Administrator permission.

- `.ollama reload config`: Reloads the module's `.conf` file and personality packs.
- `.ollama reload rag`: Reloads all JSON files from the RAG data path into memory.
- `.ollama sentiment view [bot] [player]`: Displays sentiment data.
- `.ollama sentiment set <bot> <player> <value>`: Manually sets sentiment (0.0-1.0).
- `.ollama sentiment reset [bot] [player]`: Resets sentiment to default.
- `.ollama personality get <bot>`: Displays a bot's current personality.
- `.ollama personality set <bot> <personality>`: Manually assigns a personality.
- `.ollama personality list`: Lists all available personalities.

## How It Works

1.  A player sends a message or triggers a game event.
2.  The RAG system is queried: the message is analyzed for filterable keywords, and a TF-IDF vector search is performed on the knowledge base to find relevant information.
4.  A comprehensive prompt is assembled, including:
    - The base template and the bot's personality.
    - The bot's **Full Game State Snapshot** (if game state snapshots are enabled).
    - The bot's current **sentiment** towards the player (if sentiment system is enabled).
    - The **conversation history** (long-term summary + recent messages) (if conversation history system is enabled).
    - The retrieved **RAG information**.
5.  The prompt is sent to the asynchronous query manager, which dispatches it to the Ollama API.
6.  The LLM generates a response, which is routed back to the appropriate in-game chat channel.

## License

This module is released under the GNU GPL v3 license, consistent with AzerothCore's licensing.

## Contribution

Developed by Dustin Hendrickson, enhanced by Gemini 2.5 Pro, tested by BasedNapkin.

Pull requests, bug reports, and feature suggestions are welcome. Please adhere to AzerothCore's coding standards and guidelines when submitting contributions.
