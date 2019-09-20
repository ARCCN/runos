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

#include <cstdint>
#include <cstddef>
#include <fluid/of13/of13match.hh>

#include "oxm/field.hh"
#include "oxm/field_set_fwd.hh"

namespace runos {

class FluidOXMAdapter : public fluid_msg::of13::OXMTLV {
    oxm::field<> field_;
public:
    FluidOXMAdapter(const oxm::field<> f);
    OXMTLV& operator=(const OXMTLV& other) override;
    FluidOXMAdapter* clone() const override;

    bool equals(const OXMTLV& other) override;

    size_t pack(uint8_t* buffer) override;
    fluid_msg::of_error unpack(uint8_t* buffer) override;
};

fluid_msg::of13::Match make_of_match(const oxm::field_set &match);

} // namespace runos
