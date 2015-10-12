#if 0 // COMMENTED_OUT_FOR_MAC_BUILD

#include "FpgaTracker.h"
#include "timer.h"
#include <string.h>
#include <algorithm>
#include <pthread.h>


TS_INIT(fpgatracker, 4);
TC_INIT(fpgatracker, 2);


inline int build_send_buf(unsigned int *sendBuf, IplImage *img, Tracker *tr[], 
	int numTrackers, int train, int numFeats, int numSelFeats, bool print) {
	int numSent = 0;
	int offset = 0;
	int regionSize;
	int cropSize;
	int * intPtr;
	char * charPtr;
	float * floatPtr;

	sendBuf[offset++] = numTrackers; // Num trackers
	sendBuf[offset++] = train; // Train (0/1)
	sendBuf[offset++] = numFeats; // Num feats
	sendBuf[offset++] = numSelFeats; // Num sel feats

	for (int t=0; t < numTrackers; t++) {
		// Skip training if necessary
		if (train && tr[t]->_train == 0)
			continue;
		else
			numSent++;

		if (train) // Need 1.2*searchrad for training
			regionSize = tr[t]->_w + (1.2*tr[t]->_trparams->_srchwinsz)*2;
		else 
			regionSize = tr[t]->_w + (tr[t]->_trparams->_srchwinsz*2);
		// The regionSize is the dimension of a square of pixels the FPGA tracker needs 
		// after resizing. It should = width + 2*radius. It should also be odd so 
		// that there will be a center pixel.
		regionSize += (regionSize % 2 ? 0 : 1);
		// The cropSize is the dimension we need to crop and send. The cropped pixels
		// will be scaled on the FPGA according to the scale factor, so this should 
		// be the multiplicative inverse.
		cropSize = (regionSize*tr[t]->_scale);

		sendBuf[offset++] = cropSize*cropSize; // Num pixels (crop size)x(crop size)
		sendBuf[offset++] = 0;
		floatPtr = (float *)(sendBuf+offset);
		floatPtr[0] = tr[t]->_clfparams->_lRate;// Lrn rate
		floatPtr[1] = 1-tr[t]->_clfparams->_lRate; // 1 - lrn rate
		offset++;
		offset++;

		// Feature parameters
		floatPtr = (float *)(sendBuf+offset);
		if (train) {
			for (int i=0; i < numFeats; i++) { 
				floatPtr[i*4 + 0] = tr[t]->_clf->_weakclf[i]->_mu1; 
				floatPtr[i*4 + 1] = tr[t]->_clf->_weakclf[i]->_sig1; 
				floatPtr[i*4 + 2] = tr[t]->_clf->_weakclf[i]->_e1; 
				offset += 4;
			}
		}
		else {
			for (int i=0; i < numFeats; i++) { 
				floatPtr[i*4 + 0] = tr[t]->_clf->_weakclf[tr[t]->_clf->_selectors[i]]->_mu1; 
				floatPtr[i*4 + 1] = tr[t]->_clf->_weakclf[tr[t]->_clf->_selectors[i]]->_sig1; 
				floatPtr[i*4 + 2] = tr[t]->_clf->_weakclf[tr[t]->_clf->_selectors[i]]->_e1; 
				offset += 4;
			}
		}

		// Scale parameters
		sendBuf[offset++] = cropSize; 
		sendBuf[offset++] = regionSize; 
		sendBuf[offset++] = (0x4000*cropSize)/regionSize; 
		sendBuf[offset++] = 0; 

		// Quadrant weights
		floatPtr = (float *)(sendBuf+offset);
		for (int i=0; i < 9; i++)
			floatPtr[i] = tr[0]->_trparams->_weights[i]; 
		offset += 12; 

		// Radius parameters
		if (train) {
			sendBuf[offset++] = tr[t]->_trparams->_posradtrain*tr[t]->_trparams->_posradtrain; 
			sendBuf[offset++] = (1.2*tr[t]->_trparams->_srchwinsz)*(1.2*tr[t]->_trparams->_srchwinsz); 
			sendBuf[offset++] = (tr[t]->_trparams->_posradtrain*2)*(tr[t]->_trparams->_posradtrain*2); 
			sendBuf[offset++] = (int)(2.5*(sendBuf[offset-2] - sendBuf[offset-1])/tr[t]->_trparams->_negnumtrain);
		}
		else {
			sendBuf[offset++] = tr[t]->_trparams->_srchwinsz*tr[t]->_trparams->_srchwinsz; 
			sendBuf[offset++] = 0; 
			sendBuf[offset++] = 0; 
			sendBuf[offset++] = 0; 
		}

		// Haar parameters
		Ftr *ftr;
		int tlx, tly, brx, bry, wt;
		intPtr = (int *)sendBuf;
		for (int i=0; i < numFeats; i++) { 
			if (train)
				ftr = tr[t]->_clf->_weakclf[i]->_ftr;
			else
				ftr = tr[t]->_clf->_weakclf[tr[t]->_clf->_selectors[i]]->_ftr;
			for (int j=0; j<3; j++) {
				if (j < ftr->_rects.size()) {
					tlx = ftr->_rects[j].x;
					tly = ftr->_rects[j].y;
					brx = ftr->_rects[j].x + ftr->_rects[j].width;
					bry = ftr->_rects[j].y + ftr->_rects[j].height;
 					wt = (int)ftr->_weights[j];
					intPtr[offset++] = (tlx | ((tly & 0x3F)<<6) | ((brx & 0x3F)<<12) | ((bry & 0x3F)<<18) | ((wt & 0xF)<<24)); 
				}
				else {
					intPtr[offset++] = 0;
				}
			}
			intPtr[offset++] = 0; 
			for (int j=3; j<6; j++) {
				if (j < ftr->_rects.size()) {
					tlx = ftr->_rects[j].x;
					tly = ftr->_rects[j].y;
					brx = ftr->_rects[j].x + ftr->_rects[j].width;
					bry = ftr->_rects[j].y + ftr->_rects[j].height;
 					wt = (int)ftr->_weights[j];
					intPtr[offset++] = (tlx | ((tly & 0x3F)<<6) | ((brx & 0x3F)<<12) | ((bry & 0x3F)<<18) | ((wt & 0xF)<<24)); 
				}
				else {
					intPtr[offset++] = 0;
				}
			}
			intPtr[offset++] = 0; 
		}

		// Template squared sum and template in padded column major format
		intPtr[offset] = tr[t]->_trparams->_tplSqSum;
		offset += 4;
		charPtr = (char *)(sendBuf + offset);
		memcpy(charPtr, tr[t]->_trparams->_tplCol, TRACKING_WIN_DIM*(((TRACKING_WIN_DIM+15)/16)*16));
		offset += (TRACKING_WIN_DIM*(((TRACKING_WIN_DIM+15)/16)*16))/4;

		// Pixel data (needs to be odd sized square with extra value on top and left)
		charPtr = (char *)(sendBuf + offset);
		memset(charPtr, 0, ((cropSize*cropSize + 15)/16)*16);
		offset += ((cropSize*cropSize + 15)/16)*4;
		int x = tr[t]->_x*tr[t]->_scale - 1;
		int y = tr[t]->_y*tr[t]->_scale - 1;
		int rad = (train ? 1.2*tr[t]->_trparams->_srchwinsz : tr[t]->_trparams->_srchwinsz)*tr[t]->_scale;
		int minrow = max(0, y - rad);
		int maxrow = min(img->height, y - rad + cropSize);
		int mincol = max(0, x - rad);
		int maxcol = min(img->width, x - rad + cropSize);
		int startrow = max(0, 0 - (y - rad));
		int startcol = max(0, 0 - (x - rad));
		for (int j=minrow, y=startrow; j<maxrow; j++, y++) {
			for (int i=mincol, x=startcol; i<maxcol; i++, x++) {
				charPtr[y*cropSize + x] = img->imageData[j*img->widthStep+i];
			}
		}		
	}

	// Update the amount of tracker data actually sent.
	sendBuf[0] = numSent; // Num trackers

	// DEBUGGING
//	printf("numSent: %d (%d)\n", numSent, train);
	if (print) {
		printf("len in words:%d\n", offset);
		for (int i = 0; i < offset; i=i+4)
			printf("%d : CHNL_RX_DATA = {32'h%08x, 32'h%08x, 32'h%08x, 32'h%08x};\n", 
				i/4, sendBuf[i+3], sendBuf[i+2], sendBuf[i+1], sendBuf[i]);
	}
	return offset;
}



void * sender_fxn(void *arg) {
	FpgaTracker *ft = (FpgaTracker *)arg;
	FpgaTrackerParams *fpgaParams;
	int sendingWords;
	int sentWords;

	// Loop until end
	for (int cnt = 0; 1; cnt++) {
TS_STAMP(fpgatracker, 0);
		// Get image, will not be resized.
		if (!ft->_getParamsFxn(&fpgaParams))
			break;

		// Wait until training response is received
		pthread_mutex_lock(&ft->_mutex);
		while (ft->_start == 0 && ft->_end == 0) 
			pthread_cond_wait(&ft->_cv_send, &ft->_mutex);
		ft->_start = 0;
		ft->_fpgaParams = fpgaParams; // Save the new FpgaParameters
		pthread_cond_signal(&ft->_cv_recv);
		if (ft->_end == 1) {
			pthread_mutex_unlock(&ft->_mutex);
			break;
		}
		pthread_mutex_unlock(&ft->_mutex);

		// Build tracking data
		sendingWords = build_send_buf(ft->_sendBuf, ft->_fpgaParams->_grayIpl, 
			ft->_fpgaParams->_tr, ft->_fpgaParams->_numTrackers, 0, 
			ft->_fpgaParams->_numSelFeats, ft->_fpgaParams->_numSelFeats, false);

		// Start sending the request for classification
		if (sendingWords > 4) { // Only send if there's at least one to classify
			fpga_reset(ft->_fpga);
			sentWords = fpga_send(ft->_fpga, ft->_chnl, ft->_sendBuf, sendingWords, 
				0, 1, ft->_timeoutMs);

//			while (sentWords != sendingWords) {
//				printf("%d) Sent %d words (detect), expecting %d. Retrying.\n", 
//					cnt, sentWords, sendingWords);
//				usleep(50000);
//				fpga_reset(ft->_fpga);
//				sentWords = fpga_send(ft->_fpga, ft->_chnl, ft->_sendBuf, sendingWords, 
//				0, 1, ft->_timeoutMs);
//			}

			if (sentWords != sendingWords) {
				printf("%d) Sent %d words (detect), expecting %d. Exiting.\n", 
					cnt, sentWords, sendingWords);
				break;
			}
		}

		// Wait until classification response is received
		pthread_mutex_lock(&ft->_mutex);
		while (ft->_start == 0 && ft->_end == 0) 
			pthread_cond_wait(&ft->_cv_send, &ft->_mutex);
		ft->_start = 0;
		pthread_cond_signal(&ft->_cv_recv);
		if (ft->_end == 1) {
			pthread_mutex_unlock(&ft->_mutex);
			break;
		}
		pthread_mutex_unlock(&ft->_mutex);

		// Build training data
		sendingWords = build_send_buf(ft->_sendBuf, ft->_fpgaParams->_grayIpl, 
			ft->_fpgaParams->_tr, ft->_fpgaParams->_numTrackers, 1, 
			ft->_fpgaParams->_numFeats, ft->_fpgaParams->_numSelFeats, false);

		// Start sending the request for training
		if (sendingWords > 4) { // Only send if there's at least one to train
			sentWords = fpga_send(ft->_fpga, ft->_chnl, ft->_sendBuf, sendingWords, 
				0, 1, ft->_timeoutMs);

//			while (sentWords != sendingWords) {
//				printf("%d) Sent %d words (train), expecting %d. Retrying.\n", 
//					cnt, sentWords, sendingWords);
//				usleep(50000);
//				fpga_reset(ft->_fpga);
//				sentWords = fpga_send(ft->_fpga, ft->_chnl, ft->_sendBuf, sendingWords, 
//					0, 1, ft->_timeoutMs);
//			}

			if (sentWords != sendingWords) {
				printf("%d) Sent %d words (train), expecting %d. Exiting.\n", 
					cnt, sentWords, sendingWords);
				break;
			}
		}
TS_STAMP(fpgatracker, 1);
TC_ACCRUE(fpgatracker, 0, 1, 0);
	}
printf("fpgatracker: %f (%ld)\n", TC_ITER_AVG(fpgatracker, 0), TC_ITERS(fpgatracker, 0));

	// To release other thread (if blocking)
	pthread_mutex_lock(&ft->_mutex);
	ft->_end = 1;
	pthread_cond_signal(&ft->_cv_recv);
	pthread_mutex_unlock(&ft->_mutex);
}


void * receiver_fxn(void *arg) {
	FpgaTracker *ft = (FpgaTracker *)arg;
	unsigned char * charPtr;
	float * floatPtr;
	float mean, sqMean;
	int recvdWords;
	int pol, shift;
	long score;
	int predLoc;
	float ncc;
	int numTrained;
	int offset;
	int expectedWords;

	// Track
	for (int cnt = 0; 1; cnt++) {
		// Start send thread, wait until it starts sending request for classification.
		pthread_mutex_lock(&ft->_mutex);
		ft->_start = 1;
		pthread_cond_signal(&ft->_cv_send);
		while (ft->_start != 0 && ft->_end == 0) 
			pthread_cond_wait(&ft->_cv_recv, &ft->_mutex);
		if (ft->_end == 1) {
			pthread_mutex_unlock(&ft->_mutex);
			break;
		}
		pthread_mutex_unlock(&ft->_mutex);

		// Receive the response
		expectedWords = ft->_fpgaParams->_numTrackers*4*(NUM_PRED*2+1);
		if (expectedWords > 0) { // Only expect a response if one was sent
			recvdWords = fpga_recv(ft->_fpga, ft->_chnl, ft->_recvBuf, expectedWords, ft->_timeoutMs);

//			while (recvdWords != expectedWords) {
//				printf("%d) Received %d words (detect), expecting %d. Retrying.\n", 
//					cnt, recvdWords, expectedWords);
//				usleep(50000);
//				recvdWords = fpga_recv(ft->_fpga, ft->_chnl, ft->_recvBuf, expectedWords, ft->_timeoutMs);
//			}

			if (recvdWords != expectedWords) {
				printf("%d) Received %d words (detect), expecting %d. Exiting.\n", 
					cnt, recvdWords, expectedWords);
				break;
			}
		}

		// Update the tracking locations. Format is 128 bits (4 ints) per location.
		for (int i=0; i < ft->_fpgaParams->_numTrackers; i++) {
			// Get the means
			sqMean = *((float *)(&ft->_recvBuf[(i*4*(NUM_PRED*2+1))+3]));
			mean = *((float *)(&ft->_recvBuf[(i*4*(NUM_PRED*2+1))+2]));
			ft->_fpgaParams->_tr[i]->_predVar = sqMean - mean*mean;
			ft->_fpgaParams->_tr[i]->_predMean = mean;
			// Get the top NUM_PRED predictions (in ascending order)
			// {signed 64 bit value, 32 bit 0, 1 bit neg, 15 bit y dist sqrd, 1 bit neg, 15 bit x dist sqrd}
			// Signed 64 bit value is 40.24 fixed point.
			for (int j=0; j < NUM_PRED; j++) {
				// Get the score
				score = ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*j)+3];
				score = score<<32 | ((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*j)+2]) & 0xFFFFFFFF);
				ft->_fpgaParams->_tr[i]->_predScore[j] = score;

				// Get the x direction shift
				pol = 1 - 2*((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*j)]>>15) & 0x1);
				shift = pol * sqrt((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*j)]>>00) & 0x7FFF);
				predLoc = ft->_fpgaParams->_tr[i]->_x + shift;
				// Make sure it doesn't shift out of frame
				if (predLoc < 0)
					predLoc = 0;
				else if (predLoc >= ft->_fpgaParams->_grayIpl->width/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_w)
					predLoc = ft->_fpgaParams->_grayIpl->width/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_w - 1;
				ft->_fpgaParams->_tr[i]->_predX[j] = predLoc;

				// Get the y direction shift
				pol = 1 - 2*((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*j)]>>31) & 0x1);
				shift = pol * sqrt((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*j)]>>16) & 0x7FFF);
				predLoc = ft->_fpgaParams->_tr[i]->_y + shift;
				// Make sure it wont't shift out of frame
				if (predLoc < 0)
					predLoc = 0;
				else if (predLoc >= ft->_fpgaParams->_grayIpl->height/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_h)
					predLoc = ft->_fpgaParams->_grayIpl->height/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_h - 1;
				ft->_fpgaParams->_tr[i]->_predY[j] = predLoc;
			}

			// Get the top NUM_PRED ncc values (in ascending order)
			// {32 bit 0, 32 bit ncc float val, 1 bit neg, 15 bit y dist sqrd, 1 bit neg, 15 bit x dist sqrd}
			for (int j=0; j < NUM_PRED; j++) {
				// Get the score
				ncc = *((float *)&(ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*(j+NUM_PRED))+2]));
				ft->_fpgaParams->_tr[i]->_nccScore[j] = ncc;

				// Get the x direction shift
				pol = 1 - 2*((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*(j+NUM_PRED))]>>15) & 0x1);
				shift = pol * sqrt((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*(j+NUM_PRED))]>>00) & 0x7FFF);
				predLoc = ft->_fpgaParams->_tr[i]->_x + shift - 1; // Minus 1 b/c of zero padding
				// Make sure it doesn't shift out of frame
				if (predLoc < 0)
					predLoc = 0;
				else if (predLoc >= ft->_fpgaParams->_grayIpl->width/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_w)
					predLoc = ft->_fpgaParams->_grayIpl->width/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_w - 1;
				ft->_fpgaParams->_tr[i]->_nccX[j] = predLoc;

				// Get the y direction shift
				pol = 1 - 2*((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*(j+NUM_PRED))]>>31) & 0x1);
				shift = pol * sqrt((ft->_recvBuf[(i*4*(NUM_PRED*2+1))+(4*1)+(4*(j+NUM_PRED))]>>16) & 0x7FFF);
				predLoc = ft->_fpgaParams->_tr[i]->_y + shift - 1; // Minus 1 b/c of zero padding
				// Make sure it wont't shift out of frame
				if (predLoc < 0)
					predLoc = 0;
				else if (predLoc >= ft->_fpgaParams->_grayIpl->height/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_h)
					predLoc = ft->_fpgaParams->_grayIpl->height/ft->_fpgaParams->_tr[i]->_scale - ft->_fpgaParams->_tr[i]->_h - 1;
				ft->_fpgaParams->_tr[i]->_nccY[j] = predLoc;
			}
		}

		// Adjust the positions
		ft->_adjustLocFxn(ft->_fpgaParams);

		// Start send thread, wait until it starts sending request for training.
		pthread_mutex_lock(&ft->_mutex);
		ft->_start = 1;
		pthread_cond_signal(&ft->_cv_send);
		while (ft->_start != 0 && ft->_end == 0) 
			pthread_cond_wait(&ft->_cv_recv, &ft->_mutex);
		if (ft->_end == 1) {
			pthread_mutex_unlock(&ft->_mutex);
			break;
		}
		pthread_mutex_unlock(&ft->_mutex);

		// Receive the response
		numTrained = 0;
		for (int i=0; i < ft->_fpgaParams->_numTrackers; i++)
			numTrained += ft->_fpgaParams->_tr[i]->_train;
		expectedWords = numTrained*(((ft->_fpgaParams->_numFeats+1)/2)*4 + ((ft->_fpgaParams->_numSelFeats+15)/16)*4);
		if (expectedWords > 0) { // Only expect a response if one was sent
			recvdWords = fpga_recv(ft->_fpga, ft->_chnl, ft->_recvBuf, expectedWords, ft->_timeoutMs);

//			while (recvdWords != expectedWords) {
//				printf("%d) Received %d words (train), expecting %d. Retrying.\n", 
//					cnt, recvdWords, expectedWords);
//				usleep(50000);
//				recvdWords = fpga_recv(ft->_fpga, ft->_chnl, ft->_recvBuf, expectedWords, ft->_timeoutMs);
//			}

			if (recvdWords != expectedWords) {
				printf("%d) Received %d words (train), expecting %d. Exiting.\n", 
					cnt, recvdWords, expectedWords);
				break;
			}

			// Update the classifiers
			floatPtr = (float *)ft->_recvBuf;
			for (int i=0, t=0; i < ft->_fpgaParams->_numTrackers; i++) {
				// Skip those that were not trained
				if (ft->_fpgaParams->_tr[i]->_train == 0)
					continue;

				// Update feature parameters
				offset = t*(((ft->_fpgaParams->_numFeats+1)/2)*4 + ((ft->_fpgaParams->_numSelFeats+15)/16)*4);
				for (int j=0; j < ft->_fpgaParams->_numFeats; j++) {
					ft->_fpgaParams->_tr[i]->_clf->_weakclf[j]->_mu1 = floatPtr[offset++];
					ft->_fpgaParams->_tr[i]->_clf->_weakclf[j]->_sig1 = floatPtr[offset++];
					ft->_fpgaParams->_tr[i]->_clf->_weakclf[j]->_e1 = -1.0f/(2.0f*ft->_fpgaParams->_tr[i]->_clf->_weakclf[j]->_sig1+1e-99);
				}

				// Update selected features in classifier
				offset = t*(((ft->_fpgaParams->_numFeats+1)/2)*4 + ((ft->_fpgaParams->_numSelFeats+15)/16)*4) + ((ft->_fpgaParams->_numFeats+1)/2)*4;
				charPtr = (unsigned char *)(ft->_recvBuf + offset);
				ft->_fpgaParams->_tr[i]->_clf->_selectors.clear();
				for (int j=0; j < ft->_fpgaParams->_numSelFeats; j++)
					ft->_fpgaParams->_tr[i]->_clf->_selectors.push_back(charPtr[j]);

				// Increment trained count
				t++;
			}
		}
	}

	// To release other thread (if blocking)
	pthread_mutex_lock(&ft->_mutex);
	ft->_end = 1;
	pthread_cond_signal(&ft->_cv_send);
	pthread_mutex_unlock(&ft->_mutex);
}


FpgaTracker::FpgaTracker(fpga_t *fpga, int chnl, long timeout, 
	bool (*getParamsFxn)(FpgaTrackerParams **), 
	void (*adjustLocFxn)(FpgaTrackerParams *)) {
	// Save
	_getParamsFxn = getParamsFxn;
	_adjustLocFxn = adjustLocFxn;
	_timeoutMs = timeout;
	_chnl = chnl;
	_fpga = fpga;
}


bool FpgaTracker::init() {
	// Validate FPGA
    if(_fpga == NULL) {
        printf("ERROR: Invalid FPGA handle.\n");
        return false;
    }

	// Validate getParamsFxn
    if(_getParamsFxn == NULL) {
        printf("ERROR: Invalid getParamsFxn pointer.\n");
        return false;
    }

	// Create the condition variables and mutex
	pthread_mutex_init(&_mutex, NULL);
	pthread_cond_init (&_cv_send, NULL);
	pthread_cond_init (&_cv_recv, NULL);

	// Allocate buffers
	int tplPixels = TRACKING_WIN_DIM*(((TRACKING_WIN_DIM+15)/16)*16);
	int maxNumPixels = ((( (1080+(1.2f*MAX_SEARCH_RADIUS))*(1080+(1.2f*MAX_SEARCH_RADIUS)) )+15)/16)*16;
	int sz = 4 + MAX_NUM_TRACKERS*(4 + MAX_NUM_FEATS*4 + 4 + 3*4 + 4 + MAX_NUM_FEATS*8 + 4 + tplPixels/4 + (maxNumPixels+3)/4);
	_sendBuf = (unsigned int *)malloc(sz*sizeof(unsigned int));
    if (_sendBuf == NULL) {
        printf("ERROR: Unable to malloc send buffer (%lu bytes).\n", sz*sizeof(unsigned int));
        return false;
    }
	memset(_sendBuf, 0, sz*sizeof(unsigned int));

	sz = MAX_NUM_TRACKERS*(((MAX_NUM_FEATS+1)/2)*4 + ((MAX_NUM_SEL_FEATS+15)/16)*4);
	_recvBuf = (unsigned int *)malloc(sz*sizeof(unsigned int));
    if (_recvBuf == NULL) {
        printf("ERROR: Unable to malloc receive buffer (%lu bytes).\n", sz*sizeof(unsigned int));
        return false;
    }
	memset(_recvBuf, 0, sz*sizeof(unsigned int));

	return true;
}


void FpgaTracker::start() {
	// Initialize
	_end = 0;
	_start = 0;

	// Reset the FPGA
	fpga_reset(_fpga);

	// Start tracking threads
    pthread_create(&_threadSend, NULL, sender_fxn, this);
    pthread_create(&_threadRecv, NULL, receiver_fxn, this);
}


void FpgaTracker::stop() {
	// End threads (if not already signaled)
	pthread_mutex_lock(&_mutex);
	_end = 1;
	pthread_cond_signal(&_cv_send);
	pthread_cond_signal(&_cv_recv);
	pthread_mutex_unlock(&_mutex);

	// Wait for threads to complete
    pthread_join(_threadSend, NULL);
    pthread_join(_threadRecv, NULL);

	// Clean up
	free(_sendBuf);
	free(_recvBuf);
	pthread_mutex_destroy(&_mutex);
	pthread_cond_destroy(&_cv_send);
	pthread_cond_destroy(&_cv_recv);
}

#endif // #if 0 // COMMENTED_OUT_FOR_MAC_BUILD
