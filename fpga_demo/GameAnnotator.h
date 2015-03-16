#ifndef GAME_ANNOTATOR_PUBLIC
#define GAME_ANNOTATOR_PUBLIC

#include "Annotator.h"


class GameController;
class GridAnnotator;


//////////////////////////////////////////////////////////////////////////////////////////////////////////
class GameAnnotator : public Annotator
{
public:
    GameAnnotator(int id, GameController * game_controller, GridAnnotator * grid_annotator);
    virtual ~GameAnnotator(void){};
    virtual bool draw_long_term(IplImage *img);
    virtual void draw_short_term(IplImage *img);

private:
    void draw_prompt_state(IplImage * img);
    void draw_instruction_state(IplImage * img);
    void draw_countdown_state(IplImage * img);
    void draw_record_state(IplImage * img);
    void draw_feedback_state(IplImage * img);
    void draw_reset_state(IplImage * img);

    GameController * _controller;
    GridAnnotator *  _grid_annotator;
};


#endif // #define GAME_ANNOTATOR_PUBLIC
