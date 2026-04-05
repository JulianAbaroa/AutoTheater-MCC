#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/SyncSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void VideoSystem::StartRecording()
{
    auto configuration = g_pState->Infrastructure->FFmpeg->GetConfiguration();
    g_pState->Infrastructure->Video->SetMaxFrames(configuration.MaxBufferedFrames);

    int width = g_pState->Infrastructure->Render->GetWidth();
    int height = g_pState->Infrastructure->Render->GetHeight();
	size_t frameByteSize = static_cast<size_t>(width) * height * 4;
	g_pState->Infrastructure->Video->SetFrameByteSize(frameByteSize);

	g_pState->Infrastructure->Video->SetRecording(true);
    g_pSystem->Debug->Log("[VideoSystem] INFO: Video recording started.");
}

void VideoSystem::StopRecording()
{
    g_pState->Infrastructure->Video->Reset();
	g_pState->Infrastructure->Video->SetRecording(false);
    g_pSystem->Debug->Log("[VideoSystem] INFO: Video recording stopped.");
}


std::deque<FrameData> VideoSystem::ExtractQueue()
{
	std::deque<FrameData> localQueue;
	g_pState->Infrastructure->Video->SwapFrameQueue(localQueue);
	return localQueue;
}

void VideoSystem::ClearQueue()
{
    std::deque<FrameData> framesToClear = 
        g_pState->Infrastructure->Video->DiscardAndTakeQueue();

    while (!framesToClear.empty())
    {
        this->ReturnBuffer(std::move(framesToClear.front().Buffer));
        framesToClear.pop_front();
    }

    g_pSystem->Debug->Log("[VideoSystem] INFO: Video queue cleared.");
}


void VideoSystem::PreallocatePool(UINT width, UINT height)
{
	size_t newBufferSize = static_cast<size_t>(width) * height * 4;
	auto configuration = g_pState->Infrastructure->FFmpeg->GetConfiguration();
    int maxFrames = configuration.MaxBufferedFrames + 1;

	if (g_pState->Infrastructure->Video->IsPoolValid(newBufferSize))
    {
        g_pSystem->Debug->Log("[VideoSystem] INFO: Pool already allocated"
            " with correct size, skipping.");

        return;
    }

	std::deque<std::vector<uint8_t>> initialBuffers;
    for (int i = 0; i < 8; i++)
    {
        initialBuffers.emplace_back(newBufferSize, 0);
    }

	g_pState->Infrastructure->Video->InitializePool(
        newBufferSize, maxFrames, std::move(initialBuffers));

    g_pSystem->Debug->Log("[VideoSystem] INFO: Pool initialized with 8 frames."
        " Will grow up to %d. FrameSize: %zu MB", maxFrames,
        newBufferSize / (1024ULL * 1024));
}

void VideoSystem::ReturnBuffer(std::vector<uint8_t>&& buffer)
{
    if (buffer.size() != g_pState->Infrastructure->Video->GetFrameByteSize())
    {
        g_pSystem->Debug->Log("[VideoSystem] WARNING: ReturnBuffer mismatch."
            " Got=%zu Expected=%zu. Buffer discarded.",
            buffer.size(), g_pState->Infrastructure->Video->GetFrameByteSize());
    }

	g_pState->Infrastructure->Video->PushFreeBuffer(std::move(buffer));
}

std::vector<uint8_t> VideoSystem::GetFreeBuffer()
{
    std::vector<uint8_t> buffer = g_pState->Infrastructure->Video->PopFreeBuffer();
    if (!buffer.empty()) return buffer;

    if (g_pState->Infrastructure->Video->GetTotalAllocatedBuffers() >= 
        (size_t)g_pState->Infrastructure->Video->GetMaxPoolSize())
    {
        return {};
    }

    size_t currentSize = g_pState->Infrastructure->Video->GetFrameByteSize();
    if (currentSize > 0)
    {
        g_pSystem->Debug->Log("[VideoSystem] INFO: Pool growing lazily.");
        return std::vector<uint8_t>(currentSize, 0);
    }

    return {};
}


void VideoSystem::PushFrame(const uint8_t* pData, UINT width, UINT height, UINT rowPitch, double engineTime)
{
    if (!g_pState->Infrastructure->Video->IsRecording()) return;

    std::vector<uint8_t> bufferToUse = this->GetFreeBuffer();

    if (bufferToUse.empty())
    {
        g_pSystem->Debug->Log("[VideoSystem] ERROR: Could not get a free buffer.");
        return;
    }

	UINT evenWidth = width & ~1;
	UINT evenHeight = height & ~1;
	size_t targetSize = g_pState->Infrastructure->Video->GetFrameByteSize();
    size_t targetStride = static_cast<size_t>(evenWidth) * 4;

	if (bufferToUse.size() != targetSize)
    {
        bufferToUse.resize(targetSize);
    }

	if (rowPitch == targetStride)
    {
        memcpy(bufferToUse.data(), pData, targetSize);
    }
    else
    {
        for (UINT i = 0; i < evenHeight; ++i) 
        {
            memcpy(bufferToUse.data() + (i * targetStride),
                pData + (i * rowPitch),
                targetStride);
        }
    }

    FrameData frame;
    frame.RealTime = engineTime;
	frame.Buffer = std::move(bufferToUse);

	if (!g_pState->Infrastructure->Video->TryPushFrame(std::move(frame)))
    {
        g_pSystem->Debug->Log("[VideoSystem] WARNING: Frame dropped! Queue full.");
    }
}


void VideoSystem::Cleanup()
{
    g_pState->Infrastructure->Video->Reset();
    g_pState->Infrastructure->Video->Cleanup();
    g_pSystem->Debug->Log("[VideoSystem] INFO: Cleanup completed.");
}