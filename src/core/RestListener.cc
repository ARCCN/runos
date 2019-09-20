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

#include "RestListener.hpp"

// Workaround hack to fix incompatibility between BOOST_FOREACH and Qt
#ifdef foreach
#undef foreach
#endif

#include <runos/core/logging.hpp>
#include <runos/core/catch_all.hpp>

#include <boost/network/include/http/server.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/network/uri.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <thread>
#include <iterator> // back_inseter
#include <functional> // bind

namespace runos {

REGISTER_APPLICATION(RestListener, {""})

namespace rest {
    struct resource_dispatcher {
        virtual void mount(const path_spec& pathspec,
                           resource_mapper mapper) = 0;

        virtual void dispatch(const std::string& path,
                              resource_continuation c) = 0;
    };
}

namespace raw {
    class RawPathExtractor {
    public:
        explicit RawPathExtractor(const std::string& path) noexcept
            : path_(path)
            , rpos_(raw_pos())
            , raw_path_(raw_path())
        { }

        std::string path() const noexcept { return path_.substr(0, rpos_ + 1); }
        std::string rawPath() const noexcept { return raw_path_; }
        bool isRaw() const noexcept { return path_ != raw_path_; }

      private:
        std::string path_;
        size_t rpos_;
        std::string raw_path_;

        size_t raw_pos() const noexcept 
        {
            return std::min(path_.size(), path_.find(RAW_SUBPATH)); 
        }

        std::string raw_path() const noexcept 
        {
            auto pos = rpos_ + RAW_SUBPATH_LENGTH;
            if (pos > path_.size()) {
                return path_;
            }
            auto raw_path = path_.substr(pos);
            std::replace(raw_path.begin(), raw_path.end(), '/', '.');
            raw_path.pop_back();
            return raw_path;
        }

        static constexpr auto RAW_SUBPATH = "/raw/";
        static constexpr auto RAW_SUBPATH_LENGTH = std::strlen(RAW_SUBPATH);
    };

    class GetResponseExtractor {
    public:
        using ptree = rest::ptree;

        explicit GetResponseExtractor(const ptree& data)
            : full_response_(data)
        { }

        ptree extract(const std::string& path) const
        {
            ptree ret;
            auto res = get_value(path);
            ret.put(res.path(), res.value());
            return ret;
        }

    private:
        const ptree& full_response_;

        using ResultBase = std::pair<std::string, std::string>;

        struct Result : ResultBase {
            using ResultBase::ResultBase;

            std::string path() const { return this->first; }
            std::string value() const { return this->second; }
        };

        Result get_value(const std::string& path) const 
        {
            const auto raw_val = full_response_.get_optional<std::string>(path);

            if (!raw_val) {
                return { ERROR, SNMP_ERROR_NO_SUCH_NAME };
            } 

            auto value = parse_value(raw_val.value());
            if (value.empty()) {
                return { ERROR, SNMP_ERROR_BAD_VALUE };
            }

            return { VALUE, value };
        }

        std::string parse_value(std::string value) const
        {
            if (value[0] == ' ') {
                // Ptree returns strings with ' ' at front 
                value.erase(0);
            }
            return value;
        }

        static constexpr auto ERROR = "Error";
        static constexpr auto VALUE = "Value";
        static constexpr auto SNMP_ERROR_NO_SUCH_NAME = "2";
        static constexpr auto SNMP_ERROR_BAD_VALUE = "3";
    };
} // namespace raw

namespace http = boost::network::http;
namespace uri = boost::network::uri;

typedef http::server<struct RestHandler>
    rest_server;

using boost::property_tree::json_parser::read_json;
using boost::property_tree::json_parser::write_json;
using boost::property_tree::json_parser::json_parser_error;

struct RestHandler {
    typedef rest_server::request request;
    typedef rest_server::connection connection;
    typedef rest_server::connection_ptr connection_ptr;
    typedef rest_server::string_type string_type;

    explicit RestHandler(rest::resource_dispatcher& dispatcher)
        : dispatcher_(dispatcher)
    { }

    size_t get_content_length(request const& req)
    {
        auto content_len_it = boost::find_if(req.headers,
                [](request::header_type const& h) {
                    return boost::iequals(h.name, "content-length");
                });

        if (content_len_it != req.headers.end())
        try {
            return boost::lexical_cast<size_t>(content_len_it->value);
        } catch (const boost::bad_lexical_cast&) {
            THROW(rest::http_error(400), "Bad Content-Length: {}",
                  content_len_it->value );
        }

        return 0;
    }

    std::string path(request const& req)
    {
        // cpp-netlib can't parse uri without scheme and host,
        // so add them manually
        auto url_str = std::string("http://localhost") + req.destination;
        boost::network::uri::uri url(url_str);

        if (!url.is_valid()) {
            THROW(rest::http_error(connection::bad_request), "Bad url");
        }

        return url.path();
    }

    void dispatch(std::string path, rest::resource_continuation c)
    {
        dispatcher_.dispatch(path, c);
    }

    template<class Callable>
    void exception_guard(request const& req,
                         connection_ptr connection,
                         Callable&& callable)
    {
        if (auto diag = catch_all_and_log([&]() {
            try {
                callable();
            } catch (const rest::http_error& err) {
                rest::ptree resp;
                resp.put("error.type", "http_error");
                resp.put("error.code", err.code());
                resp.put("error.msg", err.what());
                respond( req, connection, resp, (connection::status_t) err.code() );
            } catch (const json_parser_error& e) {
                rest::ptree resp;
                resp.put("error.type", "json_parser_error");
                resp.put("error.msg", e.message());
                resp.put("error.line", e.line());
                respond( req, connection, resp, connection::bad_request );
            }
        })) {
            rest::ptree resp;
            resp.put("error.type", "internal_error");
            respond( req, connection, resp, connection::internal_server_error );
        }
    }

#define EXCEPTION_GUARD(req, connection) \
    { exception_guard(req, connection, [&, this]()
#define EXCEPTION_GUARD_END \
    );}

    void operator()(request const& req_, connection_ptr connection)
    EXCEPTION_GUARD(req_, connection)
    {
        request req = req_; // copy to read-write
        size_t content_len;

        if (( content_len = get_content_length(req) ))
            req.body.reserve( content_len );

        read_more(std::move(req)
                , content_len
                , boost::make_iterator_range("")
                , connection->has_error() ?
                      connection->error()->code()
                    : boost::system::error_code()
                , 0
                , connection);
    }
    EXCEPTION_GUARD_END

    void read_more(request req, size_t content_length,
                   boost::iterator_range<char const *> read,
                   boost::system::error_code ec,
                   size_t bytes_transferred,
                   connection_ptr connection)
    EXCEPTION_GUARD(req, connection)
    {
        auto& body = req.body;

        if (bytes_transferred > 0) {
            std::copy( read.begin(), read.begin() + bytes_transferred,
                       std::back_inserter(body) );
        }

        if (ec == boost::asio::error::eof || body.length() >= content_length) {
            THROW_IF(body.length() != content_length, rest::http_error(400),
                     "Bad Content-Length");
            request_ready(req, connection);
        } else if (ec) {
            throw boost::system::system_error(ec);
        } else {
            using std::placeholders::_1;
            using std::placeholders::_2;
            using std::placeholders::_3;
            using std::placeholders::_4;
            connection->read( std::bind( &RestHandler::read_more
                                       , this, std::move(req), content_length
                                       , _1, _2, _3, _4) );
        }
    }
    EXCEPTION_GUARD_END

    void request_ready(request const& req, connection_ptr connection)
    EXCEPTION_GUARD(req, connection)
    {
        if (req.method == "HEAD") {
            bool found;
            dispatch(path(req), [&](rest::resource& r) {
                found = r.Head();
            });

            respond( req, connection, rest::ptree(), found ?
                     connection::ok
                   : connection::not_found );
        } else if (req.method == "GET") {
            rest::ptree resp;
            raw::RawPathExtractor path_parser(path(req));

            dispatch(path_parser.path(), [&](rest::resource& r) {
                resp = r.Get();
                if (path_parser.isRaw()) {
                    auto raw_path = path_parser.rawPath();
                    raw::GetResponseExtractor extractor(resp);
                    resp = extractor.extract(raw_path);
                }
            });

            respond(req, connection, resp);
        } else if (req.method == "PUT" || req.method == "POST") {
            bool post = req.method == "POST";

            std::istringstream body_stream(req.body);
            rest::ptree req_json;
            read_json(body_stream, req_json);
            
            rest::ptree resp;
            dispatch(path(req), [&](rest::resource& r) {
                resp = post ? r.Post(req_json) : r.Put(req_json);
            });

            respond(req, connection, resp);
        } else if (req.method == "DELETE") {
            rest::ptree resp;
            dispatch(path(req), [&](rest::resource& r) { 
                    resp = r.Delete(); 
            });
            
            respond(req, connection, resp);
        } else {
            THROW(rest::http_error(405), "Unknown method: {}", req.method);
        }
    }
    EXCEPTION_GUARD_END

    void respond(
        request const&
      , connection_ptr connection
      , const rest::ptree body
      , connection::status_t status = connection::ok
    ) try {
        std::string resp_str;

        try {
            std::ostringstream ss;
            write_json(ss, body);
            resp_str = ss.str();
            connection->set_status(status);
        } catch (json_parser_error& e) {
            std::ostringstream ss;
            ss << R"({ "error": {)"
                  R"(  "type": "write_json_error",)"
                  R"(  "msg": )" << '"' << e.what() << '"' << "} }";
            resp_str = ss.str();
            connection->set_status(connection::internal_server_error);
        }

        std::vector<rest_server::response_header>
            headers;

        headers.push_back({"Content-Length", std::to_string(resp_str.size())});

        if (body.find("array") != body.not_found() &&
            body.find("_size") != body.not_found())
        {
            headers.push_back(
                    {"Content-Type", "application/x-collection+json"});
        } else {
            headers.push_back(
                    {"Content-Type", "application/x-resource+json"});
        }

        connection->set_headers(headers);
        connection->write( std::move(resp_str) );

    } catch (boost::system::system_error const& e) {
        LOG(ERROR) << "Rest handler failed: " << e.what();
    } catch (...) {
        LOG(ERROR) << "Rest handler failed";
    }

    void error(boost::system::error_code const& ec)
    {
        LOG(ERROR) << "Rest error: " << ec.message();
    }

protected:
    rest::resource_dispatcher& dispatcher_;

#undef EXCEPTION_GUARD_END
#undef EXCEPTION_GUARD
};

struct RestListener::implementation final
    : rest::resource_dispatcher
{
    std::vector< std::pair<rest::path_spec, rest::resource_mapper> >
        mounts;

    void mount(const rest::path_spec& pathspec, rest::resource_mapper mapper)
        override;

    void dispatch(const std::string& path, rest::resource_continuation c)
        override;

    RestHandler handler{ *this };
    std::unique_ptr< rest_server > server;
};

RestListener::RestListener()
    : impl(std::make_unique<implementation>())
{ }

RestListener::~RestListener() noexcept = default;

void RestListener::implementation::mount(
    const rest::path_spec& pathspec, rest::resource_mapper mapper
) {
    mounts.emplace_back(pathspec, mapper);
}

void RestListener::implementation::dispatch
    (const std::string& path, rest::resource_continuation c)
{
    rest::path_match match;

    for (auto& mount : mounts) {
        if (std::regex_match(path, match, mount.first)) {
            mount.second(match, c);
            return;
        }
    }

    THROW(rest::http_error(404), "No route matched");
}

void RestListener::init(Loader*, const Config& rootConfig)
{
    auto config = config_cd(rootConfig, "rest-listener");

    rest_server::options options(impl->handler);
    options.address(config_get(config, "address", "127.0.0.1"))
           .port(config_get(config, "port", "8000"));

    impl->server.reset(new rest_server(options));
}

void RestListener::startUp(Loader*)
{
    std::thread{[this]{impl->server->run();}}.detach();
}

void RestListener::mount_impl(const rest::path_spec& pathspec,
                              rest::resource_mapper mapper)
{
    impl->mount(pathspec, mapper);
}

} // namespace runos
