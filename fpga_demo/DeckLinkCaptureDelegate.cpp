/*
 * DeckLinkCaptureDelegate.cpp
 *
 *  Created on: Jun 12, 2015
 *      Author: robert
 */

#include "DeckLinkCaptureDelegate.h"
#include <cstdio>

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate(FramePool *videoPool, int width, int height) : m_refCount(0),
    videoPool(videoPool),
    frame(NULL),
    frameNum(0),
    height(height),
    width(width)
{
    pthread_mutex_init(&m_mutex, NULL);

    // Initialize the FramePool
    IplImage *bgr;
    char * frameBuf;

    for (int i = 0; i < videoPool->_size; i++) {
        if ((frameBuf = (char *)malloc(width*height*3)) == NULL) {
            printf("ERROR: Cannot allocate image buffer space.\n");
        }
        bgr = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 3);
        bgr->imageData = frameBuf;
        videoPool->add_frame(bgr);
    }
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
    pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
    pthread_mutex_lock(&m_mutex);
        m_refCount++;
    pthread_mutex_unlock(&m_mutex);

    return (ULONG)m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
    pthread_mutex_lock(&m_mutex);
        m_refCount--;
    pthread_mutex_unlock(&m_mutex);

    if (m_refCount == 0)
    {
        delete this;
        return 0;
    }

    return (ULONG)m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
    void* frameBytes;

    // Handle Video Frame
    if (videoFrame)
    {
        if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
        {
            //printf("Frame received - No input signal detected\n");
        }
        else
        {
            //printf("Frame received - Valid Frame - Size: %li bytes\n",
            //    videoFrame->GetRowBytes() * videoFrame->GetHeight());

            frame = videoPool->update_next(frame, frameNum++, false);


            videoFrame->GetBytes(&frameBytes);

            IplImage * bgr = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 4);
            cvSetData(bgr, frameBytes, videoFrame->GetRowBytes());
            cvCvtColor(bgr, frame->_bgr, CV_BGRA2BGR);

            // need to flip image if camera is upside down
            cvFlip(frame->_bgr, NULL, 0);
        }

    }

    return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags)
{
    // This only gets called if bmdVideoInputEnableFormatDetection was set
    // when enabling video input
//    HRESULT result;
//    char*   displayModeName = NULL;
//
//    if (!(events & bmdVideoInputDisplayModeChanged))
//        return S_OK;
//
//    mode->GetName((const char**)&displayModeName);
//    printf("Video format changed to %s\n", displayModeName);
//
//    if (displayModeName)
//        free(displayModeName);
//
//    if (g_deckLinkInput)
//    {
//        g_deckLinkInput->StopStreams();
//
//        result = g_deckLinkInput->EnableVideoInput(mode->GetDisplayMode(), g_config.m_pixelFormat, g_config.m_inputFlags);
//        if (result != S_OK)
//        {
//            fprintf(stderr, "Failed to switch video mode\n");
//            goto bail;
//        }
//
//        g_deckLinkInput->StartStreams();
//    }
//
//bail:
    return S_OK;
}


