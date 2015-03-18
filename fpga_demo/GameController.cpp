#include "GameController.h"
#include "FramePool.h"
#include "BallController.h"
#include "GridController.h"

using std::cout;
using std::endl;


static void * controller_function(void * args);

static bool out_file_created = false;
static bool first_line = false;
static ofstream out_file;


//*****************************************************************************
// EngagementController methods
//*****************************************************************************
GameController::GameController
(
    void                (*toggleRecord)(void),
    int                   img_w,
    int                   img_h,
    int                   recording_duration,
	FramePool *           video_pool,
    BallController *      ball_controller,
	GridController *      grid_controller
)
: _toggleRecord(toggleRecord),
  _recording_started(false),
  _img_w(img_w), _img_h(img_h), _recording_duration(recording_duration),
  _instruction_duration(5), _countdown_duration(3),
  _current_state(PROMPT_STATE), _current_state_done(false), _end(false),
  _video_pool(video_pool),
  _frame_num(0), _is_first_frame(true), _first_frame_num(0),
  _engaged_target_id(0), _engagement_triggered_time(0.0),
  _current_score(0),
  _hoop_center_x(img_w/2), _hoop_center_y(60), _hoop_radius(80),
  _ball_controller(ball_controller), _grid_controller(grid_controller)
{
    _state_start_time = get_current_time_ms();

    // Create targets
    //_targets.push_back(GridSquare(400, 400, 72, 72));
    _engagement_targets.push_back(GridSquare(100, 100, 50, 50));
    _engagement_targets.push_back(GridSquare(1000, 100, 50, 50));
    _engagement_targets.push_back(GridSquare(100, 650, 50, 50));
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

            _grid_controller->start();
            _ball_controller->start();
            _current_score = 0;
            break;
        case RECORD_STATE:
            _current_state = FEEDBACK_STATE;

            _ball_controller->stop();
            _grid_controller->stop();
            break;
        case FEEDBACK_STATE:
            _current_state = RESET_STATE;
            break;
        case RESET_STATE:
            _current_state = PROMPT_STATE;

            _is_first_frame = true;
            resetEngagementTargets();

            _grid_controller->reset();
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
GameController::resetEngagementTargets
(
    void
)
{
    SquareIter end = _engagement_targets.end();
    for (SquareIter it = _engagement_targets.begin();
         end != it; ++it)
    {
        GridSquare & gs = *it;
        gs.reset();
    }
    _engaged_target_id = 0;
}


#if 0
void
GameController::doPromptState
(
    void
)
{
    Frame * frame = NULL;

    _video_pool->acquire(&frame, _frame_num, false, true);

    cv::Mat img(frame->_bgr);

    SquareIter end = _engagement_targets.end();
    for (SquareIter it = _engagement_targets.begin();
         end != it; ++it)
    {
        GridSquare & gs = *it;

        //-------------------------------------------------------------
        // Get greyscale image of window
        cv::Mat sub_img = img(cv::Range(gs._x0, gs._x0 + gs._w),
                              cv::Range(gs._y0, gs._y0 + gs._h));
        cv::Mat grey_sub_img;
        cv::cvtColor(sub_img,
                     grey_sub_img,
                     cv::COLOR_BGR2GRAY);

        //cout << "source type is " << grey_sub_img.type() << endl;



        //-------------------------------------------------------------
        // Compute optical flow
        vector<cv::Point2f> next_corners;
        vector<unsigned char> status;
        vector<float> errs;

        if (!_prev_corners.empty())
        {
            cv::calcOpticalFlowPyrLK(_prev_sub_img,
                                     grey_sub_img,
                                     _prev_corners,
                                     next_corners,
                                     status,
                                     errs);
        }

        //-------------------------------------------------------------
        // Get vectors

        cv::Point2f avg_vec(0.f, 0.f);
        int count = 0;
        _avg_flow_vec = avg_vec; // reset

        for (unsigned int i = 0; i < status.size(); ++i)
        {
            if (status[i])
            {
                float error = MIN(errs[i], 100.0f);
                float coef  = (100.f - error)*(100.f - error)*0.0001;

                cv::Point2f & t0 = _prev_corners[i];
                cv::Point2f & t1 = next_corners[i];
                cv::Point2f delta = coef*(t1 - t0);

                avg_vec = avg_vec + delta;

                ++count;
            }
        }

        float magnitude = 0.f;

        if (count > 0)
        {
            avg_vec.x = avg_vec.x / count;
            avg_vec.y = avg_vec.y / count;
            magnitude = sqrtf(avg_vec.x*avg_vec.x + avg_vec.y*avg_vec.y);

            _avg_flow_vec = avg_vec;
            _avg_flow_vec_magnitude = magnitude;
        }

        if (magnitude > 1.8)
        {
            cout << "magnitude is " << magnitude << endl;
            cout << "Avg vec is [" << avg_vec.x << "," << avg_vec.y << "]" <<endl;
        }

        out_file << avg_vec.x << "," << avg_vec.y << endl;

        //-------------------------------------------------------------
        // Get features for the current frame
        vector<cv::Point2f> corners;
        cv::goodFeaturesToTrack(grey_sub_img,   // input image
                                corners,        // output
                                16,             // max corners
                                0.1,            // quality level
                                1.0);
        //cout << "Num features detected is " << corners.size() << endl;

        // Update previous frame (for the next iteration)
        grey_sub_img.copyTo(_prev_sub_img);
        _prev_corners = corners;
    }

    ++_frame_num;

    _video_pool->release(frame);
}

#else

void
GameController::doPromptState
(
    void
)
{
    Frame * frame = NULL;

    int new_frame_num = _video_pool->acquire(&frame,
                                             _frame_num,
                                             false,
                                             true);

    if (NULL == frame || new_frame_num <= _frame_num)
    {
        return;
    }

    _frame_num = new_frame_num;

    cv::Mat img(frame->_bgr);

    int target_id = 0;

    SquareIter end = _engagement_targets.end();
    for (SquareIter it = _engagement_targets.begin();
         end != it; ++it)
    {
        GridSquare & gs = *it;
        ++target_id;

        //-------------------------------------------------------------
        // Compute the RGB averages for each of the target squares
        double averageR = 0.0;
        double averageG = 0.0;
        double averageB = 0.0;

        for (int j = gs._x0; j < gs._x0 + gs._w; ++j)
        {
            for (int k = gs._y0; k < gs._y0 + gs._h; ++k)
            {
                cv::Vec3b pixel = img.at<cv::Vec3b>(k, j);
                averageB += pixel[0];
                averageG += pixel[1];
                averageR += pixel[2];
            }
        }
        averageR /= (double) (gs._w * gs._h);
        averageG /= (double) (gs._w * gs._h);
        averageB /= (double) (gs._w * gs._h);

        if (_is_first_frame)
        {
            gs.runningMeanB = averageB;
            gs.runningMeanR = averageR;
            gs.runningMeanG = averageG;
            gs.meanShortB = averageB;
            gs.meanShortR = averageR;
            gs.meanShortG = averageG;
            gs.runningMeanSquaredB = averageB * averageB;
            gs.runningMeanSquaredR = averageR * averageR;
            gs.runningMeanSquaredG = averageG * averageG;

            _is_first_frame = false;
            _first_frame_num = _frame_num;
        }
        else
        {
            if (!gs.occupied) {
                double alpha = _grid_controller->longAlpha;
                gs.runningMeanR = (1.0 - alpha) * gs.runningMeanR + alpha * averageR;
                gs.runningMeanSquaredR = (1.0 - alpha) * gs.runningMeanSquaredR + alpha * averageR * averageR;
                gs.runningMeanG = (1.0 - alpha) * gs.runningMeanG + alpha * averageG;
                gs.runningMeanSquaredG = (1.0 - alpha) * gs.runningMeanSquaredG + alpha * averageG * averageG;
                gs.runningMeanB = (1.0 - alpha) * gs.runningMeanB + alpha * averageB;
                gs.runningMeanSquaredB = (1.0 - alpha) * gs.runningMeanSquaredB + alpha * averageB * averageB;
            }

            cout << "meanShortB=" << gs.meanShortB << endl;

            double smallAlpha = _grid_controller->shortAlpha;
            gs.meanShortB = (1.0 - smallAlpha) * gs.meanShortB + smallAlpha * averageB;
            gs.meanShortG = (1.0 - smallAlpha) * gs.meanShortG + smallAlpha * averageG;
            gs.meanShortR = (1.0 - smallAlpha) * gs.meanShortR + smallAlpha * averageR;

            double stdR = std::sqrt(gs.runningMeanSquaredR - gs.runningMeanR * gs.runningMeanR);
            double stdG = std::sqrt(gs.runningMeanSquaredG - gs.runningMeanG * gs.runningMeanG);
            double stdB = std::sqrt(gs.runningMeanSquaredB - gs.runningMeanB * gs.runningMeanB);

            double diffR = std::abs(gs.meanShortR - gs.runningMeanR);
            double diffG = std::abs(gs.meanShortG - gs.runningMeanG);
            double diffB = std::abs(gs.meanShortB - gs.runningMeanB);

            double diff = _grid_controller->diff;

            if (_frame_num > 200 + _first_frame_num) {
                cout << "diffR=" << diffR << ", diff*stdR=" << diff*stdR << endl;
                if (diffR > diff * stdR || diffG > diff * stdG || diffB > diff * stdB) {
                    gs.occupied = true;

                    // Verify that targets are activated in order
                    if (target_id == _engaged_target_id ||
                        target_id == _engaged_target_id + 1)
                    {
                        gs.occupied = true;
                        _engaged_target_id = target_id;
                        _engagement_triggered_time = get_current_time_ms();
                    }
                    else
                    {
                        _engaged_target_id = 0;
                    }

                } else {
                    //gs.occupied = false;
                }
            }
        }
    }


    // Clear targets if timer expired or if targets are activated out of order
    if (0 == _engaged_target_id ||
        get_current_time_ms() - _engagement_triggered_time > 5000)
    {
        SquareIter end = _engagement_targets.end();
        for (SquareIter it = _engagement_targets.begin();
             end != it; ++it)
        {
            GridSquare & gs = *it;
            gs.occupied = false;
        }
    }


    _video_pool->release(frame);

    //cout << "target id is now " << _engaged_target_id << endl;
    if (3 == _engaged_target_id)
    {
        cout << "State is done!" << endl;
        _current_state_done = true;
    }
}
#endif


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

    // Check if the ball is inside the hoop
    float xdiff = static_cast<float>(_hoop_center_x - _ball_controller->xPos);
    float ydiff = static_cast<float>(_hoop_center_y - _ball_controller->yPos);
    float distance = sqrtf(xdiff*xdiff + ydiff*ydiff);
    if (distance <= static_cast<float>(_hoop_radius))
    {
        // If we weren't in the hoop before, udpate the score!
        if (!_is_in_hoop)
        {
            ++_current_score;
            _is_in_hoop = true;
        }
    }
    else if (_is_in_hoop)
    {
        // We moved from inside to outside the hoop!
        _is_in_hoop = false;
    }

    double time_elapsed = 0.001 * timeElapedInCurrentStateMS();
    _current_state_done = time_elapsed > _recording_duration;

    // Stop Recording if duration is up
    if (_current_state_done && _recording_started)
    {
        _toggleRecord();
        _recording_started = false;
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

    // Shut down grid and ball controller threads if necessary
    if (GameController::RECORD_STATE == ec->_current_state)
    {
        ec->_ball_controller->stop();
        ec->_grid_controller->stop();
    }

    //out_file.close();

    return NULL;
}








