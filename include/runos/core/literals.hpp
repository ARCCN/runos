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

#pragma once

/**
 * Common registry for User-Defined literals used to avoid collisions.
 */

/* Forward declarations */
namespace std
{
    inline namespace literals { }
}

namespace fmt
{
    inline namespace literals { }
}

/* Forward "imports" */
namespace runos {

inline namespace literals {
    /*
     * Don't care about collisions of std::literals because they starts from letter.
     * All non-standard literals should start from '_'.
     */
    using namespace std::literals;

    /*
     * _format(const char*, size_t)
     * _a(const char*, size_t)
     */
    using namespace fmt::literals;
}

} // runos
