#ifndef GAME_CONTROLLER_PUBLIC
#define GAME_CONTROLLER_PUBLIC

#include "Public.h"
#include "GridController.h"
#include "StickController.h"
#include "AudioPlayer.h"


class FramePool;
class BallController;


//////////////////////////////////////////////////////////////////////////////////////////////////////////


class GameController : _IMPLEMENTS_ StickControllerListener
{
    friend class GameAnnotator;

public:

    enum GameState
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

	void doPromptState(void);
	void doInstructionState(void);
	void doCountdownState(void);
	void doRecordState(void);
	void doFeedbackState(void);
	void doResetState(void);
	virtual void handleSpinCompleted(void);


	void           (*_toggleRecord)(void);  // function to toggle video recording
	bool             _recording_started;    // true if recording has been started
	int              _img_w;                // width of image in pixels
	int              _img_h;                // height of image in pixels
	int              _recording_duration;   // how many seconds to record
	int              _instruction_duration; // how many seconds to keep instructions on screen
	int              _countdown_duration;   // Number of seconds to coundown before starting
    GameState  _current_state;        // current state in state machine
    bool             _current_state_done;   // true if we are done with this state
    bool             _end;                  // true if someone invokes stop() method
    FramePool *      _video_pool;           // the video pool for tracking
    int              _frame_num;
    bool             _is_first_frame;
    int              _first_frame_num;
    int              _current_score;

    StickController *     _stick_controller;      // We need to know location of ball
    GridController *      _grid_controller;

    std::string           _spin_wav_file_path;
    WavHandle *           _spin_wav_handle;
    AudioPlayer *         _audio_player;

protected:

	double          _state_start_time;     // Time we started current state
	pthread_t       _thread;               // Thread for tracking

};


#endif // #define GAME_CONTROLLER_PUBLIC



