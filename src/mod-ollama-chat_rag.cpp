#include "mod-ollama-chat_rag.h"
#include "mod-ollama-chat_config.h"
#include "Log.h"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <unordered_map>
#include <cmath> // For std::log

#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

#define RAG_LOG "server.script"

OllamaRAGSystem::OllamaRAGSystem() : _isInitialized(false) {}

OllamaRAGSystem::~OllamaRAGSystem() {}

std::vector<std::string> OllamaRAGSystem::TokenizeString(const std::string& text) const
{
    std::stringstream ss(text);
    std::string token;
    std::vector<std::string> tokens;
    while (ss >> token)
    {
        std::transform(token.begin(), token.end(), token.begin(),
            [](unsigned char c){ return std::tolower(c); });
        token.erase(std::remove_if(token.begin(), token.end(),
            [](unsigned char c) { return std::ispunct(c); }), token.end());
        if (!token.empty())
        {
            tokens.push_back(token);
        }
    }
    return tokens;
}

bool OllamaRAGSystem::Initialize()
{
    LOG_INFO(RAG_LOG, "[Ollama RAG] Initializing RAG system (TF-IDF Vector Model)...{}", "");

    if (g_RAGDataPath.empty())
    {
        LOG_ERROR(RAG_LOG, "[Ollama RAG] RAG data path is not configured. System disabled.{}", "");
        return false;
    }
    if (!fs::exists(g_RAGDataPath) || !fs::is_directory(g_RAGDataPath))
    {
        LOG_ERROR(RAG_LOG, "[Ollama RAG] RAG data path '{}' does not exist or is not a directory.", g_RAGDataPath);
        return false;
    }

    _ragEntries.clear();
    int filesLoaded = 0;
    for (const auto& entry : fs::directory_iterator(g_RAGDataPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            if (LoadRAGEntriesFromFile(entry.path().string()))
            {
                filesLoaded++;
            }
        }
    }

    if (filesLoaded == 0 || _ragEntries.empty())
    {
        LOG_ERROR(RAG_LOG, "[Ollama RAG] No valid RAG JSON files were found or loaded from '{}'.", g_RAGDataPath);
        return false;
    }

    LOG_INFO(RAG_LOG, "[Ollama RAG] Successfully loaded {} RAG entries from {} JSON file(s).", static_cast<uint32_t>(_ragEntries.size()), filesLoaded);

    BuildTFIDFVectors();
    
    _isInitialized = true;
    return true;
}

void OllamaRAGSystem::BuildTFIDFVectors()
{
    LOG_INFO(RAG_LOG, "[Ollama RAG] Building TF-IDF vocabulary and vectors...{}", "");

    _vocabulary.clear();
    _idf_vector.clear();
    _document_vectors.clear();
    
    std::unordered_map<std::string, int> doc_frequencies;
    int vocab_idx = 0;

    // --- Pass 1: Build vocabulary and document frequencies ---
    for (const auto& entry : _ragEntries) {
        std::unordered_set<std::string> terms_in_doc;
        auto process_text = [&](const std::string& text) {
            for (const auto& token : TokenizeString(text)) {
                if (_vocabulary.find(token) == _vocabulary.end()) {
                    _vocabulary[token] = vocab_idx++;
                }
                terms_in_doc.insert(token);
            }
        };
        
        process_text(entry.title);
        process_text(entry.summary);
        process_text(entry.content);
        for(const auto& keyword : entry.keywords) process_text(keyword);
        for(const auto& q : entry.questions) process_text(q);

        for (const auto& term : terms_in_doc) {
            doc_frequencies[term]++;
        }
    }

    // --- Pass 2: Calculate IDF scores ---
    _idf_vector.resize(_vocabulary.size());
    size_t total_docs = _ragEntries.size();
    for (const auto& pair : _vocabulary) {
        _idf_vector[pair.second] = std::log((double)total_docs / (1.0 + doc_frequencies[pair.first]));
    }
    
    LOG_INFO(RAG_LOG, "[Ollama RAG] Vocabulary size: {}. IDF vector calculated.", static_cast<uint32_t>(_vocabulary.size()));

    // --- Pass 3: Create TF-IDF vectors for each document ---
    _document_vectors.resize(_ragEntries.size());
    for (size_t i = 0; i < _ragEntries.size(); ++i) {
        const auto& entry = _ragEntries[i];
        std::unordered_map<int, double> term_frequencies;
        int total_terms = 0;

        auto process_tf = [&](const std::string& text) {
            for (const auto& token : TokenizeString(text)) {
                if (_vocabulary.count(token)) {
                    term_frequencies[_vocabulary[token]]++;
                    total_terms++;
                }
            }
        };

        process_tf(entry.title);
        process_tf(entry.summary);
        process_tf(entry.content);
        for(const auto& keyword : entry.keywords) process_tf(keyword);
        for(const auto& q : entry.questions) process_tf(q);

        if (total_terms > 0) {
            Eigen::SparseVector<double> vec(_vocabulary.size());
            vec.reserve(term_frequencies.size());
            for (const auto& tf_pair : term_frequencies) {
                double tf = (double)tf_pair.second / total_terms;
                double tf_idf = tf * _idf_vector[tf_pair.first];
                vec.insert(tf_pair.first) = tf_idf;
            }

            // FIX for Eigen 5.0.0: .normalized() is removed for SparseVector.
            // Normalize by dividing by the vector's norm.
            double norm = vec.norm();
            if (norm > 0) {
                vec /= norm;
            }
            _document_vectors[i] = vec; // Pre-normalize for faster cosine similarity
        }
    }
    
    LOG_INFO(RAG_LOG, "[Ollama RAG] Pre-computed and normalized {} document vectors.", static_cast<uint32_t>(_document_vectors.size()));
}


bool OllamaRAGSystem::LoadRAGEntriesFromFile(const std::string& filePath)
{
    std::ifstream fileStream(filePath);
    if (!fileStream.is_open())
    {
        LOG_ERROR(RAG_LOG, "[Ollama RAG] Failed to open file: {}", filePath);
        return false;
    }

    try
    {
        nlohmann::json data = nlohmann::json::parse(fileStream);
        if (!data.is_array())
        {
            LOG_ERROR(RAG_LOG, "[Ollama RAG] JSON file is not an array: {}", filePath);
            return false;
        }

        for (const auto& item : data)
        {
            RAGEntry entry;
            item.at("id").get_to(entry.id);
            item.at("title").get_to(entry.title);
            item.at("content").get_to(entry.content);
            item.at("keywords").get_to(entry.keywords);
            item.at("tags").get_to(entry.tags);

            entry.parentId = item.value("parentId", "");
            entry.summary = item.value("summary", "");
            entry.entityType = item.value("entityType", "");
            
            if (item.contains("questions")) item["questions"].get_to(entry.questions);
            if (item.contains("metadata") && item["metadata"].is_array()) {
                for (const auto& metaItem : item["metadata"]) {
                    entry.metadata.push_back({ metaItem.value("key", ""), metaItem.value("value", "") });
                }
            }
            if (item.contains("relations") && item["relations"].is_array()) {
                for (const auto& relItem : item["relations"]) {
                    entry.relations.push_back({ relItem.value("id", ""), relItem.value("type", ""), relItem.value("relationship", "") });
                }
            }
            if (item.contains("source") && item["source"].is_object()) {
                entry.source = { item["source"].value("url", ""), item["source"].value("commentId", "") };
            }

            _ragEntries.push_back(entry);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(RAG_LOG, "[Ollama RAG] Exception while parsing '{}': {}", filePath, e.what());
        return false;
    }

    return true;
}

// Helper function to check if an entry's metadata matches all provided filters.
bool OllamaRAGSystem::DoesEntryMatchFilters(const RAGEntry& entry, const RAGFilterMap& filters) const
{
    for (const auto& filter : filters)
    {
        bool matchFound = false;
        for (const auto& meta : entry.metadata)
        {
            if (meta.key == filter.first && meta.value == filter.second)
            {
                matchFound = true;
                break;
            }
        }
        if (!matchFound)
        {
            return false;
        }
    }
    return true;
}

std::vector<RAGResult> OllamaRAGSystem::RetrieveRelevantInfo(const std::string& query, const RAGFilterMap& filters, int maxResults, float similarityThreshold)
{
    if (!_isInitialized) return {};

    LOG_DEBUG(RAG_LOG, "[Ollama RAG] Received query: '{}'", query);

    // --- STAGE 1: HYBRID SEARCH - METADATA FILTERING ---
    std::vector<size_t> candidate_indices;
    if (!filters.empty()) {
        for (size_t i = 0; i < _ragEntries.size(); ++i) {
            if (DoesEntryMatchFilters(_ragEntries[i], filters)) {
                candidate_indices.push_back(i);
            }
        }
        LOG_DEBUG(RAG_LOG, "[Ollama RAG] {} entries remained after filtering.", static_cast<uint32_t>(candidate_indices.size()));
    } else {
        // If no filters, all documents are candidates
        candidate_indices.resize(_ragEntries.size());
        for(size_t i = 0; i < _ragEntries.size(); ++i) candidate_indices[i] = i;
    }

    if (candidate_indices.empty()) {
        return {};
    }

    // --- STAGE 2: CREATE QUERY VECTOR ---
    std::unordered_map<int, double> query_tf;
    int total_query_terms = 0;
    for (const auto& token : TokenizeString(query)) {
        if (_vocabulary.count(token)) {
            query_tf[_vocabulary[token]]++;
            total_query_terms++;
        }
    }

    if (total_query_terms == 0) return {};

    Eigen::SparseVector<double> query_vector(_vocabulary.size());
    query_vector.reserve(query_tf.size());

    for (const auto& tf_pair : query_tf) {
        double tf = (double)tf_pair.second / total_query_terms;
        double tf_idf = tf * _idf_vector[tf_pair.first];
        query_vector.insert(tf_pair.first) = tf_idf;
    }

    // FIX for Eigen 5.0.0: .normalize() is removed for SparseVector.
    // Normalize by dividing by the vector's norm.
    double norm = query_vector.norm();
    if (norm > 0) {
        query_vector /= norm;
    }
    
    // --- STAGE 3: CALCULATE COSINE SIMILARITY FOR CANDIDATES ---
    std::vector<RAGResult> results;
    results.reserve(candidate_indices.size());

    for (size_t index : candidate_indices) {
        // Cosine similarity is just the dot product of normalized vectors
        double similarity = _document_vectors[index].dot(query_vector);
        if (similarity >= similarityThreshold) {
            results.push_back({&_ragEntries[index], (float)similarity});
        }
    }
    
    // --- STAGE 4: SORT AND LIMIT ---
    std::sort(results.rbegin(), results.rend());
    if (results.size() > maxResults)
    {
        results.resize(maxResults);
    }

    if (g_DebugEnabled)
    {
        std::stringstream debug_ss;
        debug_ss << "[Ollama RAG] Top " << results.size() << " results for query '" << query << "':\\n";
        for (const auto& result : results)
        {
            debug_ss << "  - ID: " << result.entry->id << ", Score: " << result.similarity << "\\n";
        }
        LOG_DEBUG(RAG_LOG, "{}", debug_ss.str());
    }

    return results;
}

std::string OllamaRAGSystem::GetFormattedRAGInfo(const std::vector<RAGResult>& results)
{
    if (results.empty()) return "";

    std::stringstream ss;
    ss << "### Relevant Information Found ###\\n";
    
    for (const auto& result : results)
    {
        const RAGEntry* entry = result.entry;
        ss << "\\n--- Entry: " << entry->title << " (Score: " << result.similarity << ") ---\\n";
        if (!entry->summary.empty())
        {
            ss << "**Summary:** " << entry->summary << "\\n";
        }
        ss << "**Content:** " << entry->content << "\\n";

        if (!entry->metadata.empty()) {
            ss << "**Metadata:**\\n";
            for (const auto& meta : entry->metadata) {
                ss << "- " << meta.key << ": " << meta.value << "\\n";
            }
        }
        if (!entry->relations.empty()) {
            ss << "**Related Entities:**\\n";
            for (const auto& rel : entry->relations) {
                ss << "- " << rel.id << " (" << rel.type << "): " << rel.relationship << "\\n";
            }
        }
    }
    return ss.str();
}
