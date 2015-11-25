#ifndef DEBUG_LOGGER_PUBLIC
#define DEBUG_LOGGER_PUBLIC

#include "Public.h"
#include <string>

// MySQL headers
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/metadata.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/exception.h>
#include <cppconn/warning.h>

using std::string;

class FramePool;
class Frame;

//////////////////////////////////////////////////////////////////////////////////////////////////////////


class DebugLogger
{
private:
    sql::Driver * driver;

public:
    DebugLogger(void);
    ~DebugLogger(void);

    void stop(void);
    void start(void);

    void recordGameScoreSQL(int score);

    void log(string s);

    bool                 _end;
    std::vector<string>  _log_buffer;

    Frame *              _bg_buffer;
    Frame *              _fg_buffer;

    pthread_t            _logger_thread;
    pthread_mutex_t      _logger_mutex;
};


#endif // #define DEBUG_LOGGER_PUBLIC



