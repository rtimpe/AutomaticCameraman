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
    DebugLogger(void);
    ~DebugLogger(void);

    void stop(void);
    void start(void);

    void log(string s);

    bool                 _end;
    std::vector<string>  _log_buffer;

    Frame *              _bg_buffer;
    Frame *              _fg_buffer;

    pthread_t            _logger_thread;
    pthread_mutex_t      _logger_mutex;
};


#endif // #define DEBUG_LOGGER_PUBLIC



