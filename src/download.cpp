// SPDX-FileCopyrightText: 2021-2026 Jens A. Koch
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/jakoch/wikifolio_universe_converter.

#include "download.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include <curl/curl.h>

namespace
{
    size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata)
    {
        FILE* stream = static_cast<FILE*>(userdata);
        if (stream == nullptr) {
            return 0;
        }
        return fwrite(ptr, size, nmemb, stream);
    }
} // anonymous namespace

bool download(char const * url, std::string const & save_as_filename)
{
    // Ensure global init/cleanup are tied to program lifetime but triggered
    // on first use of this function.
    static struct CurlGlobal
    {
        CurlGlobal()
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }
        ~CurlGlobal()
        {
            curl_global_cleanup();
        }
        CurlGlobal(CurlGlobal const &)            = delete;
        CurlGlobal& operator=(CurlGlobal const &) = delete;
        CurlGlobal(CurlGlobal&&)                  = delete;
        CurlGlobal& operator=(CurlGlobal&&)       = delete;
    } const guard;

    CURLcode res = CURLE_OK;

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers                    = curl_slist_append(headers, "Content-Type:application/octet-stream");
    headers                    = curl_slist_append(headers, "Content-Transfer-Encoding: Binary");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    using file_ptr = std::unique_ptr<FILE, decltype(&std::fclose)>;

    file_ptr const fhandle(std::fopen(save_as_filename.c_str(), "wb"), &std::fclose);

    if (fhandle == nullptr) {
        if (headers != nullptr) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fhandle.get());

    std::array<char, CURL_ERROR_SIZE> error_buffer{};
    error_buffer[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer.data());

    int http_code = 0;
    res           = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    int const HTTP_ERROR_ANY = 400;
    if (res == CURLE_OK) {
        if (http_code >= HTTP_ERROR_ANY) {
            std::cout << "Request failed with status" << http_code << "\n";
            remove(save_as_filename.c_str());
            res = CURLE_HTTP_RETURNED_ERROR;
        }
    } else {
        remove(save_as_filename.c_str());
        std::cout << "Request failed: " << error_buffer.data() << "\n";
    }

    if (headers != nullptr) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}
