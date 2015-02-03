#include "DebugLogger.h"
#include "FramePool.h"
#include <pthread.h>
#include <sys/stat.h>
#include <sstream>

using std::cout;
using std::endl;
using std::stringstream;


static char LOG_FOLDER_NAME[] = "logs";


static void * logger_function(void * args);


DebugLogger::DebugLogger
(
    void
)
: _end(false), _log_buffer(0)
{}

void
DebugLogger::log
(
    string s
)
{
    stringstream ss;
    ss << get_current_datetime_string() << " : " << s;

    pthread_mutex_lock(&_logger_mutex);

    _log_buffer.push_back(ss.str());

    pthread_mutex_unlock(&_logger_mutex);
}


void
DebugLogger::start
(
    void
)
{
    _end = false;
    pthread_create(&_logger_thread, NULL, logger_function, this);
}


void
DebugLogger::stop
(
    void
)
{
    _end = true;
    pthread_join(_logger_thread, NULL);
}


static void *
logger_function
(
    void * args
)
{
    DebugLogger * dl = reinterpret_cast<DebugLogger *>(args);

    //----------------------------------------------------------------
    // Create directory for logs if we want to save them
    if (-1 == mkdir(LOG_FOLDER_NAME, S_IRWXU | S_IRWXG|  S_IRWXO)) {
        perror("Error creating logs directory");
    }

    //-----------------------------------------------------------------
    // Create log file
    stringstream ss;
    ss << LOG_FOLDER_NAME << "/log_" << get_current_datetime_string();

    ofstream log_file;
    log_file.open(ss.str().c_str());

    //-----------------------------------------------------------------
    // Loop until the program ends
    while (!dl->_end)
    {
        pthread_mutex_lock(&dl->_logger_mutex);
        if (!dl->_log_buffer.empty())
        {
            vector<string>::iterator end = dl->_log_buffer.end();
            for (vector<string>::iterator it = dl->_log_buffer.begin();
                 it != end; ++it)
            {
                log_file << *it;
                cout << *it;
            }
            dl->_log_buffer.clear();
        }
        pthread_mutex_unlock(&dl->_logger_mutex);

        // Sleep 100 milliseconds
        usleep(100000);
    }

    //-----------------------------------------------------------------
    // Flush remaining log messages to disk
    pthread_mutex_lock(&dl->_logger_mutex);
    vector<string>::iterator end = dl->_log_buffer.end();
    for (vector<string>::iterator it = dl->_log_buffer.begin();
         it != end; ++it)
    {
        log_file << *it;
        cout << *it;
    }
    dl->_log_buffer.clear();
    pthread_mutex_unlock(&dl->_logger_mutex);

    //-----------------------------------------------------------------
    // Close log file
    log_file.close();

    return NULL;
}
