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

#include "FluidOXMAdapter.hpp"
#include "oxm/field_set.hh"

using namespace fluid_msg;

namespace runos {

FluidOXMAdapter::FluidOXMAdapter(const oxm::field<> f)
    : OXMTLV(static_cast<uint16_t>(f.type().ns()),
             static_cast<uint8_t>(f.type().id()),
             not f.exact(),
             f.type().nbytes() * (f.exact() ? 1 : 2)),
      field_(f)
{
    create_oxm_req(0, 0, 0, 0);
}

of13::OXMTLV& FluidOXMAdapter::operator=(const of13::OXMTLV& other)
{
    const FluidOXMAdapter& rhs =
        dynamic_cast<const FluidOXMAdapter&>(other);
    OXMTLV::operator=(rhs);
    field_ = rhs.field_;
    return *this;
}

bool FluidOXMAdapter::equals(const of13::OXMTLV& other)
{
    if (const FluidOXMAdapter* other_adapter =
            dynamic_cast<const FluidOXMAdapter*>(&other))
    {
        return field_ == other_adapter->field_;
    } else {
        return false;
    }
}

FluidOXMAdapter* FluidOXMAdapter::clone() const
{
    return new FluidOXMAdapter(*this);
}

size_t FluidOXMAdapter::pack(uint8_t* buffer)
{
    OXMTLV::pack(buffer);
    field_.value_bits().to_buffer(buffer + of13::OFP_OXM_HEADER_LEN);
    if (not field_.exact()) {
        field_.mask_bits().to_buffer(buffer + of13::OFP_OXM_HEADER_LEN
                                            + field_.type().nbytes());
    }
    return 0;
}

of_error FluidOXMAdapter::unpack(uint8_t* buffer)
{
    OXMTLV::unpack(buffer);
    
    auto value_bits = bits<>(field_.type().nbits(),
                             buffer + of13::OFP_OXM_HEADER_LEN);
    if (has_mask_) {
        auto mask_bits = bits<>(field_.type().nbits(),
                                buffer
                                    + of13::OFP_OXM_HEADER_LEN
                                    + field_.type().nbytes());

        field_ = oxm::field<>(field_.type(), std::move(value_bits),
                                             std::move(mask_bits));
    } else {
        field_ = oxm::field<>(field_.type(), std::move(value_bits));
    }
    return 0;
}

of13::Match make_of_match(const oxm::field_set &match)
{
    of13::Match ret;
    for (const oxm::field<>& field : match)
        ret.add_oxm_field(new FluidOXMAdapter(field));
    return ret;
}

} // namespace runos
