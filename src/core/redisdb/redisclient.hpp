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

#ifndef __REDISCLIENT__H_
#define	 __REDISCLIENT__H_

#include <string>
#include <vector>

#define RC_NULL 0
#define RC_ERR -1
  
#define RC_ERR_CONECTION_CLOSE -2
#define RC_ERR_SEND -101
#define RC_ERR_TIMEOUT -102
#define CR_ERR_RECV -103

#define RC_ERR_PROTOCOL -104
#define RC_ERR_BUFFER_OVERFLOW -105
#define RC_ERR_DATA_FORMAT -106

#define RC_ERR_DATA_BUFFER_OVERFLOW -107
 

#define RC_LOG_NONE 0
#define RC_LOG_ERROR 1
#define RC_LOG_WARN 2
#define RC_LOG_LOG 3
#define RC_LOG_DEBUG 4

/**
 * Тип ожидаемых от редиса данных
 */
#define RC_ERROR '-'

/**
 * Тип ожидаемых от редиса данных
 * Строка в ответе
 * @see http://redis.io/topics/protocol
 */
#define RC_INLINE '+'

/**
 * Тип ожидаемых от редиса данных
 * Определяет длину аргумента ответа
 * -1 нет ответа
 * @see http://redis.io/topics/protocol
 */
#define RC_BULK '$'
 
/**
 * Тип ожидаемых от редиса данных
 * Определяет количество аргументов ответа
 * -1 нет ответа
 * @see http://redis.io/topics/protocol
 */
#define RC_MULTIBULK '*'

/**
 * Тип ожидаемых от редиса данных
 */
#define RC_INT ':'

/**
 * Тип ожидаемых от редиса данных
 */
#define RC_ANY '?'

/**
 * Тип ожидаемых от редиса данных
 */
#define RC_NONE ' '

int read_int(const char* buffer, char delimiter, int* delta);
int read_int(const char* buffer, char delimiter);
int read_int(const char* buffer, int* delta);

long read_long(const char* buffer, char delimiter, int* delta);
long read_long(const char* buffer, char delimiter);
long read_long(const char* buffer, int* delta);

class SimpleRedisClient
{
    int fd = 0;
    int yes = 1;
    int timeout = 1000;

    char* buffer = 0;
    char* buf = 0;
    int buffer_size = 0;

    /**
     * Максимально допустимый размер буфера
     */
    int max_buffer_size = 1000000;
    
    int port = 6379;
    char* host = 0;
    
    int version_major = -1;
    int version_minor = -1;
    int version_patch = -1;
    
    /**
     * Хранит количество пришедших данных в ответе.
     * Для списка это колво элементов для остальных типов это колво байт.
     */
    unsigned int data_size = 0;
    char* data = 0;
    int debug = 0;
    int last_error = 0;
    std::vector<std::string> answer_multibulk_vec_;
    
private:
    
    /**
     * Запрещаем копирование для объектов донного класса
     * Если открыто соединение с редисом, а потом выполнена копия объекта
     *  то при удалении любого из объектов в деструкторе соединение закроется.
     *  ,а при удалении второго вообще ни чего хорошего ждать не надо.
     * 
     * Возможно в следующих версиях библиотеки будет реализован адекватный конструктор копирования.
     */
    SimpleRedisClient(const SimpleRedisClient& ) = delete;
    
    /**
     * Запрещаем копирование для объектов донного класса
     * Возможно в следующих версиях библиотеки будет реализован адекватный конструктор копирования.
     */
   void operator=( const SimpleRedisClient& ) = delete;
    
public:
    
    void LogLevel(int);
    int LogLevel(void);

    SimpleRedisClient();
    
    void setPort(int Port);
    int getPort();
    
    void setHost(const char* Host);
    char* getHost() const;
    
    virtual ~SimpleRedisClient();

    int redis_connect();
    int redis_connect(const char* Host, int Port);
    int redis_connect(const char* Host,int Port, int TimeOut);
    
    /* 
      vector3 operator-=(float v); 
      bool    operator==(vector3 v);
     */
    
    int getMultiBulkVectorData();
    
    /**
     * Ни ключь ни значение не должны содержать "\r\n"
     * @param key
     * @param val
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int set(const char *key, const char *val);
    
    /**
     * Операция set только с форматированием строки параметров.
     * Ключ от значения отделяется пробелом.
     * @param format
     * @param ... ключь пробел значение
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int set_printf(const char *format, ...);
    
    /**
     * 
     * @param key Ключ
     * @param format
     * @param ... значение
     * @return 
    int set_printf(const char *key, const char *format, ...);
     */
    
    /**
     * Выполняет установку значения в редис
     * Ключ от значения отделяется пробелом.
     * @code rc = "MyKey MyValue";
     * 
     * 
     * @param key_val
     * @return 
     */
    SimpleRedisClient& operator=(const char *key_val);
     
    /**
     * Вернёт значение ключа или 0 в случаии отсутсвия ключа или ошибки
     * Пример:
     * 
     * char* val = rc["key"];
     * if(val != 0)
     * {
     *    printf("VAL:%s\n", val);
     * }
     *  
     * Выдаётся указатьель на внутриний буфер чтения/записи ссылка потеряет актуальность при вызове любой функции на получение или запись данных.
     * Или по указаному адресу будут уже другие данные либо мусор.
     */
    char* operator[] (const char *key);
    
    
    /**
     * Равносильно методу getData()
     * @return 
     */
    operator char* () const;
    
    /**
     * Равносильно методу getData() с преобразованием в int
     * @return 
     */
    operator int () const;
    
    /**
     * Равносильно методу getData() с преобразованием в long
     * @return 
     */
    operator long () const;
    
    /**
     * Инкриментирует значение ключа key, эквивалентно incr
     * @param key ключь в редисе
     * @return 
     */
    int operator +=( const char *key);
    
    /**
     * Декриментирует значение ключа key, эквивалентно decr
     * @param key ключь в редисе
     * @return 
     */ 
    int operator -=( const char *key); 

    /**
     * Вернёт true если соединение установлено 
     */
    operator bool () const;
     
    /**
     *  rc == true  истино если соединение установлено
     *  rc == false  истино если соединение не установлено 
     */
    int operator == (bool); 
    
    int incr(const char *key);
    int incr_printf(const char *format, ...);
    
    int decr(const char *key);
    int decr_printf(const char *format, ...);
    
    int setex(const char *key, const char *val, int seconds);
    int setex_printf(int seconds, const char *key, const char *format, ...);
    
    /**
     * Формат должен иметь вид: key seconds data
     * Где 
     * key строковый ключ
     * seconds %d число секунд
     * data строковые данные
     * 
     * @param format
     * @param ...
     * @return  Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int setex_printf(const char *format, ...);

    int get(const char *key);
    int get_printf(  const char *format, ...);
    
    
    /**
     * Set the string value of a key and return its old value
     * @param key
     * @param set_val
     * @param get_val
     * @return  Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int getset(const char *key, const char *set_val);
    int getset_printf(const char *format, ...);

    /**
     * Ping the server
     */
    int ping();

    int echo(const char *message);

    int quit();

    int auth(const char *password);
   
    int setnx(const char *key, const char *val);
    int setnx_printf(const char *format, ...);

    /**
     * http://redis.io/commands
     * 
     * 
     * static int cr_incr(REDIS rhnd, int incr, int decr, const char *key, int *new_val)
     * credis_incr
     * credis_decr
     * credis_incrby
     * credis_decrby
     * 
     * 
     * cr_multikeybulkcommand
     * cr_multikeystorecommand
     * credis_mget
     * 
     * Set multiple keys to multiple values
     * MSET key value [key value ...]
     */ 

    int append(const char *key, const char *val);
    int append_printf(const char *format, ...);

    int substr( const char *key, int start, int end);

    int exists( const char *key);
    int exists_printf(const char *format, ...);

    int del( const char *key);
    int del_printf(const char *format, ...);
    
    int type( const char *key);

    int keys(const char *pattern);
    int keys_printf(const char *format, ...);

    int randomkey();

    int rename( const char *key, const char *new_key_name);
    int rename_printf(const char *format, ...);

    int renamenx( const char *key, const char *new_key_name);
    int renamenx_printf(const char *format, ...);

    /**
     * Return the number of keys in the selected database 
     */
    int dbsize();

    int expire( const char *key, int secs);
    int expire_printf(const char *format, ...);

     
    int flushall(void);

    int sadd(const char *key, const char *member);
    int sadd_printf(const char *format, ...);

    int srem(const char *key, const char *member);
    int srem_printf(const char *format, ...);

    int smembers(const char *key);
    int smembers_printf(const char *format, ...);
    
    int scard(const char *key);
    int scard_printf(const char *format, ...);
    
    int lpush(const char *key, const char *member);
    int lpush_printf(const char *format, ...);
    
    int rpush(const char *key, const char *member);
    int rpush_printf(const char *format, ...);
    
    int ltrim(const char *key, int start_pos, int count_elem);
    int ltrim_printf(const char *format, ...);
    
    int lpop(const char *key);
    int lpop_printf(const char *format, ...);
    
    int rpop(const char *key);
    int rpop_printf(const char *format, ...);
    
    int llen(const char *key);
    int llen_printf(const char *format, ...);
    
    int lrem(const char *key, int n,const char* val);
    int lrem_printf(const char *format, ...);
    
    int lrange(const char *key, int start, int stop);
    int lrange_printf(const char *format, ...);

    int setMasterRole();
    int setSlaveOf(const char *address, const char *port);
    int setSlaveReadOnlyNo();

    /**
     * Returns the remaining time to live of a key that has a timeout.
     * @param key
     * @return 
     */
    int ttl( const char *key);
    int ttl_printf(const char *format, ...);
     
    
    int getRedisVersion();
    
    /**
     * 
     * @param TimeOut
     */
    void setTimeout( int TimeOut);

    /**
     * Закрывает соединение
     */
    void redis_close();
        
    /** 
     * Выдаётся указатьель на внутриний буфер чтения/записи ссылка потеряет актуальность при вызове любой функции на получение или запись данных.
     * Или по указаному адресу будут уже другие данные либо мусор. 
     */
    char* getData() const;

    std::vector<std::string> getVectorData() const;
    
    /**
     * Возвращает количество пришедших данных в ответе.
     * Для списка это колво элементов для остальных типов это колво байт.
     */
    int getDataSize() const;
     
    void setBufferSize(int size);
    int getBufferSize();
    
    void setMaxBufferSize(int size);
    int getMaxBufferSize();
    
    /**
     * Выбор бызы данных
     * @param index
     * @see http://redis.io/commands/select
     */
    int selectDB(int index);
    
    int getError();
    
    int redis_raw_send(char recvtype, const char *buffer);
protected:
    
    char* lastAuthPw = nullptr;
    int lastSelectDBIndex = 0;
      
    int reconnect( );
    
    
    int read_select(int fd, int timeout )  const;
    
    int wright_select(int fd, int timeout )  const;
    
    /**
     * Отправляет запрос редису
     * @param recvtype тип ожидаемого результата
     * @param format Строка запроса
     * @param ...
     * @return Если меньше нуля то код ошибки, а если больше нуля то количество принятых байт
     */
    int redis_send(char recvtype, const char *format, ...);
    
    /**
     * Отправляет данные
     * @param buf
     * @return 
     */
    int send_data( const char *buf ) const;

    void parse_multibulk_data();
 
};


#endif	/* __REDISCLIENT__H_ */


