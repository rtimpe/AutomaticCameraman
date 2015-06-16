#include "Public.h"
#include "FramePool.h"
#include "VideoImages.h"
#include "FrameAnnotator.h"
#include "FrameDisplayer.h"
#include "GridAnnotator.h"
#include "GridController.h"
#include "GameController.h"
#include "BallController.h"
#include "GameAnnotator.h"
#include "DebugLogger.h"
#include "FrameSaver.h"
#include "StickController.h"
#include <riffa.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/stat.h>
#include <cstdlib>
#include <algorithm>
#include "DeckLinkAPI.h"
#include "DeckLinkCaptureDelegate.h"


// Forward declarations for functions
void handle_stop(void);
void toggle_record(void);
void runtime_logic(int width,
                   int height,
                   FrameAnnotator * annotator,
                   FramePool *      videoPool,
                   GridController * grid_controller,
				   StickController * stickController,
				   GameController * gameController);


// Global variables
int end = 0;
static char DISPLAYER_TITLE[] = "Capture Video";
char prefix[] = "../../milboosttracker-master/sylv/imgs/";
char stem[] = "%simg%05d.png";
int startFrame = 1;
int endFrame = 1344;
int startCenterX = 146;
int startCenterY = 84;
int startDim = 50; // with TRAIN_THRESH 7
int frameW = 320;
int frameH = 240;
int save_images = 0;
char images_folder[] = "images";


DebugLogger *       debug_logger = 0;
static FrameSaver * frame_saver = 0;


int myrandom(int i) { return std::rand() % i;};

void cameraStuff(FramePool *videoPool) {
    IDeckLink* deckLink = NULL;
    IDeckLinkIterator* deckLinkIterator = CreateDeckLinkIteratorInstance();
    HRESULT result;
    IDeckLinkDisplayMode* displayMode;
    IDeckLinkInput*  g_deckLinkInput = NULL;

    result = deckLinkIterator->Next(&deckLink);

    if (result != S_OK || deckLink == NULL)
    {
        fprintf(stderr, "Unable to get DeckLink device\n");
        return;
    }

    result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&g_deckLinkInput);
    if (result != S_OK) {
        cout << "error" << end;
        return;
    }

    g_deckLinkInput->EnableVideoInput(bmdModeHD720p5994, bmdFormat8BitBGRA, 0);

    DeckLinkCaptureDelegate * delegate = new DeckLinkCaptureDelegate(videoPool, 1280, 720);
    g_deckLinkInput->SetCallback(delegate);

    result = g_deckLinkInput->StartStreams();
    if (result != S_OK) {
        cout << result;
        cout << "fail starting streams\n";
    }
}

//*****************************************************************************
// * main
//*****************************************************************************
int
main
(
   int argc,
   const char** argv
)
{
    randinitalize(0);

    double shortAlpha = atof(argv[1]);
    double longAlpha = atof(argv[2]);
    double diff = atof(argv[3]);

    //----------------------------------------------------------------
    // Create a debug logger
    debug_logger = new DebugLogger();
    debug_logger->start();

    //-----------------------------------------------------------------
    // Create the FramePools

    // Make video pool size large enough to store the last 30 seconds
    // worth of video (for debugging purposes)
    int pool_size = 10;

    FramePool *videoPool = new FramePool(pool_size);
    FramePool *annotationPool = new FramePool(pool_size);


    //-----------------------------------------------------------------
	// Create Frame saver for saving video or for dumping the last n frames
	// upon program termination
	frame_saver = new FrameSaver(0,
	                             false);
    frame_saver->SetBGPool(videoPool);
    frame_saver->SetFGPool(annotationPool);
    frame_saver->start();

    //-----------------------------------------------------------------
    // Create the VideoImages source
    //VideoImages *video = new DiskImages(prefix, stem, startFrame, endFrame, 
    //                                    frameW, frameH, 30, videoPool);
//    VideoImages *video = new LiveImages(fpgaCapture,
//                                        0,
//                                        7000,
//                                        1280,
//                                        720,
//                                        60,
//                                        videoPool);

    int height = 720;
    int width = 1280;
    cameraStuff(videoPool);

//    if (!video->init()) {
//        printf("ERROR: Could not initialize the VideoImages.\n");
//        return -1;
//    }

    //-----------------------------------------------------------------
    // Create the FrameAnnotator
    FrameAnnotator *annotator = new FrameAnnotator(width,
                                                   height,
                                                   NULL,
                                                   annotationPool);
    if (!annotator->init()) {
        printf("ERROR: Could not initialize the FrameAnnotator.\n");
        return -1;
    }

    //-----------------------------------------------------------------
    // Create the FrameDisplayer
    FrameDisplayer *displayer = new FrameDisplayer(DISPLAYER_TITLE,
                                                   width,
                                                   height,
                                                   videoPool,
                                                   annotationPool,
                                                   handle_stop,
                                                   toggle_record);
    if (!displayer->init()) {
        printf("ERROR: Could not initialize the FrameDisplayer.\n");
        return -1;
    }

    //-----------------------------------------------------------------
    // Create controller object
    GridController * grid_controller = new GridController(10,
                                                          10,
                                                          10,
                                                          10,
                                                          8,
                                                          width,
                                                          height,
                                                          videoPool,
														  shortAlpha,
														  longAlpha,
														  diff);

    StickController * stickController = new StickController(grid_controller);

//    BallController * ballController = new BallController(grid_controller, video->_width, video->_height, 30);

    debug_logger->log("Creating new grid controller");

    //-----------------------------------------------------------------
    // Start the various threads
//    video->start();
    displayer->start();

    //-----------------------------------------------------------------
    // Create game controller

    GameController * game_controller =
        new GameController(toggle_record,
                           width,
                           height,
                           60,
                           videoPool,
                           stickController,
                           grid_controller);
    game_controller->start();

	//-----------------------------------------------------------------
	// Enter runtime logic
	runtime_logic(width,
	              height,
	              annotator,
	              videoPool,
	              grid_controller,
				  stickController,
				  game_controller);

    //-----------------------------------------------------------------
    // Cleanup Code
    frame_saver->stop();
    videoPool->destroy();
    annotationPool->destroy();

    // Stop the various threads (annotator last)
    displayer->stop();
//    video->stop();

    annotator->stop();
    game_controller->stop();
    //ballController->stop();
    //grid_controller->stop();


    // Stop logs
    debug_logger->stop();

        printf("Done.\n");
    return 0;
}


//*****************************************************************************
// * runtime_logic
//*****************************************************************************
void
runtime_logic
(
    int width,
    int height,
    FrameAnnotator * annotator,
    FramePool *      videoPool,
    GridController * grid_controller,
	StickController * stickController,
	GameController * game_controller
)
{
    // For the time being, just get an image from the videoPool and initialize
    // on a specific location there. In time, we should replace this logic with
    // a proper target registration routine.
    usleep(3*1000*1000);

    //-----------------------------------------------------------------
    // Get a Frame
    Frame *frame;
    if (videoPool->acquire(&frame, 0, true, true) < 0) {
        printf("ERROR: Could not acquire the initial Frame.\n");
        return;
    }

    //-----------------------------------------------------------------
    // Convert the frame to gray scale
    IplImage *gray = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
    IppiSize roi;
    roi.width = width;
    roi.height = height;
    ippiRGBToGray_8u_C3C1R((Ipp8u*)frame->_bgr->imageData,
                           frame->_bgr->width*3,
                           (Ipp8u*)gray->imageData,
                           gray->width*1,
                           roi);

    //-----------------------------------------------------------------
    // Create controller
    GridAnnotator * grid_annotator = new GridAnnotator(0, grid_controller, NULL, stickController);
    annotator->add(grid_annotator);

    GameAnnotator * game_annotator =
        new GameAnnotator(1, game_controller, grid_annotator);

    annotator->add(game_annotator);

    //-----------------------------------------------------------------
    // Release the frame
    videoPool->release(frame);
    annotator->start();     // DELAY STARTING THESE THINGS UNTIL NOW TO
    grid_controller->start();
//    ballController->start();
    stickController->start();

    // Just wait for the end
    while (end == 0)
        usleep(30*1000);
}


//*****************************************************************************
//* handle_stop
//*****************************************************************************
void
handle_stop()
{
    end = 1;
}

//*****************************************************************************
//* toggle_record
//*****************************************************************************
void
toggle_record()
{
    printf("Toggle Record called\n");
    if (frame_saver)
    {
        frame_saver->ToggleRecord();
    }
}


