#include "cautil.h"
#include <CoreFoundation/CoreFoundation.h>

static OSStatus render_callback(void *data, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
	struct Player *player = data;
	struct PlayerEvent evt = {
		.player = player,
		.type = PLAYER_RENDER
	};
	if(ioData->mBuffers[0].mData && ioData->mBuffers[1].mData) {
		evt.left = ioData->mBuffers[0].mData;
		evt.right = ioData->mBuffers[1].mData;
		evt.size = ioData->mBuffers[0].mDataByteSize / sizeof(float);
		player->handleEvent(&evt);
	}
	if(inTimeStamp->mSampleTime >= player->samples && player->playing) {
		player->playing = 0;
		evt.player = player;
		evt.type = PLAYER_DONE;
		player->handleEvent(&evt);
	}
	return 0;
}

OSStatus CAInitPlayer(struct Player *player, PlayerEventHandler eventHandler) {
	OSStatus res = -1;
	if(!eventHandler)
		goto done;
	player->handleEvent = eventHandler;
	if(NewAUGraph(&player->graph))
		goto done;
	AudioComponentDescription cd = {
		.componentType = kAudioUnitType_Output,
		.componentSubType = kAudioUnitSubType_DefaultOutput,
		.componentManufacturer = kAudioUnitManufacturer_Apple
	};
	if(AUGraphAddNode(player->graph, &cd, &player->outputNode))
		goto close_graph;
	cd.componentType = kAudioUnitType_Generator;
	cd.componentSubType = kAudioUnitSubType_AudioFilePlayer;
	if(AUGraphAddNode(player->graph, &cd, &player->fileNode))
		goto close_graph;
	if(AUGraphOpen(player->graph))
		goto close_graph;
	if(AUGraphNodeInfo(player->graph, player->fileNode, NULL, &player->fileAU))
		goto close_graph;
	AudioStreamBasicDescription outputFormat;
	UInt32 propsize = sizeof(AudioStreamBasicDescription);
	if(AudioUnitGetProperty(player->fileAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outputFormat, &propsize))
		goto close_graph;
	outputFormat.mFormatFlags = (outputFormat.mFormatFlags & ~(kAudioFormatFlagIsSignedInteger)) | kLinearPCMFormatFlagIsFloat;
	outputFormat.mSampleRate = 96000.;
	outputFormat.mChannelsPerFrame = 2;
	if(AudioUnitSetProperty(player->fileAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outputFormat, sizeof(outputFormat)))
		goto close_graph;
	if(AUGraphConnectNodeInput(player->graph, player->fileNode, 0, player->outputNode, 0))
		goto close_graph;
	if(AudioUnitAddRenderNotify(player->fileAU, render_callback, player))
		goto close_graph;
	player->playing = 0;
	return 0;
close_graph:
	AUGraphClose(player->graph);
done:
	return -1;
}

OSStatus CAPlayFile(struct Player *player, const char *path) {
	AudioFileID audioFile;
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)path, strlen(path), false);
	if(!url)
		goto done;
	if(AudioFileOpenURL(url, kAudioFileReadPermission, 0, &player->audioFile)) {
		CFRelease(url);
		goto done;
	}
	CFRelease(url);
	AudioStreamBasicDescription fileFormat;
	UInt32 propsize = sizeof(AudioStreamBasicDescription);
	if(AudioFileGetProperty(player->audioFile, kAudioFilePropertyDataFormat, &propsize, &fileFormat) || fileFormat.mChannelsPerFrame > 2)
		goto close_file;
	if(AudioUnitSetProperty(player->fileAU, kAudioUnitProperty_ScheduledFileIDs, kAudioUnitScope_Global, 0, &player->audioFile, sizeof(player->audioFile)))
		goto close_file;
	if(AUGraphInitialize(player->graph))
		goto close_file;
	usleep(10000);
	UInt64 packets;
	propsize = sizeof(packets);
	if(AudioFileGetProperty(player->audioFile, kAudioFilePropertyAudioDataPacketCount, &propsize, &packets))
		goto uninit_graph;
	player->samples = packets * fileFormat.mFramesPerPacket * 96000/(fileFormat.mSampleRate);
	ScheduledAudioFileRegion rgn = {
		.mTimeStamp = {
			.mFlags = kAudioTimeStampSampleTimeValid,
			.mSampleTime = 0
		},
		.mCompletionProc = NULL,
		.mAudioFile = player->audioFile,
		.mLoopCount = 0,
		.mStartFrame = 0,
		.mFramesToPlay = (UInt32)(packets * fileFormat.mFramesPerPacket)
	};
	if(AudioUnitSetProperty(player->fileAU, kAudioUnitProperty_ScheduledFileRegion, kAudioUnitScope_Global, 0, &rgn, sizeof(rgn))) {
		fprintf(stderr, "Failed to schedule file for playback\n");
		goto uninit_graph;
	}
	UInt32 defaultVal = 0;
	if(AudioUnitSetProperty(player->fileAU, kAudioUnitProperty_ScheduledFilePrime, kAudioUnitScope_Global, 0, &defaultVal, sizeof(defaultVal))) {
		fprintf(stderr, "Failed to schedule file for playback\n");
		goto uninit_graph;
	}
	AudioTimeStamp startTime = {
		.mFlags = kAudioTimeStampSampleTimeValid,
		.mSampleTime = -1
	};
	if(AudioUnitSetProperty(player->fileAU, kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global, 0, &startTime, sizeof(startTime))) {
		fprintf(stderr, "Failed to schedule file for playback\n");
		goto uninit_graph;
	}
	player->playing = 1;
	if(AUGraphStart(player->graph)) {
		player->playing = 0;
		fprintf(stderr, "Failed to start playback\n");
		goto uninit_graph;
	}
	return 0;
uninit_graph:
	AUGraphUninitialize(player->graph);
close_file:
	AudioFileClose(player->audioFile);
done:
	return -1;
}

void CAStopPlayback(struct Player *player) {
	AUGraphStop(player->graph);
	AUGraphUninitialize(player->graph);
	AudioFileClose(player->audioFile);
}

void CADestroyPlayer(struct Player *player) {
	AUGraphClose(player->graph);
}
