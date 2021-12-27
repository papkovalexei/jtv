#include <clocale>
#include <ctime>
#include <endian.h>
#include <getopt.h>
#include <unistd.h>

#include "parse_func.hpp"

#define WINDOWS_TICK 10000000
#define SEC_TO_UNIX_EPOCH 11644473600LL
#define PROGRAMM_SIZE 12
#define NUMBER_ARGUMENTS 9
#define SIZE_DATE_FORMAT 8
#define MAX_NAME_SIZE 512

bool only_number(const std::string& str)
{
    for (auto it = str.cbegin(); it != str.cend(); it++) {
        if (((*it) < '0' || (*it) > '9') && (*it) != '-') {
            return false;
        }
    }
    return true;
}

time_t filetime_to_unix(long long windowsTicks)
{
    long long secs;
    time_t t;

    secs = (windowsTicks / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
    t = (time_t) secs;
    if (secs != (long long) t) {
        return (time_t) -1;
    }
    return t;
}

void print_usage()
{
    std::cout << "Usage: jtv [OPTION]" << std::endl
            << "All flags are required" << std::endl
            << "-f, --input-file <file>\tflag for specifying the input data table" << std::endl
            << "-d, --date <date>\tthe program date of interest" << std::endl
            << "\t<date> - YYYYMMDD" << std::endl
            << "-c, --channel <channel>\twhat TV channel" << std::endl << std::endl
            << "-t, --time-zone <number> - time zone UTC + number" << std::endl
            << "Example:" << std::endl << "\tjtv -f jtv.zip -d 20210607 -c CTC -t 3" << std::endl;
}


int main(int argc, char *argv[])
{
    if (argc != NUMBER_ARGUMENTS) {
        print_usage();
        return 1;
    }

    const char* short_options = "hc:d:f:t:";

    const option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"input-file", required_argument, NULL, 'f'},
        {"date", required_argument, NULL, 'd'},
        {"channel", required_argument, NULL, 'c'},
        {"time-zone", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}
    };

    int res;
    std::string file, date, name, time_zone;

    while ((res = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (res) {
        case 'h':
            print_usage();
            break;
        case 'f':
            file = optarg;
            break;
        case 'd':
            date = optarg;
            break;
        case 'c':
            name = optarg;
            break;
        case 't':
            time_zone = optarg;
            break;
        case '?':
        default:
            print_usage();
            return 1;
        }
    }
    // date YYYYMMDD

    if (date.size() != SIZE_DATE_FORMAT || !only_number(date)) {
        std::cerr << "Incorrect date format, see -h" << std::endl;
        return 1;
    }

    if (!only_number(time_zone)) {
        std::cerr << "Incorrect time zone format" << std::endl;
    }

    tm time_programm;

    time_programm.tm_year = atoi(date.substr(0, 4).c_str()) - 1900;
    time_programm.tm_mon = atoi(date.substr(4, 2).c_str()) - 1;
    time_programm.tm_mday = atoi(date.substr(6, 2).c_str());
    std::setlocale(LC_ALL, "");

    std::vector<uint8_t> pdt_data, ndx_data;
    std::vector<std::pair<time_t, int>> programm_in_pdt;

    if (unzipped_data(file, name, pdt_data, ndx_data) != 0) {
        return 1;
    }

    if (pdt_data.empty() || ndx_data.empty()) {
        std::cerr << "There is no schedule for this date or channel" << std::endl;
        return 1;
    }

    for (int i = 0; i < PROGRAMM_SIZE * le16toh(*reinterpret_cast<uint16_t *>(ndx_data.data())); i += 12) {
        time_t buf = filetime_to_unix(le64toh(*reinterpret_cast<uint64_t *>(ndx_data.data() + i + 4)));

        buf -= atoi(time_zone.c_str()) * 60 * 60;

        tm *time1 = localtime(&buf);
        tm time = *time1;

        if (time.tm_year == time_programm.tm_year
            && time.tm_mon == time_programm.tm_mon
            && time.tm_mday == time_programm.tm_mday) {
            if (programm_in_pdt.empty() && i != 0) {
                if (time.tm_hour != 0 || time.tm_min != 0) {
                    time_t prev_day = filetime_to_unix(le64toh(*reinterpret_cast<uint64_t *>(ndx_data.data() + i - 8)));
                    prev_day -= atoi(time_zone.c_str()) * 60 * 60;
                    programm_in_pdt.push_back(std::make_pair(prev_day, le16toh(*reinterpret_cast<uint16_t *>(ndx_data.data() + i))));
                }
            }
            programm_in_pdt.push_back(std::make_pair(buf, le16toh(*reinterpret_cast<uint16_t *>(ndx_data.data() + i + 12))));
        }
    }

    std::vector<char> buffer(MAX_NAME_SIZE), encode(MAX_NAME_SIZE);
    for (auto& programm : programm_in_pdt) {
        int length = le16toh(*reinterpret_cast<uint16_t *>(pdt_data.data() + programm.second));
        std::string title;

        for (int i = 0; i < length; i++) {
            title += (char)(*(pdt_data.data() + programm.second + i + 2));
        }

        if (title.size() + 1 > MAX_NAME_SIZE) {
            buffer.resize(title.size() + 1);
            encode.resize(title.size() + 1);
        }

        memcpy(buffer.data(), title.c_str(), title.size() + 1);
        cp1251utf8(buffer.data(), encode.data());
        title = encode.data();
        std::cout << ctime(&programm.first) << " " << title << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
