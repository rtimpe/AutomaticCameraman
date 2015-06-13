/*
 * DeckLinkCaptureDelegate.h
 *
 *  Created on: Jun 12, 2015
 *      Author: robert
 */

#ifndef DECKLINKCAPTUREDELEGATE_H_
#define DECKLINKCAPTUREDELEGATE_H_

#include "DeckLinkAPI.h"
#include "FramePool.h"
#include <pthread.h>

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
public:
    DeckLinkCaptureDelegate(FramePool *videoPool, int width, int height);
    ~DeckLinkCaptureDelegate();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE  Release(void);
    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);


private:
    ULONG               m_refCount;
    pthread_mutex_t     m_mutex;
    FramePool *videoPool;
    Frame *frame;
    long frameNum;
    int height;
    int width;
};


#endif /* DECKLINKCAPTUREDELEGATE_H_ */
