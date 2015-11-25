#include "GameController.h"
#include "FramePool.h"
#include "GridController.h"

using std::cout;
using std::endl;


static void * controller_function(void * args);

static bool out_file_created = false;
static bool first_line = false;
static ofstream out_file;


//*****************************************************************************
// GameController methods
//*****************************************************************************
GameController::GameController
(
    void                (*toggleRecord)(void),
    int                   img_w,
    int                   img_h,
    int                   recording_duration,
    FramePool *           video_pool,
    StickController *     stick_controller,
    GridController *      grid_controller,
    DebugLogger *         debug_logger
)
: _toggleRecord(toggleRecord),
  _recording_started(false),
  _img_w(img_w), _img_h(img_h), _recording_duration(recording_duration),
  _instruction_duration(4), _countdown_duration(3),
  _current_state(PROMPT_STATE), _current_state_done(false), _end(false),
  _video_pool(video_pool),
  _frame_num(0), _is_first_frame(true), _first_frame_num(0),
  _current_score(0),
  _stick_controller(stick_controller), _grid_controller(grid_controller),
  _spin_wav_file_path("assets/tone.wav"), _spin_wav_handle(0),
  _audio_player(new AudioPlayer()),
  _debug_logger(debug_logger)
{
    _state_start_time = get_current_time_ms();
    _spin_wav_handle = _audio_player->loadWav(_spin_wav_file_path);
    _stick_controller->registerListener(this);
}


void
GameController::start
(
    void
)
{
    pthread_create(&_thread, NULL, controller_function, this);
}


void
GameController::stop
(
    void
)
{
    _end = true;
    pthread_join(_thread, NULL);
}


void
GameController::updateState
(
    void
)
{
    if (_current_state_done)
    {
        switch (_current_state)
        {
        case PROMPT_STATE:
            _current_state = INSTRUCTION_STATE;
            break;
        case INSTRUCTION_STATE:
            _current_state = COUNTDOWN_STATE;
            break;
        case COUNTDOWN_STATE:
            _current_state = RECORD_STATE;

            //_grid_controller->start();
            //_stick_controller->start();
            _current_score = 0;
            break;
        case RECORD_STATE:
            _current_state = FEEDBACK_STATE;

            //_stick_controller->stop();
            //_grid_controller->stop();
            break;
        case FEEDBACK_STATE:
            _current_state = RESET_STATE;
            break;
        case RESET_STATE:
            _current_state = PROMPT_STATE;

            _is_first_frame = true;

//            _grid_controller->reset();
        }

        _current_state_done = false;
        _state_start_time   = get_current_time_ms();
    }
}


double
GameController::timeElapedInCurrentStateMS
(
    void
)
const
{
    double current_time = get_current_time_ms();
    return current_time - _state_start_time;
}


GameController::~GameController
(
    void
)
{
}


void
GameController::doPromptState
(
    void
)
{
}


void
GameController::doInstructionState
(
    void
)
{
    double time_elapsed = 0.001 * timeElapedInCurrentStateMS();
    _current_state_done = time_elapsed > (double)_instruction_duration+ 1.0;
}


void
GameController::doCountdownState
(
    void
)
{
    double time_elapsed = 0.001 * timeElapedInCurrentStateMS();
    _current_state_done = time_elapsed > (double)_countdown_duration + 1.0;
}


void
GameController::doRecordState
(
    void
)
{
    // Start recording if it hasn't been started yet
    if (!_recording_started)
    {
        _toggleRecord();
        _recording_started = true;
    }

    double time_elapsed = 0.001 * timeElapedInCurrentStateMS();
    _current_state_done = time_elapsed > _recording_duration;

    // Stop Recording if duration is up
    if (_current_state_done && _recording_started)
    {
        _toggleRecord();
        _recording_started = false;

        // save the score as well
        _debug_logger->recordGameScoreSQL(_current_score);
    }
}


void
GameController::doFeedbackState
(
    void
)
{
    double time_elapsed = 0.001 * timeElapedInCurrentStateMS();
    _current_state_done = time_elapsed > 6.0;
}


void
GameController::doResetState
(
    void
)
{
    double time_elapsed = 0.001 * timeElapedInCurrentStateMS();
    _current_state_done = time_elapsed > 6.0;
}


void
GameController::handleSpinCompleted
(
    void
)
{
    if (PROMPT_STATE == _current_state)
    {
        _current_state_done = true;
    } 
    else if (RECORD_STATE == _current_state)
    {
        ++_current_score;
    }

    //_audio_player->playWav(_spin_wav_handle);
}



//*****************************************************************************
// local (Non-class) functions
//*****************************************************************************

static void *
controller_function
(
    void *arg
)
{
    GameController * ec = (GameController *)arg;

    //stringstream ss;
    //ss << "opticalflow_" << get_current_datetime_string();

    //out_file.open(ss.str().c_str());
    //out_file_created = true;


    while (!ec->_end)
    {
        switch (ec->_current_state)
        {
        case GameController::PROMPT_STATE:
            ec->doPromptState();
            break;
        case GameController::INSTRUCTION_STATE:
            ec->doInstructionState();
            break;
        case GameController::COUNTDOWN_STATE:
            ec->doCountdownState();
            break;
        case GameController::RECORD_STATE:
            ec->doRecordState();
            break;
        case GameController::FEEDBACK_STATE:
            ec->doFeedbackState();
            break;
        case GameController::RESET_STATE:
            ec->doResetState();
            break;
        }

        ec->updateState();

        usleep(10*1000);
    }

    //out_file.close();

    return NULL;
}









