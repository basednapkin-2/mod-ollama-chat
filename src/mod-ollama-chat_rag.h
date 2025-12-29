#ifndef MOD_OLLAMA_CHAT_RAG_H
#define MOD_OLLAMA_CHAT_RAG_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

// Include Eigen for sparse vector operations.
// Use the main <Eigen/Sparse> header for compatibility and completeness.
#include <Eigen/Sparse>

// Represents a key-value pair for the 'metadata' field.
struct RAGMetadata {
    std::string key;
    std::string value;
};

// Represents a link to another entity in the knowledge graph.
struct RAGRelation {
    std::string id;
    std::string type;
    std::string relationship;
};

// Represents the source of the information.
struct RAGSource {
    std::string url;
    std::string commentId; // Optional
};

// Represents a single entry in our RAG knowledge base.
struct RAGEntry {
    // Basic Schema Fields
    std::string id;
    std::string title;
    std::string content;
    std::vector<std::string> keywords;
    std::vector<std::string> tags;
    // Advanced Schema Fields
    std::string parentId;
    std::string summary;
    std::string entityType;
    std::vector<std::string> questions;
    std::vector<RAGMetadata> metadata;
    std::vector<RAGRelation> relations;
    RAGSource source;
};

// Represents a query result with its similarity score.
struct RAGResult {
    const RAGEntry* entry;
    float similarity;

    bool operator<(const RAGResult& other) const {
        return similarity < other.similarity;
    }
};

class Player; // Forward declaration

class OllamaRAGSystem
{
public:
    OllamaRAGSystem();
    ~OllamaRAGSystem();

    bool Initialize();

    // Type alias for the new filter map.
    using RAGFilterMap = std::unordered_map<std::string, std::string>;
    
    // UPDATED: Now accepts a filter map for hybrid search.
    std::vector<RAGResult> RetrieveRelevantInfo(const std::string& query, const RAGFilterMap& filters, int maxResults, float similarityThreshold);
    
    std::string GetFormattedRAGInfo(const std::vector<RAGResult>& results);

private:
    bool LoadRAGEntriesFromFile(const std::string& filePath);
    std::vector<std::string> TokenizeString(const std::string& text) const;
    bool DoesEntryMatchFilters(const RAGEntry& entry, const RAGFilterMap& filters) const;
    void BuildTFIDFVectors();

    std::vector<RAGEntry> _ragEntries;
    bool _isInitialized;

    // --- TF-IDF Data Structures ---
    std::unordered_map<std::string, int> _vocabulary;
    std::vector<double> _idf_vector;
    std::vector<Eigen::SparseVector<double>> _document_vectors;
};

#endif // MOD_OLLAMA_CHAT_RAG_H