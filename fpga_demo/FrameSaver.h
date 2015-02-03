#ifndef FRAME_SAVER_PUBLIC
#define FRAME_SAVER_PUBLIC

#include "Public.h"
#include <string>

using std::string;

class FramePool;
class Frame;

//////////////////////////////////////////////////////////////////////////////////////////////////////////


class FrameSaver
{
public:
    FrameSaver(int nframes_to_save,
               bool save_video);
    ~FrameSaver(void);

    void SetBGPool(FramePool * bg_pool);
    void SetFGPool(FramePool * fg_pool);

    void stop(void);
    void start(void);

    void UpdateBGIndex(void);
    void UpdateFGIndex(void);


    int                  _nframes_to_save;
    bool                 _save_video;
    bool                 _end;
    std::vector<string>  _log_buffer;

    int                  _last_bg_frame_num;
    int                  _last_fg_frame_num;
    int                  _bg_index;
    int                  _fg_index;

    Frame *              _bg_buffer;
    Frame *              _fg_buffer;

    pthread_t            _frame_saver_thread;
    pthread_mutex_t      _frame_saver_mutex;

    FramePool *          _bg_pool;
    FramePool *          _fg_pool;
};


#endif // #define FRAME_SAVER_PUBLIC



