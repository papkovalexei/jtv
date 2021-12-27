#ifndef H_PARSE_FUNC
#define H_PARSE_FUNC

#include <iostream>
#include <vector>
#include <regex>
#include <memory>
#include <archive.h>
#include <archive_entry.h>

#define CHUNK 0x4000
#define NDX 1
#define PDT 2

int unzipped_data(const std::string &path_archive, const std::string &channel, 
                    std::vector<uint8_t> &pdt_data, std::vector<uint8_t> &ndx_data);
void cp1251utf8(char* str, char* res);
#endif
