#include "GameAnnotator.h"
#include "GameController.h"
#include "GridAnnotator.h"


GameAnnotator::GameAnnotator
(
    int id,
    GameController * game_controller,
    GridAnnotator * grid_annotator
)
:_controller(game_controller), _grid_annotator(grid_annotator)
{
    _id = id;
}


bool
GameAnnotator::draw_long_term
(
    IplImage *img
)
{
    if (GameController::RECORD_STATE == _controller->_current_state) {
        _grid_annotator->draw_long_term(img);
    }
}


void
GameAnnotator::draw_short_term
(
    IplImage *img
)
{
    string state_name;

    switch (_controller->_current_state)
    {
    case GameController::PROMPT_STATE:
        draw_prompt_state(img);
        state_name = "PROMPT_STATE";
        break;
    case GameController::INSTRUCTION_STATE:
        draw_instruction_state(img);
        state_name = "INSTRUCTION_STATE";
        break;
    case GameController::COUNTDOWN_STATE:
        draw_countdown_state(img);
        state_name = "COUNTDOWN_STATE";
        break;
    case GameController::RECORD_STATE:
        draw_record_state(img);
        state_name = "RECORD_STATE";
        break;
    case GameController::FEEDBACK_STATE:
        draw_feedback_state(img);
        state_name = "FEEDBACK_STATE";
        break;
    case GameController::RESET_STATE:
        draw_reset_state(img);
        state_name = "RESET_STATE";
        break;
    }
}


void
GameAnnotator::draw_prompt_state
(
    IplImage *img
)
{
    cv::Scalar default_color(230.0, 160.0, 10.0, 255.0);
    cv::Scalar occupied_color(0.0, 0.0, 255.0, 255.0);

    // Draw the targets
    int i = 0;
    SquareIter end = _controller->_engagement_targets.end();
    for (SquareIter iter = _controller->_engagement_targets.begin();
         iter != end; ++iter)
    {
        GridSquare & square = *iter;

        cv::Scalar color = (square.occupied)? occupied_color :
                                              default_color;
        // Draw square
        draw_rect(img,
                  square._x0,
                  square._y0,
                  square._w,
                  square._h,
                  1,
                  color.val[2],   // R
                  color.val[1],   // G
                  color.val[0],   // B
                  color.val[3]);  // alpha


        // Draw the digit inside the square
        std::stringstream ss;
        ss << i++;

        int    base_line  = 0;
        int    font_face  = CV_FONT_HERSHEY_PLAIN;
        double font_scale = 2.0;
        int    thickness  = 3;

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  square._x0 + square._w/2 - textSize.width/2,
                  square._y0 + square._h/2 + textSize.height/2,
                  ss.str(),
                  font_face,
                  font_scale,
                  color,
                  thickness,
                  8);
    }

    // Draw prompt message
    {
        std::stringstream ss;
        ss << "Active each square in the order listed!";

        int    base_line  = 0;
        int    font_face  = CV_FONT_HERSHEY_PLAIN;
        double font_scale = 2.0;
        int    thickness  = 3;

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  img->width/2 - textSize.width/2,
                  (int)img->height/4,
                  ss.str(),
                  font_face,
                  font_scale,
                  default_color,
                  thickness,
                  8);
    }

    // Draw arrow
    if (_controller->_avg_flow_vec_magnitude > 2.0f)
    {
        cv::Point p1(img->width/2, img->height/2);
        int thickness = 5;
        double scale = 30.0;

        cv::Point p2(p1.x + _controller->_avg_flow_vec.x * scale,
                     p1.y + _controller->_avg_flow_vec.y * scale);

        draw_line(img,
                  p1.x, p1.y, p2.x, p2.y,
                  thickness,
                  255, 255, 255, 255);
    }
}


void
GameAnnotator::draw_instruction_state
(
    IplImage *img
)
{
    // Draw text informing user that the game will start
    {
        std::stringstream ss;
        ss << "Try to get the ball into the hoop as";

        int        base_line  = 0;
        int        font_face  = CV_FONT_HERSHEY_PLAIN;
        double     font_scale = 3.0;
        int        thickness  = 5;
        cv::Scalar textColor(230.0, 160.0, 10.0, 255.0);

        cv::Size textSize1 = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  img->width/2 - textSize1.width/2,
                  (int)img->height/4,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);

        cv::Size textSize2 = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        ss.str("");
        ss << "many times as you can in " << _controller->_recording_duration
           << " seconds!";

        draw_text(img,
                  img->width/2 - textSize2.width/2,
                  (int)img->height/4 + textSize1.height + 20,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);
    }

    if (GameController::RECORD_STATE == _controller->_current_state) {
        _grid_annotator->draw_short_term(img);
    }
}


void
GameAnnotator::draw_countdown_state
(
    IplImage *img
)
{
    int seconds_elapsed =
        (int)(floor(0.001 * _controller->timeElapedInCurrentStateMS()) + 0.5);
    int seconds_remaining = _controller->_countdown_duration - seconds_elapsed;

    // Draw text informing user that the game will start
    if (0 < seconds_remaining)
    {
        std::stringstream ss;
        ss << "Game will start in:";

        int        base_line  = 0;
        int        font_face  = CV_FONT_HERSHEY_PLAIN;
        double     font_scale = 6.0;
        int        thickness  = 8;
        cv::Scalar textColor(230.0, 160.0, 10.0, 255.0);

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  img->width/2 - textSize.width/2,
                  (int)img->height/4,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);
    }


    // Draw the countdown timer
    if (0 < seconds_remaining)
    {
        std::stringstream ss;
        ss << seconds_remaining;

        int        base_line  = 0;
        int        font_face  = CV_FONT_HERSHEY_PLAIN;
        double     font_scale = 20.0;
        int        thickness  = 20;
        cv::Scalar textColor(230.0, 160.0, 10.0, 255.0);

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  img->width/2 - textSize.width/2,
                  img->height/2 + textSize.height/2,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);
    }
}


void
GameAnnotator::draw_record_state
(
    IplImage *img
)
{
    int seconds_elapsed =
        (int)(floor(0.001 * _controller->timeElapedInCurrentStateMS()) + 0.5);

    // Draw a flashing "REC" sign in the corner
    if (0 == seconds_elapsed % 2)
    {
        std::stringstream ss;
        ss << "REC";

        int    base_line  = 0;
        int    font_face  = CV_FONT_HERSHEY_PLAIN;
        double font_scale = 2.0;
        int    thickness  = 4;

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  img->width/20 - textSize.width/2,
                  img->height/18 + textSize.height/2,
                  ss.str(),
                  font_face,
                  font_scale,
                  cv::Scalar(0.0, 0.0, 255.0, 255.0),
                  thickness,
                  8);
    }

    // Draw a small timer at the bottom right showing how much time remains
    {
        std::stringstream ss;
        int time_remaining = _controller->_recording_duration - seconds_elapsed;
        int minutes_remaining = time_remaining / 60;
        int seconds_remaining = time_remaining % 60;
        if (minutes_remaining < 10)
            ss << "0";
        ss << minutes_remaining << ":" ;
        if (seconds_remaining < 10)
            ss << "0";
        ss<< seconds_remaining;

        int        base_line  = 0;
        int        font_face  = CV_FONT_HERSHEY_PLAIN;
        double     font_scale = 2.0;
        int        thickness  = 4;
        cv::Scalar textColor  = (time_remaining > 5)?
                                cv::Scalar(230.0, 160.0, 10.0, 255.0) :
                                cv::Scalar(0.0, 0.0, 255.0, 255.0);

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  static_cast<int>(img->width * (19.0/20.0)) - textSize.width/2,
                  static_cast<int>(img->height * (19.8/20.0)) - textSize.height/2,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);
    }

    // Draw current score
    {
        std::stringstream ss;
        ss << "SCORE: " << _controller->_current_score;

        int        base_line  = 0;
        int        font_face  = CV_FONT_HERSHEY_PLAIN;
        double     font_scale = 2.0;
        int        thickness  = 4;
        cv::Scalar textColor(230.0, 160.0, 10.0, 255.0);

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  static_cast<int>(img->width * (19.5/20.0)) - textSize.width,
                  static_cast<int>(img->height * (0.5/20.0)) + textSize.height/2,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);
    }

    // Draw hoop
    {
        cv::Scalar textColor(230.0, 160.0, 10.0, 255.0);

        draw_circle(img,
                    _controller->_hoop_center_x,
                    _controller->_hoop_center_y,
                    _controller->_hoop_radius,
                    3, // thickness of line
                    textColor[2],
                    textColor[1],
                    textColor[0],
                    textColor[3]);
    }
}


void
GameAnnotator::draw_feedback_state
(
    IplImage *img
)
{
    int seconds_elapsed =
        (int)(floor(0.001 * _controller->timeElapedInCurrentStateMS()) + 0.5);

    if (seconds_elapsed < 7)
    {
        //-------------------------------------------------------------
        // Display final score

        std::stringstream ss;
        ss << "Final Score: " << _controller->_current_score;

        int    base_line  = 0;
        int    font_face  = CV_FONT_HERSHEY_PLAIN;
        double font_scale = 6.0;
        int    thickness  = 6;
        cv::Scalar textColor(230.0, 160.0, 10.0, 255.0);

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  img->width/2 - textSize.width/2,
                  img->height/2 + textSize.height/2,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);
    }
}


void
GameAnnotator::draw_reset_state
(
    IplImage *img
)
{
    int seconds_elapsed =
        (int)(floor(0.001 * _controller->timeElapedInCurrentStateMS()) + 0.5);


    if (0 == seconds_elapsed % 2)
    {
        //-------------------------------------------------------------
        // Display flashing "GAME OVER" text

        std::stringstream ss;
        ss << "GAME OVER";

        int    base_line  = 0;
        int    font_face  = CV_FONT_HERSHEY_PLAIN;
        double font_scale = 12.0;
        int    thickness  = 12;
        cv::Scalar textColor(230.0, 160.0, 10.0, 255.0);

        cv::Size textSize = cv::getTextSize(ss.str(),
                                            font_face,
                                            font_scale,
                                            thickness,
                                            &base_line);

        draw_text(img,
                  img->width/2 - textSize.width/2,
                  img->height/2 + textSize.height/2,
                  ss.str(),
                  font_face,
                  font_scale,
                  textColor,
                  thickness,
                  8);
    }
}
