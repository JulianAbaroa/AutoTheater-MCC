#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/PipeSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Threads/Infrastructure/WriterThread.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"

void WriterThread::Run()
{
	while (g_pState->Infrastructure->Lifecycle->IsRunning())
	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		m_CV.wait(lock, [&] { return !m_Running.load() || !m_Queue.empty(); });

		if (!m_Running.load()) break;
		if (m_Queue.empty()) continue;

		Item item = std::move(m_Queue.front());
		m_Queue.pop_front();
		m_IsProcessing.store(true);
		lock.unlock();

		bool writeSuccess = false;

		if (item.Type == ItemType::Video)
		{
			writeSuccess = g_pSystem->Infrastructure->Pipe->WriteVideo(
				item.Data.data(), item.Data.size());

			if (item.ReturnVideoToPool)
			{
				g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(item.Data));
			}
		}
		else
		{
			writeSuccess = g_pSystem->Infrastructure->Pipe->WriteAudio(
				item.Data.data(), item.Data.size());
		}

		if (!writeSuccess || g_pState->Infrastructure->Process->HasFatalError())
		{
			std::unique_lock<std::mutex> drainLock(m_Mutex);
			for (auto& queuedItem : m_Queue)
			{
				if (queuedItem.Type == ItemType::Video && queuedItem.ReturnVideoToPool)
				{
					g_pSystem->Infrastructure->Video->ReturnBuffer(
						std::move(queuedItem.Data));
				}
			}

			m_Queue.clear();
			m_IsProcessing.store(false);
			m_CV_Flush.notify_all();
			drainLock.unlock();

			g_pThread->Capture->StopRecording(true);
			continue;
		}

		lock.lock();
		m_IsProcessing.store(false);
		if (m_Queue.empty())
		{
			m_CV_Flush.notify_all();
		}
	}
}


void WriterThread::StartRecording()
{
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Queue.clear();
	}

	m_IsProcessing.store(false);
	m_Recording.store(true);
}

void WriterThread::StopRecording(bool force)
{
	m_Recording.store(false);

	if (force)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Queue.clear();
		m_CV_Flush.notify_all();
	}
}


void WriterThread::EnqueueVideo(std::vector<uint8_t>&& buffer, bool returnToPool)
{
	if (!m_Recording.load()) return;

	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Queue.push_back({
			ItemType::Video,
			std::move(buffer),
			returnToPool
			});
	}

	m_CV.notify_one();
}

void WriterThread::EnqueueAudio(std::vector<uint8_t>&& buffer)
{
	if (!m_Recording.load()) return;

	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Queue.push_back({
			ItemType::Audio,
			std::move(buffer),
			false
			});
	}

	m_CV.notify_one();
}

void WriterThread::Flush()
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	m_CV_Flush.wait(lock, [this] {
		return (!m_Recording.load()) || (m_Queue.empty() && !m_IsProcessing.load());
	});
}


void WriterThread::DropOldestVideo()
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	for (auto it = m_Queue.begin(); it != m_Queue.end(); ++it)
	{
		if (it->Type == ItemType::Video)
		{
			g_pSystem->Infrastructure->Video->ReturnBuffer(
				std::move(it->Data));

			m_Queue.erase(it);
			break;
		}
	}
}