-- This table stores condensed summaries of long conversations between bots and players.
-- It's used by the Conversation Summarization feature to maintain long-term memory
-- while minimizing token usage in prompts.

CREATE TABLE IF NOT EXISTS `mod_ollama_chat_summaries` (
  `bot_guid` BIGINT UNSIGNED NOT NULL,
  `player_guid` BIGINT UNSIGNED NOT NULL,
  `summary_text` TEXT NOT NULL,
  `last_updated` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`bot_guid`, `player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Stores AI-generated summaries of bot-player conversations.';
