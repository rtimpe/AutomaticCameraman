#ifndef GAME_CONTROLLER_PUBLIC
#define GAME_CONTROLLER_PUBLIC

#include "Public.h"
#include "GridController.h"
#include "StickController.h"

#define _INTERFACE_ class
#define _IMPLEMENTS_ public

class FramePool;
class BallController;

//////////////////////////////////////////////////////////////////////////////////////////////////////////


class GameController
{
    friend class GameAnnotator;

public:

    enum EngagementState
    {
        PROMPT_STATE,
        INSTRUCTION_STATE,
        COUNTDOWN_STATE,
        RECORD_STATE,
        FEEDBACK_STATE,
        RESET_STATE,
    };

    GameController(
        void                (*toggleRecord)(void),
        int                   img_w,
        int                   img_h,
        int                   recording_duration,
        FramePool *           video_pool,
        StickController *     stick_controller,
        GridController *      grid_controller
    );
	~GameController(void);
	void start(void);
	void stop(void);
	void updateState(void);
	double timeElapedInCurrentStateMS(void) const;
	void resetEngagementTargets(void);

	void doPromptState(void);
	void doInstructionState(void);
	void doCountdownState(void);
	void doRecordState(void);
	void doFeedbackState(void);
	void doResetState(void);


	void           (*_toggleRecord)(void);  // function to toggle video recording
	bool             _recording_started;    // true if recording has been started
	int              _img_w;                // width of image in pixels
	int              _img_h;                // height of image in pixels
	int              _recording_duration;   // how many seconds to record
	int              _instruction_duration; // how many seconds to keep instructions on screen
	int              _countdown_duration;   // Number of seconds to coundown before starting
    EngagementState  _current_state;        // current state in state machine
    bool             _current_state_done;   // true if we are done with this state
    bool             _end;                  // true if someone invokes stop() method
    FramePool *      _video_pool;           // the video pool for tracking
    int              _frame_num;
    bool             _is_first_frame;
    int              _first_frame_num;
    int              _engaged_target_id;
    int              _current_score;
    int              _hoop_center_x;
    int              _hoop_center_y;
    int              _hoop_radius;

    StickController *     _stick_controller;      // We need to know location of ball
    GridController *      _grid_controller;

    std::vector<GridSquare>  _engagement_targets;      // grid of squares
    std::vector<cv::Point2f> _prev_corners; // corners detected in previous frame
    cv::Mat                  _prev_sub_img; // previous window
    cv::Point2f              _avg_flow_vec; // the average of the vectors from optical flow
    float                    _avg_flow_vec_magnitude; // magnitude of _avg_flow_vec
protected:

	double          _state_start_time;     // Time we started current state
	pthread_t       _thread;               // Thread for tracking

};


#endif // #define GAME_CONTROLLER_PUBLIC



