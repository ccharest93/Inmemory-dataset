#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <curl/curl.h>
#include <unistd.h>
#include <iostream>
#include "zlib.h"
#include "zconf.h"
// #include "tarlib/tarlib.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <libgen.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <tarlib/tarlib.h>
std::string s;
z_stream stream3;
struct archive *a;
struct archive *ext;
struct archive_entry *entry;
tar_stream stream;
tar_header header;
int file_handle = -1;
char out[1000000];
char out2[100000];
int flag_filename;
char target[100];
void one_chunk(tar_stream &stream, tar_header &header, int &file_handle, const Byte *data, size_t size)
{
    stream.next_in = data;
    stream.avail_in = size;

    int result = TAR_OK;

    while (stream.avail_in != 0)
    {
        // Something to do
        do
        {
            if (file_handle == -1 && header.done && !tar_headerIsEmpty(&header))
            {

                if (tar_headerIsDir(&header) == TAR_TRUE)
                {
                    if (mkdir(header.file_name, S_IRWXU) == -1)
                    {
                        printf("e%s%d\n", header.file_name, errno);
                        assert(errno == EEXIST);
                    }
                }
                else
                {
                    // Case 0: "ILSVRC2012_val_00049946_n04317175.JPEG"  --> "val/n04317175/ILSVRC2012_val_00049946.JPEG"
                    // Case 1: "n01498041_2538_n01498041.JPEG" --> "train/n01498041/n01498041_2538.JPEG"
                    // Case 2: "ILSVRC2012_test_00063706.JPEG" --> "test/ILSVRC2012_test_00063706.JPEG"
                    switch (flag_filename)
                    {
                    // python version of case 0:
                    // pieces = [file[:-15] + '.JPEG', file[-14:-5]]
                    // try:
                    //     os.mkdir(path + pieces[1])
                    // except:
                    //     pass
                    // os.rename(path + file, path + pieces[1] + '/' + pieces[0])
                    case 0:
                    {
                        char *file = header.file_name;
                        char path[] = "/";
                        std::string pieces[2];
                        char *piece;
                        int i = 0;
                        s = std::string(file);
                        pieces[0] = s.substr(0, s.length() - 15) + ".JPEG";
                        pieces[1] = s.substr(s.length() - 14, 9) + "/";
                        s = std::string(target) + std::string(path) + pieces[1] + pieces[0];
                        std::string dir = std::string(target) + std::string(path) + pieces[1];
                        strcpy(header.file_name, s.c_str());
                        //printf("new_file: %s\n", s.c_str());
                        try
                        {
                            mkdir(dir.c_str(), S_IRWXU);
                        }
                        catch (...)
                        {
                            //printf("e%s%d\n", pieces[1].c_str(), errno);
                            assert(errno == EEXIST);
                        }
                        try
                        {
                            //printf("Creating %s\n", header.file_name);
                            file_handle = creat(header.file_name, S_IRUSR | S_IWUSR);
                            assert(file_handle != -1);
                        }
                        catch (...)
                        {
                            //printf("e%s%d\n", header.file_name, errno);
                            assert(errno == EEXIST);
                        }
                        break;
                    }
                    case 1:
                    {
                        char *file = header.file_name;
                        char path[] = "/";
                        std::string pieces[2];
                        char *piece;
                        int i = 0;
                        s = std::string(file);
                        pieces[0] = s.substr(0, s.length() - 15) + ".JPEG";
                        pieces[1] = s.substr(0, 9) + "/";
                        s = std::string(target) +std::string(path) + pieces[1] + pieces[0];
                        std::string dir = std::string(target) +std::string(path) + pieces[1];
                        strcpy(header.file_name, s.c_str());
                        //printf("new_file: %s\n", s.c_str());
                        try
                        {
                            mkdir(dir.c_str(), S_IRWXU);
                        }
                        catch (...)
                        {
                            //printf("e%s%d\n", pieces[1].c_str(), errno);
                            assert(errno == EEXIST);
                        }
                        try
                        {
                            //printf("Creating %s\n", header.file_name);
                            file_handle = creat(header.file_name, S_IRUSR | S_IWUSR);
                            assert(file_handle != -1);
                        }
                        catch (...)
                        {
                            //printf("e%s%d\n", header.file_name, errno);
                            assert(errno == EEXIST);
                        }
                        break;
                    }
                    case 2:
                    {
                        char *file = header.file_name;
                        char path[] = "/";
                        s = std::string(target) +std::string(path) + std::string(file);
                        strcpy(header.file_name, s.c_str());
                        //printf("new_file: %s\n", s.c_str());
                        try
                        {
                            //printf("Creating %s\n", header.file_name);
                            file_handle = creat(header.file_name, S_IRUSR | S_IWUSR);
                            assert(file_handle != -1);
                        }
                        catch (...)
                        {
                            //printf("e%s%d\n", header.file_name, errno);
                            assert(errno == EEXIST);
                        }
                        break;
                    }
                    }
                    //printf("Creating %s\n", header.file_name);
                }
            }

            result = tar_inflate(&stream, TAR_HEADER_FLUSH);
            assert(result >= TAR_OK);

            if (file_handle != -1 && stream.len_out)
            {
                int written = write(file_handle, stream.ptr_out, stream.len_out);
                assert(written == stream.len_out);
            }

        } while (result == TAR_OK && stream.avail_in != 0);

        if (file_handle != -1 && result == TAR_ENTRY_END)
        {
            //printf("Closing %s\n", header.file_name);
            assert(close(file_handle) == 0);
            file_handle = -1;
        }
    }
}
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // within this we want to inflate the data
    stream3.avail_in = nmemb;
    stream3.next_in = (Bytef *)ptr;
    // define output buffer
    int ret = inflate(&stream3, Z_NO_FLUSH);
    int have = 1000000 - stream3.avail_out;
    if (ret == Z_STREAM_END)
    {
        inflateEnd(&stream3);
    }

    one_chunk(stream, header, file_handle, (Byte *)out, have);
    stream3.avail_out = 1000000;
    stream3.next_out = (Bytef *)out;
    return size * nmemb;
}

int main(int argc, char **argv)
{
    ////
    if (argc != 5)
    {
        printf("Usage: %s <input_trace_filename> <processing case> <hf token> <output dir> \n", argv[0]);
        return 1;
    }
    strcpy(target, argv[4]);
    flag_filename = atoi(argv[2]);
    if (flag_filename == 0)
    {
        printf("Processing validation set\n");
        // make val directory
        try
        {

            mkdir(strcat(target,"/val"), S_IRWXU);
        }
        catch (...)
        {
            printf("e%s%d\n", "./val", errno);
            assert(errno == EEXIST);
        }
    }
    else if (flag_filename == 1)
    {
        printf("Processing training set\n");
        try
        {
            mkdir(strcat(target,"/train"), S_IRWXU);
        }
        catch (...)
        {
            printf("e%s%d\n", "./train", errno);
            assert(errno == EEXIST);
        }
    }
    else if (flag_filename == 2)
    {
        printf("Processing test set\n");
        try
        {
            mkdir(strcat(target,"/test"), S_IRWXU);
        }
        catch (...)
        {
            printf("e%s%d\n", "./test", errno);
            assert(errno == EEXIST);
        }
    }
    ////
    stream3.zalloc = Z_NULL;
    stream3.zfree = Z_NULL;
    stream3.opaque = Z_NULL;
    stream3.avail_out = 1000000;
    stream3.next_out = (Bytef *)out;
    assert(tar_inflateInit(&stream) == TAR_OK);
    assert(stream.len_out == 0);
    tar_inflateGetHeader(&stream, &header);
    int ret = inflateInit2(&stream3, 16 + MAX_WBITS);
    if (ret != Z_OK)
        return ret;
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
        /* example.com is redirected, so we tell libcurl to follow redirection */
        std::string header = std::string("Authorization: Bearer ") + std::string(argv[3]);
        headers = curl_slist_append(headers, header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128000L);
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return 0;
}
// int main(int argc, char **argv)
// {
//     if (argc != 2)
//     {
//         printf("Usage: %s <input_trace_filename>\n", argv[0]);
//         return 1;
//     }
//     std::string filename = std::string(argv[1]);
//     int index = filename.find_last_of("/\\");
//     input_trace_filename = filename.substr(index+1);
//     printf("filename: %s\n", input_trace_filename.c_str());
//     return 0;
// }
// Case 0: "ILSVRC2012_val_00049946_n04317175.JPEG"  --> "val/n04317175/ILSVRC2012_val_00049946.JPEG"
// Case 1: "n01498041_2538_n01498041.JPEG" --> "train/n01498041/n01498041_2538.JPEG"
// Case 2: "ILSVRC2012_test_00063706.JPEG" --> "test/ILSVRC2012_test_00063706.JPEG"
