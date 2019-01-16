#include <stdlib.h>

/*
* Status for data response callback handler
*/
enum data_stream_status {
    INIT,
    RUN,
    END
};

/*
* Structure containing information information for data streaming 
*/
struct stream_info_t {
    struct MHD_Connection *connection;
    double (*sensor_func)();
    char buffer[MAX_VALUE_DIGITS];
    int value_size;
    int buffer_index;
    int time_span;
    struct timespec time_start;
    enum data_stream_status status;
};

void
stream_handler(struct stream_info_t *info);

int
init_data_stream(struct func_args_t *args, const char *sensor, int timespan);
