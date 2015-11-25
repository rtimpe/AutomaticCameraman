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
{
    this->driver = get_driver_instance();
    pthread_mutex_init(&_logger_mutex, NULL);
}

DebugLogger::~DebugLogger
(
    void
)
{
    pthread_mutex_destroy(&_logger_mutex);
}

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


void DebugLogger::recordGameScoreSQL(int score) {
    pthread_mutex_lock(&_logger_mutex);

    try {
            sql::Connection * con = driver->connect("localhost", "root", "asdf");
   //        con -> setAutoCommit(0);
           con->setSchema("vatic");

           sql::PreparedStatement * pstmt = con->prepareStatement("INSERT INTO tracker(id, score) VALUES(?, ?)");
           pstmt->setString(1, get_current_datetime_string());
           pstmt->setInt(2, score);
           pstmt->executeUpdate();

           delete con;
           delete pstmt;

       } catch (sql::SQLException &e) {
         /*
           MySQL Connector/C++ throws three different exceptions:

           - sql::MethodNotImplementedException (derived from sql::SQLException)
           - sql::InvalidArgumentException (derived from sql::SQLException)
           - sql::SQLException (derived from std::runtime_error)
         */
         cout << "# ERR: SQLException in " << __FILE__;
         cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
         /* what() (derived from std::runtime_error) fetches error message */
         cout << "# ERR: " << e.what();
         cout << " (MySQL error code: " << e.getErrorCode();
         cout << ", SQLState: " << e.getSQLState() << " )" << endl;

       }

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
                log_file << *it << endl;
                cout << *it << endl;
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
