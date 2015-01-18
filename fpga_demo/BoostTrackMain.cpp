#include "Public.h"
#include "FramePool.h"
#include "FrameDisplayer.h"
#include "VideoImages.h"
#include "FrameAnnotator.h"
#include "TrackerController.h"
#include "TargetAnnotator.h"
#include "BaseTracker.h"
#include "BgSubTracker.h"
#include "DynamicsTracker.h"
#include "BoostedTracker.h"
#include "LibraryTracker.h"
#include <riffa.h>
#include <execinfo.h>
#include <signal.h>

int end = 0;
char fingersTrackerSer[] = "trained_fingers_up_5000.txt";

/*
char prefix[] = "../../milboosttracker-master/faceocc/imgs/";
char stem[] = "%simg%05d.png";
int startFrame = 0;
int endFrame = 885; 
int startCenterX = 175;
int startCenterY = 150;
int startDim = 70;
int frameW = 352;
int frameH = 288;
*/

/*
char prefix[] = "../../milboosttracker-master/david/imgs/";
char stem[] = "%simg%05d.png";
int startFrame = 1;
int endFrame = 462; 
int startCenterX = 157;
int startCenterY = 104;
int startDim = 70;
int frameW = 320;
int frameH = 240;
*/

/*
char prefix[] = "../../milboosttracker-master/coke/imgs/";
char stem[] = "%simg%05d.png";
int startFrame = 1;
int endFrame = 291; 
int startCenterX = 159;
int startCenterY = 98;
int startDim = 20; // with TRAIN_DIST 5
int frameW = 320;
int frameH = 240;
*/

/*
char prefix[] = "../../milboosttracker-master/faceocc2/imgs/";
char stem[] = "%simg%05d.png";
int startFrame = 8;
int endFrame = 819; 
int startCenterX = 160;
int startCenterY = 100;
int startDim = 70;
int frameW = 320;
int frameH = 240;
*/


char prefix[] = "../../milboosttracker-master/sylv/imgs/";
char stem[] = "%simg%05d.png";
int startFrame = 1;
int endFrame = 1344; 
int startCenterX = 146;
int startCenterY = 84;
int startDim = 50; // with TRAIN_THRESH 7
int frameW = 320;
int frameH = 240;


/*
char prefix[] = "../../milboosttracker-master/tiger2/imgs/";
char stem[] = "%simg%05d.png";
int startFrame = 0;
int endFrame = 364; 
int startCenterX = 33;
int startCenterY = 50;
int startDim = 45; // with TRAIN_THRESH 5
int frameW = 320;
int frameH = 240;
*/

/*
char prefix[] = "../framecap/caps_bg1/";
char stem[] = "%scap_%03d.png";
int startFrame = 50;
int endFrame = 232; 
int startCenterX = 745;
int startCenterY = 266;
int startDim = 65;
int frameW = 1280;
int frameH = 720;
*/

/*
char prefix[] = "../../milboosttracker-master/rgbimg/";
char stem[] = "%scap_%03d.png";
int startFrame = 47;
int endFrame = 525; 
int startCenterX = 408;
int startCenterY = 200;
int startDim = 55; //for rgbimg
int frameW = 640;
int frameH = 480;
*/

/*
char prefix[] = "../../milboosttracker-master/rgbimg2/";
char stem[] = "%scap_%03d.png";
int startFrame = 38;
int endFrame = 385; 
int startCenterX = 450;
int startCenterY = 250;
int startDim = 50; //for rgbimg2
int frameW = 640;
int frameH = 480;
*/

/*
char prefix[] = "../../milboosttracker-master/rgbimg3/";
char stem[] = "%scap_%03d.png";
int startFrame = 51;
int endFrame = 393;
int startCenterX = 484;
int startCenterY = 237;
int startDim = 55; //for rgbimg3
int frameW = 640;
int frameH = 480;
*/

/*
char prefix[] = "../../milboosttracker-master/rgbimg4/";
char stem[] = "%scap_%03d.png";
int startFrame = 37;
int endFrame = 640;
int startCenterX = 373;
int startCenterY = 107;
int startDim = 55; //for rgbimg4
int frameW = 640;
int frameH = 480;
*/

////////////////////////////////////////////////////////////////////////////////

//void handler(int sig) {
//  void *array[10];
//  size_t size;
//
//  // get void*'s for all entries on the stack
//  size = backtrace(array, 10);
//
//  // print out all the frames to stderr
//  fprintf(stderr, "Error: signal %d:\n", sig);
//  backtrace_symbols_fd(array, size, STDERR_FILENO);
//  exit(1);
//}



void handle_stop() {
	end = 1;
}


void runtime_track(TrackerController *controller, FrameAnnotator *annotator, 
	VideoImages *video, FramePool *videoPool, BgSub *bgSub) {
	// For the time being, just get an image from the videoPool and initialize
	// on a specific location there. In time, we should replace this logic with
	// a proper target registration routine.
	//usleep(3*1000*1000);

	// Get a Frame
	Frame *frame;
	if (videoPool->acquire(&frame, 0, true, true) < 0) {
        printf("ERROR: Could not acquire the initial Frame.\n");
        return;
    }

	// Convert the frame to gray scale
	IplImage *gray = cvCreateImage(cvSize(video->_width, video->_height), IPL_DEPTH_8U, 1);
	IppiSize roi;
	roi.width = video->_width;
	roi.height = video->_height;
	ippiRGBToGray_8u_C3C1R((Ipp8u*)frame->_bgr->imageData, frame->_bgr->width*3, 
		(Ipp8u*)gray->imageData, gray->width*1, roi);

	// For the time being, just use these coordinates for initialization
	int x = startCenterX;//1000;
	int y = startCenterY;//400;
	int dim = startDim;//45;

	// Create the MetaTrackers
	TargetAnnotator *targetAnnotator = new TargetAnnotator(1, controller);
	DynamicsTracker *dynamicsTracker = new DynamicsTracker(3);
	BgSubTracker *bgSubTracker = new BgSubTracker(4, bgSub);
	BoostedTracker *fingerTracker = new BoostedTracker(2, bgSub);
	LibraryTracker *libTracker = new LibraryTracker(5);
	LibraryTracker *libTracker2 = new LibraryTracker(6);
	BaseTracker *baseTracker = new BaseTracker(0);

	// Initialize and add the trackers
	if (!fingerTracker->init(x, y, 250, 50, 10, 5, 1.5, fingersTrackerSer)) {
	    printf("ERROR: Could not initialize the BoostedTracker.\n");
	    return;
	}
    if (!dynamicsTracker->init(x, y)) {
        printf("ERROR: Could not initialize the DynamicsTracker.\n");
        return;
    }
    if (!bgSubTracker->init(x, y, 35, 0.0000025)) {
        printf("ERROR: Could not initialize the BgSubTracker.\n");
        return;
    }
    if (!libTracker->init(x, y, dim, 250, 50, gray)) {
        printf("ERROR: Could not initialize the LibraryTracker.\n");
        return;
    }
    if (!libTracker2->init(x, y, dim, 250, 50, gray)) {
        printf("ERROR: Could not initialize the LibraryTracker.\n");
        return;
    }
	if (!baseTracker->init(x, y, dim, 250, 50, gray)) {
	    printf("ERROR: Could not initialize the BaseTracker.\n");
	    return;
	}

	// Release the frame
	videoPool->release(frame);

	// Add them
	//controller->add(bgSubTracker);
	//controller->add(fingerTracker);
	controller->add(baseTracker);
//	controller->add(libTracker);
//	controller->add(libTracker2);
	annotator->add(targetAnnotator);
	//annotator->add(bgSubTracker);
	//annotator->add(fingerTracker);
	annotator->add(baseTracker);
//	annotator->add(libTracker);
//	annotator->add(libTracker2);

	annotator->start();		// DELAY STARTING THESE THINGS UNTIL NOW TO 
	controller->start();	// ALLOW MORE ACCURATE TIMING

	// Just wait for the end
	while (end == 0)
		usleep(30*1000);
}


int main( int argc, const char** argv ) {
	char title[] = "Captured Frame";
	end = 0;

	//signal(SIGSEGV, handler);

	randinitalize(0);

	if (argc < 3) {
		printf("Usage: %s <capture fpga_id> <tracker fpga_id>]\n", argv[0]);
		return -1;
	}

	// Open the FPGA(s)
	int fpgaIdCapture = atoi(argv[1]);
	int fpgaIdTracker = atoi(argv[2]);
	fpga_t *fpgaCapture;
	fpga_t *fpgaTracker;
	fpgaTracker = fpga_open(fpgaIdTracker);
	if (fpgaIdCapture != fpgaIdTracker)
		fpgaCapture = fpga_open(fpgaIdCapture);
	else
		fpgaCapture = fpgaTracker;
    if (fpgaTracker == NULL) {
        printf("ERROR: Could not open FPGA %d.\n", fpgaIdTracker);
        return -1;
    }
    if (fpgaCapture == NULL) {
        printf("ERROR: Could not open FPGA %d.\n", fpgaIdCapture);
        return -1;
    }

	// Create the FramePools
	FramePool *videoPool = new FramePool(8);
	FramePool *annotationPool = new FramePool(3);

	// Create the TrackerController
	TrackerController *controller = new TrackerController(fpgaTracker, 0, 500, videoPool);
    if (!controller->init()) {
        printf("ERROR: Could not initialize the TrackerController.\n");
        return -1;
    }

	// Create the VideoImages source
	//VideoImages *video = new DiskImages(prefix, stem, startFrame, endFrame, frameW, frameH, 30, videoPool);
	VideoImages *video = new LiveImages(fpgaCapture, 0, 7000, 1280, 720, 120, videoPool);
    if (!video->init()) {
        printf("ERROR: Could not initialize the VideoImages.\n");
        return -1;
    }

	// Create the FrameAnnotator
	FrameAnnotator *annotator = new FrameAnnotator(video->_width, video->_height, 
		controller, annotationPool);
    if (!annotator->init()) {
        printf("ERROR: Could not initialize the FrameAnnotator.\n");
        return -1;
    }
	
	// Create the FrameDisplayer
	FrameDisplayer *displayer = new FrameDisplayer(title, video->_width, video->_height, 
		videoPool, annotationPool, handle_stop);
    if (!displayer->init()) {
        printf("ERROR: Could not initialize the FrameDisplayer.\n");
        return -1;
    }

	// Create the BgSub
	BgSub *bgSub = new BgSub(video->_width, video->_height, video->_fps);
    if (!bgSub->init()) {
        printf("ERROR: Could not initialize the BgSub.\n");
        exit(1);
    }

	// Start the various threads
	video->start();
//	annotator->start();		// DON'T START NOW, WILL START IN runtime_track
	displayer->start();
	bgSub->start();
//	controller->start();	// TO GET MORE ACCURATE TIMING.

	// Enter runtime logic
	runtime_track(controller, annotator, video, videoPool, bgSub);

	// Close the FramePools
	videoPool->destroy();
	annotationPool->destroy();

	// Stop the various threads (annotator last)
	displayer->stop();
	video->stop();
	controller->stop();
	bgSub->stop();
	annotator->stop();

	// Close the FPGA(s)
	fpga_close(fpgaTracker);
	if (fpgaIdCapture != fpgaIdTracker)
		fpga_close(fpgaCapture);

	return 0;
}


