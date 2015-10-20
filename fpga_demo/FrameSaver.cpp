#include "FrameSaver.h"
#include "FramePool.h"
#include "DebugLogger.h"
#include <pthread.h>
#include <sys/stat.h>
#include <sstream>
#include <opencv2/highgui/highgui.hpp>

using std::cout;
using std::endl;
using std::stringstream;

static char IMAGES_PARENT_FOLDER_NAME[] = "images";
static char IMAGES_FOLDER_PREFIX[] = "images";
static char VIDEO_FOLDER_NAME[] = "videos";


static void * frame_saver_function(void * args);


extern DebugLogger * debug_logger;

TS_INIT(framesaver, 2);
TC_INIT(framesaver, 1);


FrameSaver::FrameSaver
(
    int  nframes_to_save,
    bool save_video
)
: _nframes_to_save(nframes_to_save), _save_video(save_video), _end(false),
  _last_bg_frame_num(0), _last_fg_frame_num(0),
   _bg_index(0), _fg_index(0), _bg_buffer(0), _fg_buffer(0)
{
    _bg_buffer = new Frame[_nframes_to_save];
    _fg_buffer = new Frame[_nframes_to_save];

    memset(_bg_buffer, 0, sizeof(Frame)*_nframes_to_save);
    memset(_fg_buffer, 0, sizeof(Frame)*_nframes_to_save);
}


FrameSaver::~FrameSaver
(
    void
)
{
    delete[] _bg_buffer;
    delete[] _fg_buffer;
}


void
FrameSaver::SetBGPool
(
    FramePool * bg_pool
)
{
    _bg_pool = bg_pool;
}


void
FrameSaver::SetFGPool
(
    FramePool * fg_pool
)
{
    _fg_pool = fg_pool;
}


void
FrameSaver::start
(
    void
)
{
    _end = false;
    pthread_mutex_init(&_frame_saver_mutex, NULL);
    pthread_cond_init(&_frame_saver_cond, NULL);
    pthread_create(&_frame_saver_thread, NULL, frame_saver_function, this);
}


void
FrameSaver::stop
(
    void
)
{
    pthread_mutex_lock(&_frame_saver_mutex);

    _end = true;

    pthread_cond_signal(&_frame_saver_cond);
    pthread_mutex_unlock(&_frame_saver_mutex);

    pthread_join(_frame_saver_thread, NULL);
}


void
FrameSaver::UpdateBGIndex
(
    void
)
{
    _bg_index = (_bg_index + 1) % _nframes_to_save;
}


void
FrameSaver::UpdateFGIndex
(
    void
)
{
    _fg_index = (_fg_index + 1) % _nframes_to_save;
}

void
FrameSaver::ToggleRecord
(
    void
)
{
    pthread_mutex_lock(&_frame_saver_mutex);

    _save_video = !_save_video;

    pthread_cond_signal(&_frame_saver_cond);
    pthread_mutex_unlock(&_frame_saver_mutex);
}

static void
BufferBGImage
(
    FrameSaver * fs,
    IplImage *&  bg_clone
)
{
    if (fs->_bg_pool)
    {
        Frame *    bg_frame     = 0;
        int        bg_frame_num = 0;

        //---------------------------------------------------------
        // Acquire background image
        //cout << "Trying to acquire frame...(_last_bg_frame_num is " << fs->_last_bg_frame_num << ")" << endl;
        bg_frame_num = fs->_bg_pool->acquire(&bg_frame,
                                             fs->_last_bg_frame_num,
                                             false,
                                             true);

        if (bg_frame && bg_frame_num > 0 &&
            bg_frame_num != fs->_last_bg_frame_num)
        {
            // Create a copy of the background image
            bg_clone = cvCloneImage(bg_frame->_bgr);
        }
        fs->_last_bg_frame_num = bg_frame_num;

        // Release image
        fs->_bg_pool->release(bg_frame);

        //if (!bg_frame)
        //    cout << "No frame" << endl;
        //else
        //    cout << "bg_frame_num is " << bg_frame_num << endl;

        //---------------------------------------------------------
        // Place background copy into circular buffer
        if (fs->_nframes_to_save > 0 && bg_clone)
        {
            cout << "Saving image" << endl;
            Frame & frame = fs->_bg_buffer[fs->_bg_index];
            if (NULL != frame._bgr)
            {
                // Free resources for old buffer
                cvReleaseImage(&frame._bgr);
            }
            frame._bgr = bg_clone;
            frame._frame_num = bg_frame_num;
            fs->UpdateBGIndex();
        }

    } // if (fs->_bg_pool)

}


static void
BufferFGImage
(
    FrameSaver * fs,
    IplImage *&  fg_clone
)
{
    if (fs->_fg_pool)
    {
        Frame *    fg_frame     = 0;
        int        fg_frame_num = 0;

        //---------------------------------------------------------
        // Acquire foreground image

        fg_frame_num = fs->_fg_pool->acquire(&fg_frame,
                                             fs->_last_fg_frame_num,
                                             false,
                                             true);

        if (fg_frame && fg_frame_num > 0 &&
            fg_frame_num != fs->_last_fg_frame_num)
        {
           // Create a copy of the background image
           fg_clone = cvCloneImage(fg_frame->_bgr);
        }
        fs->_last_fg_frame_num = fg_frame_num;

        // Release image
        fs->_fg_pool->release(fg_frame);

        //---------------------------------------------------------
        // Place foreground copy into circular buffer
        if (fs->_nframes_to_save > 0 && fg_clone)
        {
            Frame & frame = fs->_fg_buffer[fs->_fg_index];
            if (NULL != frame._bgr)
            {
                // Free resources for old buffer
                cvReleaseImage(&frame._bgr);
            }
            frame._bgr = fg_clone;
            frame._frame_num = fg_frame_num;
            fs->UpdateFGIndex();
        }

    } //if (fs->_fg_pool)
}


static void
SaveVideo
(
    FrameSaver *      fs,
    cv::VideoWriter & video_writer,
    cv::VideoWriter & video_writer_annotated,
    IplImage *        bg_clone,
    IplImage *        fg_clone
)
{
    static IplImage * fg_clone_3      = 0;
    static IplImage * annotated_frame = 0;

    //-----------------------------------------------------------------
    // Initialize video if necessary
    if (!video_writer.isOpened())
    {
        // Open non-annotated video
        stringstream ss;
        ss << VIDEO_FOLDER_NAME << "/video_"
           << get_current_datetime_string() << ".mpeg";
        video_writer.open(ss.str(),
                          CV_FOURCC('P','I','M','1'),
                          //CV_FOURCC('M','P','G','4'),
                          45.0,
                          cv::Size(bg_clone->width, bg_clone->height),
                          true);
    }

    if (!video_writer_annotated.isOpened())
    {
        // Open annotated video
        stringstream ss;
        ss << VIDEO_FOLDER_NAME << "/an_video_"
           << get_current_datetime_string() <<".mpeg";
        video_writer_annotated.open(ss.str(),
                                    CV_FOURCC('P','I','M','1'),
                                    45.0,
                                    cv::Size(bg_clone->width, bg_clone->height),
                                    true);
    }

    //-----------------------------------------------------------------
    // initialize cvImages if necessary
    if (!fg_clone_3)
    {
        printf("[Creating fg_clone_3]\n");
        fg_clone_3 = cvCreateImage(cvSize(fg_clone->width,
                                          fg_clone->height),
                                   IPL_DEPTH_8U, 3);
    }

    if (!annotated_frame)
    {
        printf("[Creating annotated_frame]\n");
        annotated_frame = cvCreateImage(cvSize(bg_clone->width,
                                               bg_clone->height),
                                        IPL_DEPTH_8U, 3);
    }

    // We have to convert the 4 channel RGBA foreground image to
    // 3 channel RGB in order to add it to background RGBA image
    IppiSize roi;
    roi.width = fg_clone_3->width;
    roi.height = fg_clone_3->height;

    ippiCopy_8u_AC4C3R(
        reinterpret_cast<Ipp8u*>(fg_clone->imageData),
        fg_clone->widthStep,
        reinterpret_cast<Ipp8u*>(fg_clone_3->imageData),
        fg_clone_3->widthStep,
        roi);

    // Combine background and foreground frames into one
    cvAdd(bg_clone, fg_clone_3, annotated_frame);

    // Save to video
    video_writer.write(cv::Mat(bg_clone, false));
    video_writer_annotated.write(cv::Mat(annotated_frame, false));
}


static void
SaveBGToDisk
(
    FrameSaver *   fs,
    const string & image_foldername
)
{
    // figure out index of oldest item in circular buffer
    int start_index = (NULL == fs->_bg_buffer[fs->_bg_index]._bgr)?
                      0 : fs->_bg_index;

    //for (int j = 0; j < fs->_nframes_to_save; ++j)
    //{
    //    Frame & frame = fs->_bg_buffer[j];
    //    cout << "Frame[" << j << "] holds framenum " << frame._frame_num << endl;
    //}
    //cout << endl << endl;

    int i = start_index;
    while (fs->_bg_buffer[i]._bgr)
    {
        Frame & frame = fs->_bg_buffer[i];

        // Save image to file
        stringstream ss;
        ss << image_foldername << "/bg_image_" << frame._frame_num << ".png";
        string filename(ss.str());

        cout << "Saving " << filename << "..." << endl;

        cvSaveImage(filename.c_str(), frame._bgr);
        cvReleaseImage(&frame._bgr);
        frame._bgr = NULL;

        i = (i + 1) % fs->_nframes_to_save;
    }
}

static void
SaveFGToDisk
(
    FrameSaver *   fs,
    const string & image_foldername
)
{
    // figure out index of oldest item in circular buffer
    int start_index = (NULL == fs->_fg_buffer[fs->_fg_index]._bgr)?
                      0 : fs->_fg_index;

    int i = start_index;
    while (fs->_fg_buffer[i]._bgr)
    {
        Frame & frame = fs->_fg_buffer[i];

        // Save image to file
        stringstream ss;
        ss << image_foldername << "/fg_image_" << frame._frame_num << ".png";
        string filename(ss.str());

        cout << "Saving " << filename << "..." << endl;

        cvSaveImage(filename.c_str(), frame._bgr);
        cvReleaseImage(&frame._bgr);
        frame._bgr = NULL;

        i = (i + 1) % fs->_nframes_to_save;
    }
}


static void *
frame_saver_main_loop
(
    FrameSaver * fs
)
{
    cout << "Entered frame_saver_mail_loop" << endl;

    TS_STAMP(framesaver, 0);

    int nframes_captured = 0;
    cv::VideoWriter video_writer;
    cv::VideoWriter video_writer_annotated;

    //-----------------------------------------------------------------
    // Loop until either the program ends, or there's nothing to do
    // because recording was toggled off.
    while (!fs->_end &&
           (fs->_save_video || fs->_nframes_to_save > 0))
    {
        IplImage * bg_clone = 0;
        IplImage * fg_clone = 0;

        // Grab and copy Background frame into buffer
        BufferBGImage(fs, bg_clone);

        // Grab and copy Foreground frame into buffer
        BufferFGImage(fs, fg_clone);

        // Save video if necessary
        if (bg_clone && fg_clone)
        {
            ++nframes_captured;
            if (fs->_save_video)
            {
                SaveVideo(fs, 
                          video_writer, 
                          video_writer_annotated,
                          bg_clone, 
                          fg_clone);
            }
        }

        // Sleep 2 milliseconds
        usleep(2000);

        //-------------------------------------------------------------
        // If _nframes_to_save is 0 and we're saving video, we need to
        // release the clone frames here. (When _nframes_to_save is not 0,
        // the circular buffer takes care of freeing resources).
        if (fs->_save_video && 0 == fs->_nframes_to_save)
        {
            cvReleaseImage(&bg_clone);
            cvReleaseImage(&fg_clone);
        }
    }

    TS_STAMP(framesaver, 1);
    TC_ACCRUE(framesaver, 0, 1, 0);

    //-----------------------------------------------------------------
    // Compute fps of this thread
    double total_seconds = 0.001 * TC_SUM(framesaver, 0);
    double fps = nframes_captured / total_seconds;

    cout << "total seconds: " << total_seconds << endl;
    cout << "num frames saved: " << nframes_captured << endl;
    cout << "fps : " <<  fps << endl;

    stringstream ss;
    ss << "frame saver : saved video at " << fps << " fps" << endl;
    debug_logger->log(ss.str());

    //-----------------------------------------------------------------
    // Close the video file if one was created
    if (video_writer.isOpened())
    {
        video_writer.release();
    }

    if (video_writer_annotated.isOpened())
    {
        video_writer_annotated.release();
    }

    cout << "Exiting frame_saver_mail_loop" << endl;
}


static void *
frame_saver_function
(
    void * args
)
{
    FrameSaver * fs = reinterpret_cast<FrameSaver *>(args);

    //----------------------------------------------------------------
    // Create parent directories for images
    if (-1 == mkdir(IMAGES_PARENT_FOLDER_NAME,
                    S_IRWXU | S_IRWXG|  S_IRWXO)) {
        perror("Error creating images directory");
    }

    //----------------------------------------------------------------
    // Create sub directory for images for this run
    stringstream ss1;
    ss1 << IMAGES_PARENT_FOLDER_NAME << "/"
       << IMAGES_FOLDER_PREFIX << "_" << get_current_datetime_string();
    string img_foldername(ss1.str());

    if (fs->_nframes_to_save > 0)
    {
        if (-1 == mkdir(img_foldername.c_str(),
                        S_IRWXU | S_IRWXG|  S_IRWXO)) {
            perror("Error creating images directory");
        }
    }

    //-----------------------------------------------------------------
    // Create directory for the movie if we want to save one
    if (-1 == mkdir(VIDEO_FOLDER_NAME,
                    S_IRWXU | S_IRWXG|  S_IRWXO)) {
        perror("Error creating images directory");
    }

    //-----------------------------------------------------------------
    // Loop until the program is terminated
    while (!fs->_end)
    {
        // If there's something to do, enter the main loop
        if (fs->_save_video || fs->_nframes_to_save > 0)
        {
            frame_saver_main_loop(fs);
        }
        else
        {
            // If there's nothing to do, let's sleep until there IS something
            // to do
            pthread_mutex_lock(&fs->_frame_saver_mutex);

            if (!fs->_end && !fs->_save_video && fs->_nframes_to_save <= 0)
                pthread_cond_wait(&fs->_frame_saver_cond, &fs->_frame_saver_mutex);

            pthread_mutex_unlock(&fs->_frame_saver_mutex);
        }
    }


    //***********************************************************************
    //***** AT THIS POINT, THE USER HAS CHOSEN TO TERMINATE THE PROGRAM *****
    //***********************************************************************

    // Save last nframes of Background to disk
    if (fs->_bg_pool && fs->_nframes_to_save > 0)
    {
        SaveBGToDisk(fs, img_foldername);
    }

    // Save last nframes of Foreground to disk
    if (fs->_fg_pool && fs->_nframes_to_save > 0)
    {
        SaveFGToDisk(fs, img_foldername);
    }

    return NULL;
}
