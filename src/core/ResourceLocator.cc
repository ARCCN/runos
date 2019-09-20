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

#include <runos/ResourceLocator.hpp>

#include <runos/core/logging.hpp>
#include <runos/core/throw.hpp>
#include <runos/build-config.hpp>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#ifdef NDEBUG
static fs::path default_etcdir = RUNOS_INSTALL_PREFIX "/etc/runos";
static fs::path default_tooldir = RUNOS_INSTALL_PREFIX "/lib/runos";
#else
static fs::path default_etcdir = RUNOS_SOURCE_ROOT "/etc";
static fs::path default_tooldir = RUNOS_SOURCE_ROOT "/tools";
#endif

namespace runos {

struct ResourceLocatorImpl final : ResourceLocator
{
    path m_etcdir { default_etcdir };
    path m_tooldir { default_tooldir };
    
    explicit
    ResourceLocatorImpl(std::string const& etcdir,
                        std::string const& tooldir)
    {
        path p_etcdir = fs::absolute(etcdir);
        path p_tooldir = fs::absolute(tooldir);

        if (!etcdir.empty()) {
            if (fs::is_directory(p_etcdir)) {
                m_etcdir = p_etcdir;
            } else {
                LOG(WARNING) << "Bad etcdir path: " << etcdir;
            }
        }

        if (!tooldir.empty()) {
            if (fs::is_directory(p_tooldir)) {
                m_tooldir = p_tooldir;
            } else {
                LOG(WARNING) << "Bad tooldir path: " << tooldir;
            }
        }
    }

    path expand(std::string_view res) const
    try {
        path p{std::string(res)};

        if (p.is_absolute()) {
            return p;
        }
        if (p.empty()) {
            THROW(ResourceNotFoundError(std::string(res)),
                  "Empty resource path");
        }
        
        path tail;
        for (auto it = ++p.begin(); it != p.end(); ++it)
            tail += *it;
        std::string head = p.begin()->string();

        if (head == "$etcdir")
            return fs::canonical(tail, m_etcdir);
        else if (head == "$tooldir")
            return fs::canonical(tail, m_tooldir);
        else
            return fs::canonical(p);

    } catch (fs::filesystem_error const& e) {
        THROW_WITH_NESTED(ResourceNotFoundError(std::string(res)),
                          "Can't expand path");
    }

    path find_file(std::string_view res) const override
    {
        path p = expand(res);
        if (not fs::is_regular_file(p)) {
            THROW(ResourceNotFoundError(res), "Can't find resource file: {}",
                  fmt::StringRef(res.data(), res.size()));
        }
        return p;
    }

    path find_executable(std::string_view res) const override
    {
        return find_file(res);
    }

    path find_directory(std::string_view res) const override
    {
        path p = expand(res);
        if (not fs::is_directory(p)) {
            THROW(ResourceNotFoundError(res), "Can't find resource dir: {}",
                  fmt::StringRef(res.data(), res.size()));
        }
        return p;
    }
};

Rc<ResourceLocator>
ResourceLocator::make(std::string const& etcdir, std::string const& tooldir)
{
    return std::make_shared<ResourceLocatorImpl>(etcdir, tooldir);
}

} // runos
