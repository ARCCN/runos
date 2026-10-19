/*
 * Copyright 2019 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <streambuf>
#include "Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <backward.hpp>
#include <fmt/format.h>

#define MEGABYTE 1024*1024

/* SpdlogStreamfuf is required for the thirdparty libraries that don't use spdlog, 
 * but can make output through std::ostream */

class SpdlogStreambuf : public std::streambuf {
public:
    SpdlogStreambuf() = default;

protected:
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
        for( std::streamsize i = 0; i < n; ++i ) 
        {
            if (s[i] == '\n') {
                log(buffer);
                buffer.clear();
                continue;
            }
            buffer += s[i];
        }
        return n;
    }

    virtual int overflow(int c) override {
        if (c != EOF ) {
            if (c == '\n') {
                log(buffer);
                buffer.clear();
                return c;
            }
            buffer += static_cast<char>(c);
        }
        return c;
    }

private:
    std::string buffer;

    void log(const std::string& str) {
        LOG_ERROR("{}", str);
    }
};


using namespace backward;
void MessageHandle( int sig )
{
    LOG_ERROR( "signal: '{}'", strdup( strsignal( sig )));
    spdlog::set_pattern( "%^%v%$" );

    /* Take a snapshot of the current stack, with at most "trace_cnt_max" traces in it. */
    const size_t trace_cnt_max = 32;
    SpdlogStreambuf spdlog_buf;
    std::ostream st_stream(&spdlog_buf);

    StackTrace st; 
    st.load_here( trace_cnt_max );
    Printer printer;
    printer.color_mode = ColorMode::never;
    printer.print( st, st_stream ); //print stacktrace with spdlog

    exit( EXIT_FAILURE );
}

using spdlog::details::to_string_view;
using spdlog::level::from_str;
namespace runos
{

void spdlog_init( const Config& config )
{
    spdlog::info( "Spdlog initialization begin" );
    
    // console-log-level default if have fault with settings.json
    constexpr spdlog::level::level_enum def_console_level = spdlog::level::trace;
    
    // file-log-level default if have fault with settings.json
    constexpr spdlog::level::level_enum def_journal_level = spdlog::level::trace;
    
    // default flush-file-level
    constexpr spdlog::level::level_enum def_journal_level_flush = spdlog::level::warn;
    
    // default journal path
    constexpr std::string_view def_journal_name( "var/log/journal.txt" );
    
    std::string stdout_verbose, journal_verbose, journal_name, journal_size, flush_policy;

    try{ 
        auto& spd_set = config_cd( config, "spdlogger" );

        flush_policy = config_get( spd_set, "flush_policy", "0" );
        stdout_verbose = config_get( spd_set, "stdout_verbose",  fmt::format( to_string_view( def_console_level )));
        journal_verbose = config_get( spd_set, "journal_verbose", fmt::format( to_string_view( def_journal_level )));
        journal_name = config_get( spd_set, "journal_name", std::string( def_journal_name ));
        journal_size = config_get( spd_set, "journal_Msize", "2" );     // set journal_size  = 2 MByte

        spdlog::info( "flush_policy: {};", std::stoi( flush_policy ));
        spdlog::info( "stdout_verbose: {};", stdout_verbose );
        spdlog::info( "journal_verbose: {};", journal_verbose );
        spdlog::info( "journal_name: {};", journal_name );
        spdlog::info( "journal_size: {};", journal_size );
    }

    catch( const spdlog::spdlog_ex& ex )
    {
        spdlog::error( "Config initialization failed: {}", ex.what());
    }

    // "make console sink"
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    if( from_str( stdout_verbose ) != SPDLOG_LEVEL_OFF )
        console_sink->set_level( from_str( stdout_verbose ));
    else 
        console_sink->set_level( def_console_level );
        
    // "make file sink"
    uint32_t file_size = 0;
    file_size = static_cast<uint32_t>( std::stoi( journal_size ));
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>( journal_name, MEGABYTE, file_size, false );

    // "set file sink level"
    if( from_str( journal_verbose ) != SPDLOG_LEVEL_OFF )
        file_sink->set_level( from_str( journal_verbose ));
    else
        file_sink->set_level( def_journal_level );

    // "make default logger"
    spdlog::set_default_logger( 
        std::make_shared<spdlog::logger>( "spdlogger", spdlog::sinks_init_list({ console_sink, file_sink }))); 

    // "set logger policy"
    uint32_t flush_freq =  static_cast<uint32_t>( std::stoi( flush_policy ));
    if( flush_freq )
    {
        spdlog::flush_every( std::chrono::seconds( flush_freq ));
        spdlog::flush_on( def_journal_level_flush ); 
    }
    else
        spdlog::flush_on( def_journal_level_flush );

    /* set formatting
     * pattern flags: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
      
        %^  : start color range (can be used only once)
        %L  : Short log level of the message
        %d  : Day of month 01-31
        %m  : Month 01-12
        %T  : ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S
        %f  : Microsecond part of the current second 000000-999999
        %n  : Logger's name
        %!  : Source function() 
        %t  : Thread id
        %s  : Basename of the source file
        %#  : Source line
        %v  : The actual text to log 
        %$  : end color range */
    
    spdlog::set_pattern( "%^%L%m%d %T.%f [%! %t %s:%#] %v %$" );

    // set logger verbose-level
    spdlog::set_level( std::min( from_str( journal_verbose ), from_str( stdout_verbose )));

    spdlog::info( "Spdlog initialization is complete" );
  }

} // namespace runos
