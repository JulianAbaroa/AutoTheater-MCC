#pragma once

enum class SyncDecision {
    None,
    WriteAudio,
    WriteVideo,
    RepeatVideo, 
    DropVideo   
};