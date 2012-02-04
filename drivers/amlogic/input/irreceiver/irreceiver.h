#ifndef IRRECEIVER_H
#define IRRECEIVER_H

#define MAX_PLUSE 1024

struct ir_window {
    unsigned int winNum;
    unsigned int winArray[MAX_PLUSE];
};

#define IRRECEIVER_IOC_SEND     0x5500
#define IRRECEIVER_IOC_RECV     0x5501
#define IRRECEIVER_IOC_STUDY_S  0x5502
#define IRRECEIVER_IOC_STUDY_E  0x5503


#endif //IRRECEIVER_H

