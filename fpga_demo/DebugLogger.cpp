#include "DebugLogger.h"
#include "FramePool.h"
#include <pthread.h>
#include <sys/stat.h>
#include <sstream>

using std::cout;
using std::endl;
using std::stringstream;

static void * logger_function(void * args);
static void * buffer_function(void * args);
static char LOG_FOLDER_NAME[] = "logs";
static char IMAGES_FOLDER_PREFIX[] = "images";

static string
get_current_time_string()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
    std::string str(buffer);

    return str;
}


DebugLogger::DebugLogger
(
    int nframes_to_save
)
: _nframes_to_save(nframes_to_save), _end(false),
  _log_buffer(0),
  _last_bg_frame_num(0), _last_fg_frame_num(0),
   _bg_index(0), _fg_index(0), _bg_buffer(0), _fg_buffer(0)
{
    _bg_buffer = new Frame[_nframes_to_save];
    _fg_buffer = new Frame[_nframes_to_save];

    memset(_bg_buffer, 0, sizeof(Frame)*_nframes_to_save);
    memset(_fg_buffer, 0, sizeof(Frame)*_nframes_to_save);
}


DebugLogger::~DebugLogger
(
    void
)
{
    delete[] _bg_buffer;
    delete[] _fg_buffer;
}


void
DebugLogger::SetBGPool
(
    FramePool * bg_pool
)
{
    _bg_pool = bg_pool;
}


void
DebugLogger::SetFGPool
(
    FramePool * fg_pool
)
{
    _fg_pool = fg_pool;
}


void
DebugLogger::log
(
    string s
)
{
    stringstream ss;
    ss << get_current_time_string() << " : " << s;

    pthread_mutex_lock(&_logger_mutex);

    _log_buffer.push_back(ss.str());

    pthread_mutex_unlock(&_logger_mutex);
}


void
DebugLogger::start
(
    void
)
{
    _end = false;
    pthread_create(&_logger_thread, NULL, logger_function, this);
    pthread_create(&_buffer_thread, NULL, buffer_function, this);
}


void
DebugLogger::stop
(
    void
)
{
    _end = true;
    pthread_join(_logger_thread, NULL);
    pthread_join(_buffer_thread, NULL);
}


void
DebugLogger::UpdateBGIndex
(
    void
)
{
    _bg_index = (_bg_index + 1) % _nframes_to_save;
}


void
DebugLogger::UpdateFGIndex
(
    void
)
{
    _fg_index = (_fg_index + 1) % _nframes_to_save;
}


static void *
logger_function
(
    void * args
)
{
    DebugLogger * dl = reinterpret_cast<DebugLogger *>(args);

    //----------------------------------------------------------------
    // Create directory for logs if we want to save them
    if (-1 == mkdir(LOG_FOLDER_NAME, S_IRWXU | S_IRWXG|  S_IRWXO)) {
        perror("Error creating logs directory");
    }

    //-----------------------------------------------------------------
    // Create log file
    stringstream ss;
    ss << LOG_FOLDER_NAME << "/log_" << get_current_time_string();

    ofstream log_file;
    log_file.open(ss.str().c_str());

    //-----------------------------------------------------------------
    // Loop until the program ends
    while (!dl->_end)
    {
        pthread_mutex_lock(&dl->_logger_mutex);
        if (!dl->_log_buffer.empty())
        {
            vector<string>::iterator end = dl->_log_buffer.end();
            for (vector<string>::iterator it = dl->_log_buffer.begin();
                 it != end; ++it)
            {
                log_file << *it;
                cout << *it;
            }
            dl->_log_buffer.clear();
        }
        pthread_mutex_unlock(&dl->_logger_mutex);

        // Sleep 100 milliseconds
        usleep(100000);
    }

    //-----------------------------------------------------------------
    // Flush remaining log messages to disk
    pthread_mutex_lock(&dl->_logger_mutex);
    vector<string>::iterator end = dl->_log_buffer.end();
    for (vector<string>::iterator it = dl->_log_buffer.begin();
         it != end; ++it)
    {
        log_file << *it;
        cout << *it;
    }
    dl->_log_buffer.clear();
    pthread_mutex_unlock(&dl->_logger_mutex);

    //-----------------------------------------------------------------
    // Close log file
    log_file.close();

    return NULL;
}


static void *
buffer_function
(
    void * args
)
{
    DebugLogger * dl = reinterpret_cast<DebugLogger *>(args);

    //----------------------------------------------------------------
    // Create directory for images if we want to save them
    stringstream ss;
    ss << IMAGES_FOLDER_PREFIX << "_" << get_current_time_string();
    string foldername(ss.str());

    if (-1 == mkdir(foldername.c_str(), S_IRWXU | S_IRWXG|  S_IRWXO)) {
        perror("Error creating images directory");
    }

    //-----------------------------------------------------------------
    // Loop until the program ends
    while (!dl->_end)
    {
        // Copy Background
        if (dl->_bg_pool)
        {
            Frame *    bg_frame     = 0;
            int        bg_frame_num = 0;
            IplImage * bg_clone     = 0;

            //-------------------------------------------------------------
            // Acquire background image
            bg_frame_num = dl->_bg_pool->acquire(&bg_frame,
                                                 dl->_last_bg_frame_num,
                                                 false,
                                                 true);

            if (bg_frame && bg_frame_num > 0 &&
                bg_frame_num != dl->_last_bg_frame_num)
            {
                // Create a copy of the background image
                bg_clone = cvCloneImage(bg_frame->_bgr);
            }

            // Release image
            dl->_bg_pool->release(bg_frame);

            //-------------------------------------------------------------
            // Place background copy into circular buffer
            Frame & frame = dl->_bg_buffer[dl->_bg_index];
            if (NULL != frame._bgr)
            {
                // Free resources for old buffer
                cvReleaseImage(&frame._bgr);
            }
            frame._bgr = bg_clone;
            frame._frame_num = bg_frame_num;
            dl->UpdateBGIndex();
        }

        // CopyForeground
        if (dl->_fg_pool)
        {
            Frame *    fg_frame     = 0;
            int        fg_frame_num = 0;
            IplImage * fg_clone     = 0;

            //-------------------------------------------------------------
            // Acquire foreground image
            fg_frame_num = dl->_fg_pool->acquire(&fg_frame,
                                                 dl->_last_bg_frame_num,
                                                 false,
                                                 true);

            if (fg_frame && fg_frame_num > 0 &&
                fg_frame_num != dl->_last_fg_frame_num)
            {
               // Create a copy of the background image
               fg_clone = cvCloneImage(fg_frame->_bgr);
           }

            // Release image
            dl->_fg_pool->release(fg_frame);

            //-------------------------------------------------------------
            // Place foreground copy into circular buffer
            Frame & frame = dl->_bg_buffer[dl->_bg_index];
            if (NULL != frame._bgr)
            {
                // Free resources for old buffer
                cvReleaseImage(&frame._bgr);
            }
            frame._bgr = fg_clone;
            frame._frame_num = fg_frame_num;
            dl->UpdateFGIndex();
        }

        // Sleep 10 milliseconds
        usleep(10000);
    }

    //-----------------------------------------------------------------
    // Save the buffers of the last nframes to disk

    // Save Background
    if (dl->_bg_pool)
    {
        // figure out index of oldest item in circular buffer
        int start_index = (NULL == dl->_bg_buffer[dl->_bg_index]._bgr)?
                          0 : dl->_bg_index;

        int i = start_index;
        while (dl->_bg_buffer[i]._bgr)
        {
            Frame & frame = dl->_bg_buffer[i];

            // Save image to file
            stringstream ss;
            ss << foldername << "/bg_image_" << frame._frame_num << ".png";
            string filename(ss.str());

            cout << "Saving " << filename << "..." << endl;

            cvSaveImage(filename.c_str(), frame._bgr);
            cvReleaseImage(&frame._bgr);
            frame._bgr = NULL;

            i = (i + 1) % dl->_nframes_to_save;
        }
    }

    // Save Foreground
    if (dl->_fg_pool)
    {
        // figure out index of oldest item in circular buffer
        int start_index = (NULL == dl->_fg_buffer[dl->_fg_index]._bgr)?
                          0 : dl->_fg_index;

        int i = start_index;
        while (dl->_fg_buffer[i]._bgr)
        {
            Frame & frame = dl->_fg_buffer[i];

            // Save image to file
            stringstream ss;
            ss << foldername << "/fg_image_" << frame._frame_num << ".png";
            string filename(ss.str());

            cout << "Saving " << filename << "..." << endl;

            cvSaveImage(filename.c_str(), frame._bgr);
            cvReleaseImage(&frame._bgr);
            frame._bgr = NULL;

            i = (i + 1) % dl->_nframes_to_save;
        }
    }

    return NULL;
}
