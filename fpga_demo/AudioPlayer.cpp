#include "AudioPlayer.h"
#include "SDL.h"
#include "SDL_mixer.h"

class WavHandle
{
public:
    WavHandle
    (
        Mix_Chunk * mc
    ):
    _mix_chunk(mc)
    {}


    Mix_Chunk *  
    getMixChunk
    (
        void
    ) 
    {
        return _mix_chunk;
    }

private:
    Mix_Chunk * _mix_chunk;
};

AudioPlayer::AudioPlayer
(
    void
)
{
    // start SDL with audio support
    if(SDL_Init(SDL_INIT_AUDIO)==-1) 
    {
        printf("SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    // open 44.1KHz, signed 16bit, system byte order,
    //      stereo audio, using 1024 byte chunks
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1) 
    {
       printf("Mix_OpenAudio: %s\n", Mix_GetError());
       exit(2);
    }

}

AudioPlayer::~AudioPlayer
(
    void
)
{
    // Close audio
    Mix_CloseAudio();
}

WavHandle * 
AudioPlayer::loadWav
(
    const string & file_path
) const
{
    // load wav 
    Mix_Chunk * mc = Mix_LoadWAV(file_path.c_str());
    if(!mc) 
    {
        printf("Mix_LoadWAV: %s\n", Mix_GetError());
        return 0; 
    }
    else
    {
        return new WavHandle(mc);
    }
}

void 
AudioPlayer::playWav
(
    WavHandle * wh
)
{
    // play wav on first free unreserved channel
    // play it exactly once through
    if(-1 == Mix_PlayChannel(-1, wh->getMixChunk(), 0)) 
    {
        printf("Mix_PlayChannel: %s\n", Mix_GetError());
        // may be critical error, or maybe just no channels were free.
        // you could allocated another channel in that case...
    }
}

