-- This SQL script creates the necessary table for the dynamic RAG filter system.
-- It should be executed on your 'acore_characters' database.
-- AzerothCore's module system will automatically apply SQL files
-- placed in the module's `data/sql/characters/updates` directory, but
-- you may need to run this manually if you're updating an existing setup.

CREATE TABLE IF NOT EXISTS `mod_ollama_chat_rag_filters` (
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `keyword` VARCHAR(255) NOT NULL COMMENT 'The player-facing search term (e.g., "frost resist").',
  `metadata_key` VARCHAR(255) NOT NULL COMMENT 'The metadata key this keyword maps to (e.g., "resistance").',
  `metadata_value` VARCHAR(255) NOT NULL COMMENT 'The metadata value this keyword maps to (e.g., "Frost").',
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_keyword` (`keyword`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Stores dynamic keyword-to-metadata mappings for the RAG hybrid search system.';

