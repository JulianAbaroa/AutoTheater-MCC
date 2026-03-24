#pragma once

#include <condition_variable>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <queue>

class GallerySystem {
public:
    GallerySystem(); 
    ~GallerySystem(); 

    void RefreshList(const std::string& path);
    void LoadMetadataAsync(int videoIndex);
    void DeleteVideo(int videoIndex);
    void RenameVideo(int videoIndex, const std::string& newName);
    std::string FormatBytes(uint64_t bytes);
    size_t GetPendingCount();
    std::string FormatDuration(float duration);

private:
    void WorkerLoop();
    void ProcessVideoMetadata(int videoIndex);
    void ClearQueue();

    std::thread m_WorkerThread;
    std::queue<int> m_PendingMetadata;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    std::atomic<bool> m_ShuttingDown{ false };

    std::string ExecuteSilent(const std::string& command);
    std::vector<unsigned char> ExecuteSilentBinary(const std::string& command);
};