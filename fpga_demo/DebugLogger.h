#ifndef DEBUG_LOGGER_PUBLIC
#define DEBUG_LOGGER_PUBLIC

#include "Public.h"
#include <string>

using std::string;

class FramePool;
class Frame;

//////////////////////////////////////////////////////////////////////////////////////////////////////////


class DebugLogger
{
public:
    DebugLogger(int nframes_to_save);
    ~DebugLogger();

    void SetBGPool(FramePool * bg_pool);
    void SetFGPool(FramePool * fg_pool);

    void stop(void);
    void start(void);

    void log(string s);
    void UpdateBGIndex(void);
    void UpdateFGIndex(void);


    int                  _nframes_to_save;
    bool                 _end;
    std::vector<string>  _log_buffer;

    int                  _last_bg_frame_num;
    int                  _last_fg_frame_num;
    int                  _bg_index;
    int                  _fg_index;

    Frame *              _bg_buffer;
    Frame *              _fg_buffer;

    pthread_t            _logger_thread;
    pthread_mutex_t      _logger_mutex;
    pthread_t            _buffer_thread;
    pthread_mutex_t      _buffer_mutex;

    FramePool *          _bg_pool;
    FramePool *          _fg_pool;
};


#endif // #define DEBUG_LOGGER_PUBLIC



