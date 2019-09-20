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
 
#include <cstdlib>
#include <iostream>
#include <memory>

#include <pthread.h>

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <execinfo.h>

#include "runos/core/logging.hpp"

#include "redisclient.hpp"

#define debugLine printf("\n%s:%d\n", __FILE__, __LINE__)

enum class ReturnType {
    UNDEFINED,
    SIMPLE_STRING,
    ERROR,
    INTEGER,
    BULK_STRING,
    ARRAY
};

static void full_write(int fd, const char *buf, size_t len)
{
        while (len > 0) {
                ssize_t ret = write(fd, buf, len);

                if ((ret == -1) && (errno != EINTR))
                        break;

                buf += (size_t) ret;
                len -= (size_t) ret;
        }
}

void print_rcBacktrace(const char* reason)
{
        static const char start[] = "\x1b[1;31m------------ RC-BACKTRACE ------------\x1b[0m\n";
        static const char end[] =   "\x1b[1;31m--------------------------------------\x1b[0m\n";

        void *bt[1024];
        int bt_size;
        char **bt_syms;
        int i;

        bt_size = backtrace(bt, 1024);
        bt_syms = backtrace_symbols(bt, bt_size);
        full_write(STDERR_FILENO, start, strlen(start));
        full_write(STDERR_FILENO, reason, strlen(reason));
        for (i = 1; i < bt_size; i++) {
                size_t len = strlen(bt_syms[i]);
                full_write(STDERR_FILENO, bt_syms[i], len);
                full_write(STDERR_FILENO, "\n", 1);
        }
        full_write(STDERR_FILENO, end, strlen(end));
    free(bt_syms);
}

/**
 * Читает целое число из строки, если ошибка то вернёт -1
 * @param buffer Строка
 * @param delimiter Конец для числа
 * @param delta Количество символов занятое числом и разделителем
 * @return
 */
int read_int(const char* buffer,char delimiter,int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 9)
        {
            return -1;
        }
    }
    return len;
}

int read_int(const char* buffer,char delimiter)
{
    const char* p = buffer;
    int len = 0;
    int delta = 0;

    while(*p == '0' )
    {
        p++;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        delta++;
        if(delta > 9)
        {
            return -1;
        }
    }

    return len;
}

int read_int(const char* buffer, int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(1)
    {
        if(*p > '9' || *p < '0')
        {
            return len;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 9)
        {
            return -1;
        }
    }
    return len;
}
/**
 * Читает целое число из строки, если ошибка то вернёт -1
 * @param buffer Строка
 * @param delimiter Конец для числа
 * @param delta Количество символов занятое числом и разделителем
 * @return
 */
long read_long(const char* buffer,char delimiter,int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 18)
        {
            return -1;
        }
    }
    return len;
}

long read_long(const char* buffer,char delimiter)
{
    const char* p = buffer;
    int len = 0;
    int delta = 0;

    while(*p == '0' )
    {
        p++;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        delta++;
        if(delta > 18)
        {
            return -1;
        }
    }

    return len;
}

long read_long(const char* buffer, int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(1)
    {
        if(*p > '9' || *p < '0')
        {
            return len;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 18)
        {
            return -1;
        }
    }
    return len;
}
 
    #define REDIS_PRINTF_MACRO_CODE(type, comand) va_list ap;\
                        va_start(ap, format);\
                        bzero(buf, buffer_size);\
                        int  rc = vsnprintf(buf, buffer_size, format, ap);\
                        va_end(ap);\
                        if( rc >= buffer_size ) return RC_ERR_BUFFER_OVERFLOW;\
                        if(rc <  0) return RC_ERR_DATA_FORMAT;\
                        rc = redis_send( type, "%s %s\r\n", comand, buf);\
                        return rc;\


    SimpleRedisClient::SimpleRedisClient()
    {
        setBufferSize(2048);
    }

    int SimpleRedisClient::getBufferSize()
    {
        return buffer_size;
    }


    void SimpleRedisClient::setMaxBufferSize(int size)
    {
        max_buffer_size = size;
    }

    int SimpleRedisClient::getMaxBufferSize()
    {
        return max_buffer_size;
    }

    void SimpleRedisClient::setBufferSize(int size)
    {
        if(buffer != 0)
        {
            delete[] buffer;
            delete[] buf;
        }

        buffer_size = size;
        buffer = new char[buffer_size];
        buf = new char[buffer_size];
    }

    SimpleRedisClient::~SimpleRedisClient()
    {
        redis_close();

        
        if(buffer != NULL)
        {
            delete[] buffer;
        }
        
        if(buf != NULL)
        {
            delete[] buf;
        }
        
        buffer_size = 0;

        if(host != 0)
        {
            delete[] host;
        }
        
        if(lastAuthPw != NULL)
        {
            delete[] lastAuthPw;
        }
    }

    /*
     * Returns:
     *  >0  Колво байт
     *   0  Соединение закрыто
     *  -1  error
     *  -2  timeout
     **/
    int SimpleRedisClient::read_select(int fd, int timeout ) const
    {
        struct timeval tv;
        fd_set fds;

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        return select(fd + 1, &fds, NULL, NULL, &tv);
    }

    /*
     * Returns:
     *  >0  Колво байт
     *   0  Соединение закрыто
     *  -1  error
     *  -2  timeout
     **/
    int SimpleRedisClient::wright_select(int fd, int timeout ) const
    {
        struct timeval tv;
        fd_set fds;

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        return select(fd+1, NULL, &fds, NULL, &tv);
    }

    void SimpleRedisClient::LogLevel(int l)
    {
        debug = l;
    }

    int SimpleRedisClient::LogLevel(void)
    {
        return debug;
    }

    int SimpleRedisClient::getError(void)
    {
        return last_error;
    }
    
    int SimpleRedisClient::redis_raw_send(char recvtype,const char *dataCommand)
    {        
        int rc = send_data(dataCommand);
        
        if (rc < 0)
        {
            print_rcBacktrace("Данные не отправлены [RC_ERR_SEND]");
            reconnect();
            return RC_ERR_SEND;
        }
        
        if (rc != (int) strlen(dataCommand))
        { 
            print_rcBacktrace("Ответ не получен [RC_ERR_TIMEOUT]");
            reconnect();
            return RC_ERR_TIMEOUT;
        }


        bzero(buffer,buffer_size);
        rc = read_select(fd, timeout);

        if(debug) printf("REDIS read_select=%d\n", rc);
        if (rc > 0)
        {

            int offset = 0;
            do{
                usleep(1000);
                rc = recv(fd, buffer + offset, buffer_size - offset, 0);

                if(rc < 0)
                {
                    return CR_ERR_RECV;
                }

                if(rc >= buffer_size - offset && buffer_size * 2 > max_buffer_size)
                {
                    char nullbuf[1000];
                    int r = 0;
                    while( (r = recv(fd, nullbuf, 1000, 0)) >= 0)
                    {
                        if(debug) printf("REDIS read %d byte\n", r);
                    }

                    last_error = RC_ERR_DATA_BUFFER_OVERFLOW;
                    break;
                }
                else if(rc >= buffer_size - offset && buffer_size * 2 < max_buffer_size)
                {
                    if(debug) printf("REDIS Удвоение размера буфера до %d\t[rc=%d, buffer_size=%d, offset=%d]\n",buffer_size *2, rc, buffer_size, offset);

                    int last_buffer_size = buffer_size;
                    char* tbuf = buffer;

                    buffer_size *= 2;
                    buffer = new char[buffer_size];

                    delete buf;
                    buf = new char[buffer_size];

                    memcpy(buffer, tbuf, last_buffer_size);
                    offset = last_buffer_size;
                }
                else
                {
                    break;
                }

            }while(1);

            if (debug >= RC_LOG_DEBUG) {
                printf("REDIS BUF: recv:%d buffer[%s]",rc, buffer);
            }

            char prefix = buffer[0];
            if (recvtype != RC_ANY && prefix != recvtype && prefix != RC_ERROR)
            {
                // During tests I've found one problem with the difference between
                // the type we are waiting for (RC_MULTIBULK) and reveived data (prefix of
                // them sometimes is detected as RC_INLINE)
                // To fix this we manually just assign prefix to RC_MULTIBULK and successfully
                // parse the rest of data
                // Alex Nepochatykh
                if (RC_MULTIBULK == recvtype) {
                    prefix = RC_MULTIBULK;
                } else {
                    printf("\x1b[31m[fd=%d]REDIS RC_ERR_PROTOCOL[%c]:%s\x1b[0m\n",fd, recvtype, buffer);
                    print_rcBacktrace(buffer);
                    return RC_ERR_PROTOCOL;
                }
            }

            char *p;
            int len = 0;
            switch (prefix)
            {
                case RC_ERROR:
                    if (debug) {
                        printf("\x1b[31mREDIS[fd=%d] RC_ERROR:%s\x1b[0m\n",fd,buffer);
                    }
                    data = buffer;
                    data_size = rc;
                    print_rcBacktrace(buffer);
                    return -rc;
                case RC_INLINE:
                    if(debug) {
                        printf("\x1b[33mREDIS[fd=%d] RC_INLINE:%s\x1b[0m\n", fd,buffer);
                    }
                    data_size = strlen(buffer+1)-2;
                    data = buffer+1;
                    data[data_size] = 0;
                    return rc;
                case RC_INT:
                    if(debug) {
                        printf("\x1b[33mREDIS[fd=%d] RC_INT:%s\x1b[0m\n",fd, buffer);
                    }
                    data = buffer+1;
                    data_size = rc;
                    return rc;
                case RC_BULK:
                    if(debug) {
                        printf("\x1b[33mREDIS[fd=%d] RC_BULK:%s\x1b[0m\n",fd, buffer);
                    }

                    p = buffer;
                    p++;

                    if(*p == '-') {
                        data = 0;
                        data_size = -1;
                        return rc;
                    }

                    while(*p != '\r') {
                        len = (len*10)+(*p - '0');
                        p++;
                    }

                    /* Now p points at '\r', and the len is in bulk_len. */
                    if(debug > 3) {
                        printf("%d\n", len);
                    }
                    data = p+2;
                    data_size = len;
                    data[data_size] = 0;

                    return rc;
                case RC_MULTIBULK:
                {
                    parse_multibulk_data();
                    return rc;
                }

            default:
                return rc;
            }
        }
        else if (rc == 0) {
            print_rcBacktrace("Соединение закрыто [RC_ERR_CONECTION_CLOSE]");
            reconnect();
            return RC_ERR_CONECTION_CLOSE; // Соединение закрыто
        } else {
            print_rcBacktrace("Не пришли данные от redis[RC_ERR]");
            return RC_ERR; // error
        }
    }

    int SimpleRedisClient::redis_send(char recvtype, const char *format, ...)
    {
        if(fd <= 0)
        {
            if(redis_connect() < 0)
            {
                return RC_ERR;
            }
        }

        data = 0;
        data_size = 0;

        va_list ap;
        va_start(ap, format);

        bzero(buffer,buffer_size);
        int  rc = vsnprintf(buffer, buffer_size, format, ap);
        va_end(ap);

        if( rc < 0 )
        {
            return RC_ERR_DATA_FORMAT;
        }

        if( rc >= buffer_size )
        {
            return RC_ERR_BUFFER_OVERFLOW; // Не хватило буфера
        }

        if(debug > 3  ) printf("SEND:%s",buffer);
        
        return redis_raw_send(recvtype, buffer); 
    }

    /**
     * Отправляет данные
     * @param buf
     * @return
     */
    int SimpleRedisClient::send_data( const char *buf ) const
    {
        fd_set fds;
        struct timeval tv;
        int sent = 0;

        /* NOTE: On Linux, select() modifies timeout to reflect the amount
         * of time not slept, on other systems it is likely not the same */
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;

        int tosend = strlen(buf); // При отправке бинарных данных возможны баги.

        while (sent < tosend)
        {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            int rc = select(fd + 1, NULL, &fds, NULL, &tv);

            if (rc > 0)
            {
                rc = send(fd, buf + sent, tosend - sent, 0);

                if (rc < 0)
                {
                    return -1;
                }
                sent += rc;
            }
            else if (rc == 0) /* timeout */
            {
                break;
            }
            else
            {
                return -1;
            }
        }

        return sent;
    }

    void SimpleRedisClient::parse_multibulk_data()
    {
        ReturnType return_type = ReturnType::UNDEFINED;
        answer_multibulk_vec_.clear();
        int token = 0;
        auto buffer_length = strlen(buffer);
        unsigned int i = 0;
        while (i < buffer_length) {
            char symbol = buffer[i];
            if (symbol == RC_ERROR || symbol == RC_INLINE ||
                symbol == RC_BULK || symbol == RC_MULTIBULK ||
                symbol == RC_INT || symbol == RC_ANY) {
                token = 0;
                if (symbol == RC_MULTIBULK) {
                    return_type = ReturnType::ARRAY;
                }
                if (symbol == RC_BULK) {
                    return_type = ReturnType::BULK_STRING;
                }
                ++i;
                continue;
            }
            if (isdigit(symbol)) {
                auto integer = (symbol - '0');
                token = 10 * token + integer;
                ++i;
                continue;
            }
            if (symbol == '\r') {
                i += 2; // skip \r\n
                continue;
            }
            if (return_type == ReturnType::BULK_STRING) {
                // copy to new_string
                std::string new_string;
                for (int jj = 0; jj < token; ++jj) {
                    new_string.push_back(buffer[i + jj]);
                }
                answer_multibulk_vec_.push_back(new_string);
                i += token;
                token = 0;
                return_type = ReturnType::UNDEFINED;
            }
            ++i;
        }
    }

    /**
     *  public:
     */

    void SimpleRedisClient::setPort(int Port)
    {
        port = Port;
    }

    int SimpleRedisClient::getPort()
    {
        return port;
    }

    void SimpleRedisClient::setHost(const char* Host)
    {
        if(host != 0)
        {
            delete host;
        }

        host = new char[64];
        bzero(host, 64);
        memcpy(host,Host,strlen(Host));
    }

    char *SimpleRedisClient::getHost() const
    {
        return host;
    }

    /**
     * Соединение с редисом.
     */
    int SimpleRedisClient::redis_connect(const char* Host,int Port)
    {
        setPort(Port);
        setHost(Host);
        return redis_connect();
    }

    int SimpleRedisClient::redis_connect(const char* Host,int Port, int TimeOut)
    {
        setPort(Port);
        setHost(Host);
        setTimeout(TimeOut);
        return redis_connect();
    }

    int SimpleRedisClient::reconnect()
    {
        
        if(debug > 1) printf("\x1b[31mredis reconect[%s:%d]\x1b[0m\n", host, port);
        
        redis_close();
        if(!redis_connect())
        {
            return false;
        }
        
        if(lastAuthPw != NULL )
        {
            printf("\x1b[31mredis reconect lastAuthPw=%s\x1b[0m\n", lastAuthPw);
            if(redis_send( RC_INLINE, "AUTH %s\r\n", lastAuthPw))
            {
                return false;
            }
        }
        
        if(lastSelectDBIndex != 0 )
        {
            if( !selectDB(lastSelectDBIndex) )
            {
                return false;
            }
        }
        
        return true;
    }
    
    int SimpleRedisClient::redis_connect()
    {  
        if(host == 0)
        {
            setHost("127.0.0.1");
        }
        
        if(debug >= RC_LOG_LOG) printf("\x1b[32mredis host:%s %d\x1b[0m\n", host, port);
        
        int rc;
        struct sockaddr_in sa;
        bzero(&sa, sizeof(sa));

        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);

        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 || setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&yes, sizeof(yes)) == -1 || setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&yes, sizeof(yes)) == -1)
        {
            printf("open error %d\n", fd);
        }

        struct addrinfo hints, *info = NULL;
        bzero(&hints, sizeof(hints));

        if (inet_aton(host, &sa.sin_addr) == 0)
        { 
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            int err = getaddrinfo(host, NULL, &hints, &info);
            if (err)
            {
                printf("\x1b[31mgetaddrinfo error: %s [%s]\x1b[0m\n", gai_strerror(err), host);
                //print_rcBacktrace("\x1b[31mgetaddrinfo error\x1b[0m\n");
                return RC_ERR;
            }

            memcpy(&sa.sin_addr.s_addr, &(info->ai_addr->sa_data[2]), sizeof(in_addr_t));
            freeaddrinfo(info);
        }

        int flags = fcntl(fd, F_GETFL);
        if ((rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0)
        {
            printf("\x1b[31mSetting socket non-blocking failed with: %d\x1b[0m\n", rc);
            //print_rcBacktrace("\x1b[31mSetting socket non-blocking failed\x1b[0m\n");
            return RC_ERR;
        }
 
        if( connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
        {
            if (errno != EINPROGRESS)
            {
                //print_rcBacktrace("\x1b[31mconnect error\x1b[0m\n");
                close(fd);
                fd = 0;
                return RC_ERR;
            }

            if (wright_select(fd, timeout) > 0)
            {
                int err = 0;
                unsigned int len = sizeof(err);
                if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err)
                {
                    //print_rcBacktrace("\x1b[31mgetsockopt error\x1b[0m\n");
                    close(fd);
                    fd = 0;
                    return RC_ERR;
                }
            }
            else /* timeout or select error */
            {
                //print_rcBacktrace("\x1b[31mtimeout or select error\x1b[0m\n");
                close(fd);
                fd = 0;
                return RC_ERR_TIMEOUT;
            }
        }
        
        if(debug >  RC_LOG_DEBUG) printf("open ok %d\n", fd);
        return fd;
    }

    /**
     * Вернёт количество ответов.
     * @return
     */
    int SimpleRedisClient::getMultiBulkVectorData()
    {
        return answer_multibulk_vec_.size();
    }

    std::vector<std::string> SimpleRedisClient::getVectorData() const
    {
        return answer_multibulk_vec_;
    }
     
    /**
     * Выбор бызы данных
     * @param index
     * @see http://redis.io/commands/select
     */
    int SimpleRedisClient::selectDB(int index)
    {
      lastSelectDBIndex = index;
      return redis_send(RC_INLINE, "SELECT %d\r\n", index);
    }
    

    /**
     * Возвращает все члены множества, сохранённого в указанном ключе. Эта команда - просто упрощённый синтаксис для SINTER.
     * SMEMBERS key
     * Время выполнения: O(N).
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-smembers
     * @see http://redis.io/commands/smembers
     * @param key
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::smembers(const char *key)
    {
      return redis_send(RC_MULTIBULK, "SMEMBERS '%s'\r\n", key);
    }

    /**
     * Возвращает все члены множества, сохранённого в указанном ключе. Эта команда - просто упрощённый синтаксис для SINTER.
     * SMEMBERS key
     * Время выполнения: O(N).
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-smembers
     * @see http://redis.io/commands/smembers
     * @param key
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::smembers_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_MULTIBULK, "SMEMBERS")
    }

    /**
     * Returns the set cardinality (number of elements) of the set stored at key.
     * @see http://redis.io/commands/scard
     * @param key
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::scard(const char *key)
    {
      return redis_send(RC_INT, "SCARD '%s'\r\n", key);
    }

    /**
     * Returns the set cardinality (number of elements) of the set stored at key.
     * @see http://redis.io/commands/scard
     * @param key
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::scard_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "SCARD")
    }

    /**
     * Ни ключ ни значение не должны содержать "\r\n"
     * @param key
     * @param val
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::set(const char *key, const char *val)
    {
        return redis_send( RC_INLINE, "SET '%s' '%s'\r\n",   key, val);
    }

    int SimpleRedisClient::set_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INLINE, "SET")
    }

    SimpleRedisClient& SimpleRedisClient::operator=(const char *key_val)
    {
        redis_send( RC_INLINE, "SET %s\r\n", key_val);
        return *this;
    }


    int SimpleRedisClient::setex(const char *key, const char *val, int seconds)
    {
        return redis_send(RC_INLINE, "SETEX '%s' %d '%s'\r\n",key, seconds, val);
    }

    int SimpleRedisClient::setex_printf(int seconds, const char *key, const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, buffer_size);
        int  rc = vsnprintf(buf, buffer_size, format, ap);
        va_end(ap);

        if( rc >= buffer_size )
        {
            return RC_ERR_BUFFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        return redis_send(RC_INLINE, "SETEX '%s' %d '%s'\r\n",key, seconds, buf);
    }

    int SimpleRedisClient::setex_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INLINE, "SETEX")
    }


    int SimpleRedisClient::get(const char *key)
    {
      return redis_send( RC_BULK, "GET '%s'\r\n", key);
    }


    int SimpleRedisClient::get_printf( const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_BULK, "GET")
    }

    /**
     * Add the specified members to the set stored at key. Specified members that are already a member of this set are ignored. If key does not exist, a new set is created before adding the specified members.
     * An error is returned when the value stored at key is not a set.
     * @see http://redis.io/commands/sadd
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-sadd
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::sadd(const char *key, const char *member)
    {
      return redis_send(RC_INT, "SADD '%s' '%s'\r\n", key, member);
    }

    /**
     * Add the specified members to the set stored at key. Specified members that are already a member of this set are ignored. If key does not exist, a new set is created before adding the specified members.
     * An error is returned when the value stored at key is not a set.
     * @see http://redis.io/commands/sadd
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-sadd
     * @param format
     * @param ... Ключь Значение (через пробел, в значении нет пробелов)
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::sadd_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "SADD")
    }

    /**
     * Remove the specified members from the set stored at key.
     * Specified members that are not a member of this set are ignored.
     * If key does not exist, it is treated as an empty set and this command returns 0.
     * An error is returned when the value stored at key is not a set.
     *
     * @see http://redis.io/commands/srem
     * @param key
     * @param member
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::srem(const char *key, const char *member)
    {
        return redis_send(RC_INT, "SREM '%s' '%s'\r\n", key, member);
    }

    /**
     * Remove the specified members from the set stored at key.
     * Specified members that are not a member of this set are ignored. If key does not exist, it is treated as an empty set and this command returns 0.
     * An error is returned when the value stored at key is not a set.
     * @see http://redis.io/commands/srem
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-srem
     * @param format
     * @param ... Ключь Значение (через пробел, в значении нет пробелов)
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::srem_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "SREM")
    }


    char* SimpleRedisClient::operator[] (const char *key)
    {
        redis_send( RC_BULK, "GET '%s'\r\n", key);
        return getData();
    }

    SimpleRedisClient::operator char* () const
    {
        return getData();
    }

    /**
     * @todo ЗАДОКУМЕНТИРОВАТЬ
     * @return
     */
    SimpleRedisClient::operator int () const
    {
        if(data_size < 1)
        {
            printf("SimpleRedisClient::operator int (%u) \n", data_size);
            return data_size;
        }

        if(getData() == 0)
        {
            printf("SimpleRedisClient::operator int (%u) \n", data_size);
            return -1;
        }

        int d = 0;
        int r = read_int(getData(), &d);

        printf("SimpleRedisClient::operator int (%u|res=%d) \n", data_size, r);

        return r;
    }

    /**
     * @todo ЗАДОКУМЕНТИРОВАТЬ
     * @return
     */
    SimpleRedisClient::operator long () const
    {
        if(data_size < 1)
        {
            printf("SimpleRedisClient::operator long (%u) \n", data_size);
            return data_size;
        }

        if(getData() == 0)
        {
            printf("SimpleRedisClient::operator long (%u) \n", data_size);
            return -1;
        }

        int d = 0;
        int r = read_long(getData(), &d);

        printf("SimpleRedisClient::operator long (%u|res=%d) \n", data_size, r);

        return r;
    }

    int SimpleRedisClient::getset(const char *key, const char *set_val)
    {
      return redis_send( RC_BULK, "GETSET '%s' '%s'\r\n",   key, set_val);
    }

    int SimpleRedisClient::getset_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_BULK, "GETSET")
    }

    int SimpleRedisClient::ping()
    {
      return redis_send( RC_INLINE, "PING\r\n");
    }

    int SimpleRedisClient::echo(const char *message)
    {
      return redis_send( RC_BULK, "ECHO '%s'\r\n", message);;
    }

    int SimpleRedisClient::quit()
    {
      return redis_send( RC_INLINE, "QUIT\r\n");
    }

    int SimpleRedisClient::auth(const char *password)
    {
      if(lastAuthPw != NULL)
      {
          delete[] lastAuthPw;
      }
      
      int pwLen = strlen(password);
      lastAuthPw = new char[pwLen+1];
      memcpy(lastAuthPw, password, pwLen);
      lastAuthPw[pwLen] = 0;
      
      printf("\x1b[32mnewdatabase new lastAuthPw[%d]='%s'\x1b[0m\n", pwLen, lastAuthPw);
      
      return redis_send( RC_INLINE, "AUTH %s\r\n", password);
    }


    int SimpleRedisClient::getRedisVersion()
    {
      /* We can receive 2 version formats: x.yz and x.y.z, where x.yz was only used prior
       * first 1.1.0 release(?), e.g. stable releases 1.02 and 1.2.6 */
      /* TODO check returned error string, "-ERR operation not permitted", to detect if
       * server require password? */
      if (redis_send( RC_BULK, "INFO\r\n") == 0)
      {
        sscanf(buffer, "redis_version:%9d.%9d.%9d\r\n", &version_major, &version_minor, &version_patch);
        return version_major;
      }

      return 0;
    }


    int SimpleRedisClient::setnx(const char *key, const char *val)
    {
      return redis_send( RC_INT, "SETNX '%s' '%s'\r\n",  key, val);
    }

    int SimpleRedisClient::setnx_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "SETNX")
    }

    int SimpleRedisClient::append(const char *key, const char *val)
    {
      return redis_send(RC_INT, "APPEND '%s' '%s'\r\n",  key, val);
    }

    int SimpleRedisClient::append_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "APPEND")
    }

    int SimpleRedisClient::substr( const char *key, int start, int end)
    {
      return redis_send(RC_BULK, "SUBSTR '%s' %d %d\r\n",   key, start, end);
    }

    int SimpleRedisClient::exists( const char *key)
    {
      return redis_send( RC_INT, "EXISTS '%s'\r\n", key);
    }

    int SimpleRedisClient::exists_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "EXISTS")
    }


    /**
     * Время выполнения: O(1)
     * Удаление указанных ключей. Если переданный ключ не существует, операция для него не выполняется. Команда возвращает количество удалённых ключей.
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-del
     * @see http://redis.io/commands/del
     * @param key
     * @return
     */
    int SimpleRedisClient::del( const char *key)
    {
      return redis_send( RC_INT, "DEL '%s'\r\n", key);
    }

    int SimpleRedisClient::del_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "DEL")
    }

    int SimpleRedisClient::type( const char *key)
    {
      return redis_send( RC_INLINE, "TYPE '%s'\r\n", key);
    }

    int SimpleRedisClient::keys( const char *pattern)
    {
      return redis_send(RC_MULTIBULK, "KEYS '%s'\r\n", pattern);
    }

    int SimpleRedisClient::keys_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_MULTIBULK, "KEYS")
    }

    int SimpleRedisClient::randomkey()
    {
      return redis_send( RC_BULK, "RANDOMKEY\r\n");
    }

    int SimpleRedisClient::flushall(void)
    {
        return redis_send( RC_INLINE, "FLUSHALL\r\n");
    }

    int SimpleRedisClient::rename( const char *key, const char *new_key_name)
    {
      return redis_send( RC_INLINE, "RENAME '%s' '%s'\r\n",   key, new_key_name);
    }

    int SimpleRedisClient::rename_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INLINE, "RENAME")
    }

    int SimpleRedisClient::renamenx( const char *key, const char *new_key_name)
    {
      return redis_send( RC_INT, "RENAMENX '%s' '%s'\r\n", key, new_key_name);
    }

    int SimpleRedisClient::renamenx_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "RENAMENX")
    }

    /**
     * Return the number of keys in the selected database 
     */
    int SimpleRedisClient::dbsize()
    {
      return redis_send( RC_INT, "DBSIZE\r\n");
    }

    int SimpleRedisClient::expire( const char *key, int secs)
    {
      return redis_send( RC_INT, "EXPIRE '%s' %d\r\n", key, secs);
    }

    int SimpleRedisClient::expire_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "EXPIRE")
    }


    int SimpleRedisClient::ttl( const char *key)
    {
      return redis_send( RC_INT, "TTL '%s'\r\n", key);
    }

    int SimpleRedisClient::ttl_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "TTL")
    }

    // Списки

    int SimpleRedisClient::lpush(const char *key, const char *val)
    {
      return redis_send( RC_INT, "LPUSH '%s' '%s'\r\n", key, val);
    }

    int SimpleRedisClient::lpush_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "LPUSH")
    }

    int SimpleRedisClient::rpush(const char *key, const char *val)
    {
      return redis_send( RC_INT, "RPUSH '%s' '%s'\r\n", key, val);
    }

    int SimpleRedisClient::rpush_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "RPUSH")
    }

    int SimpleRedisClient::setMasterRole()
    {
      return redis_send( RC_INLINE, "SLAVEOF NO ONE\r\n");
    }

    int SimpleRedisClient::setSlaveOf(const char *address, const char *port)
    {
        return redis_send( RC_INLINE, "SLAVEOF '%s' '%s'\r\n", address, port);
    }

    int SimpleRedisClient::setSlaveReadOnlyNo()
    {
        return redis_send( RC_INLINE, "CONFIG SET slave-read-only no\r\n");
    }

    /**
     * LTRIM mylist 1 -1
     */
    int SimpleRedisClient::ltrim(const char *key, int start_pos, int count_elem)
    {
      return redis_send( RC_INLINE, "LTRIM '%s' %d %d\r\n", key, start_pos, count_elem);
    }

    /**
     * LTRIM mylist 1 -1
     */
    int SimpleRedisClient::ltrim_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INLINE, "LTRIM")
    }

    /**
     * Выборка с конца очереди (то что попало в очередь раньше других), если все сообщения добавлялись с rpush
     * @param key
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::lpop(const char *key)
    {
      return redis_send( RC_INT, "LPUSH '%s'\r\n", key);
    }

    /**
     * Выборка с начала очереди (то что попало в очередь позже других), если все сообщения добавлялись с rpush
     * @param key
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int SimpleRedisClient::lpop_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_BULK, "LPOP")
    }

    int SimpleRedisClient::rpop(const char *key)
    {
      return redis_send( RC_INT, "RPUSH '%s'\r\n", key);
    }

    int SimpleRedisClient::rpop_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_BULK, "RPOP")
    }

    int SimpleRedisClient::llen(const char *key)
    {
      return redis_send( RC_INT, "LLEN '%s'\r\n", key);
    }

    int SimpleRedisClient::llen_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "LLEN")
    }


    int SimpleRedisClient::lrem(const char *key, int n,const char* val)
    {
      return redis_send( RC_INT, "LLEN '%s' %d '%s' \r\n", key, n, val);
    }

    int SimpleRedisClient::lrem_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "LREM")
    }


    int SimpleRedisClient::lrange(const char *key, int start, int stop)
    {
      return redis_send( RC_INT, "LRANGE '%s' %d %d\r\n", key, start, stop);
    }

    int SimpleRedisClient::lrange_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_MULTIBULK, "LRANGE")
    }


    int SimpleRedisClient::incr(const char *key)
    {
      return redis_send( RC_INT, "INCR '%s'\r\n", key);
    }

    int SimpleRedisClient::incr_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "INCR")
    }

    int SimpleRedisClient::decr(const char *key)
    {
      return redis_send( RC_INT, "DECR '%s'\r\n", key);
    }

    int SimpleRedisClient::decr_printf(const char *format, ...)
    {
        REDIS_PRINTF_MACRO_CODE(RC_INT, "DECR")
    }

    int SimpleRedisClient::operator +=( const char *key)
    {
        return redis_send( RC_INT, "INCR '%s'\r\n", key);
    }

    int SimpleRedisClient::operator -=( const char *key)
    {
        return redis_send( RC_INT, "DECR '%s'\r\n", key);
    }

    /**
     *
     * @param TimeOut
     */
    void SimpleRedisClient::setTimeout( int TimeOut)
    {
      timeout = TimeOut;
    }

    /**
     * Закрывает соединение
     */
    void SimpleRedisClient::redis_close()
    {
        if(debug > RC_LOG_DEBUG) printf("close ok %d\n", fd);
        if(fd != 0 )
        {
            close(fd);
        }
    }


    SimpleRedisClient::operator bool () const
    {
        return fd != 0;
    }

    int SimpleRedisClient::operator == (bool d)
    {
        return (fd != 0) == d;
    }


    char* SimpleRedisClient::getData() const
    {
        return data;
    }

    int SimpleRedisClient::getDataSize() const
    {
        return data_size;
    }
    
