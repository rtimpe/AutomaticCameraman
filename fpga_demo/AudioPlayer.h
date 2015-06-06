#ifndef AUDIO_PLAYER_PUBLIC
#define AUDIO_PLAYER_PUBLIC

#include "Public.h"
#include <string>

using std::string;

class WavHandle;

////////////////////////////////////////////////////////////////////////////////

class AudioPlayer
{
public:
    AudioPlayer(void);
    virtual ~AudioPlayer(void);

    WavHandle * loadWav(const string & file_path) const;
    void playWav(WavHandle * wh);
};


#endif // #define AUDIO_PLAYER_PUBLIC



